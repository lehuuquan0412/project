#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#include <drivers/pin.h>

#define DBG_ENABLE
#define DBG_SECTION_NAME    "mqtt.user"
#define DBG_LEVEL           DBG_LOG
#define DBG_COLOR
#include <rtdbg.h>

#include "paho_mqtt.h"
#include "jsmn.h"
#include "mqtt.h"
#include "ppp_sp.h"
#include "connection_manager.h"

#include "obd2.h"

rt_mq_t mq_mqtt_recv = RT_NULL;
rt_mq_t mq_mqtt_pub = RT_NULL;

static int mqtt_send(int argc, const char **argv);

/**
 * MQTT URI farmat:
 * domain mode
 * tcp://iot.eclipse.org:1883
 *
 * ipv4 mode
 * tcp://192.168.10.1:1883
 * ssl://192.168.10.1:1884
 *
 * ipv6 mode
 * tcp://[fe80::20c:29ff:fe9a:a07e]:1883
 * ssl://[fe80::20c:29ff:fe9a:a07e]:1884
 */
#define MQTT_URI                "tcp://test.mosquitto.org:1883"
#define MQTT_USERNAME           "admin"
#define MQTT_PASSWORD           "admin"
#define MQTT_SUBTOPIC           "/req"
#define MQTT_PUBTOPIC           "BACKKHOAOBD2/data"
#define MQTT_WILLMSG            "{\"2000/status\":\"free\"}"
#define MQTT_FREE               "{\"2000/status\":\"free\"}"


/* define MQTT client context */
MQTTClient client;



static rt_base_t led_connect_status;

static rt_thread_t thread_mqtt_recv_id = RT_NULL;
static rt_thread_t thread_mqtt_pub_id = RT_NULL;


/* JSON handle start */

static int fetch_key(const char *JSON_STRING, jsmntok_t *t, const char *str)
{
    int length;
    length = strlen(str);
    if ((t->type == JSMN_STRING) && (length == (t->end - t->start)))
    {
        return strncmp(JSON_STRING + t->start, str, length);
    }
    else
    {
        return -1;
    }
}

/* JSON handle end */


static void mqtt_sub_callback(MQTTClient *c, MessageData *msg_data)
{
    *((char *)msg_data->message->payload + msg_data->message->payloadlen) = '\0';
    LOG_D("mqtt sub callback: %.*s %.*s",
               msg_data->topicName->lenstring.len,
               msg_data->topicName->lenstring.data,
               msg_data->message->payloadlen,
               (char *)msg_data->message->payload);
    rt_mq_send(mq_mqtt_recv, msg_data->message->payload, msg_data->message->payloadlen+1);
}

static void mqtt_sub_default_callback(MQTTClient *c, MessageData *msg_data)
{
    *((char *)msg_data->message->payload + msg_data->message->payloadlen) = '\0';
    LOG_D("mqtt sub default callback: %.*s %.*s",
               msg_data->topicName->lenstring.len,
               msg_data->topicName->lenstring.data,
               msg_data->message->payloadlen,
               (char *)msg_data->message->payload);
}

static void mqtt_connect_callback(MQTTClient *c)
{
    LOG_D("inter mqtt_connect_callback!");
    setConnectState(empty);
    paho_mqtt_publish(&client, QOS1, MQTT_PUBTOPIC, MQTT_FREE);
    rt_pin_write(led_connect_status, PIN_LOW);
}

static void mqtt_online_callback(MQTTClient *c)
{
    LOG_D("inter mqtt_online_callback!");
    mqtt_send(0, "hello");
}

static void mqtt_offline_callback(MQTTClient *c)
{
    LOG_D("inter mqtt_offline_callback!");
    setConnectState(offline);
    rt_pin_write(led_connect_status, PIN_HIGH);
}

int mqtt_connect_init(void)
{
    while(!ppp_get_status_connect(ppp_device_get()));
    rt_thread_mdelay(5000);
    char mqtt_sub_topic[100];

    rt_sprintf(mqtt_sub_topic, "%s%s", OBD_ID, MQTT_SUBTOPIC);
    //rt_sprintf(will_msg, "{\"%s/%s", OBD_ID, MQTT_WILLMSG);

    led_connect_status = GET_PIN(A, 7);
    rt_pin_mode(led_connect_status, PIN_MODE_OUTPUT);
    rt_pin_write(led_connect_status, PIN_HIGH);
    /* init condata param by using MQTTPacket_connectData_initializer */
    MQTTPacket_connectData condata = MQTTPacket_connectData_initializer;
    static char cid[20] = { 0 };

    setConnectState(offline);
    /* config MQTT context param */
    {
        client.isconnected = 0;
        client.uri = MQTT_URI;

        /* generate the random client ID */
        rt_snprintf(cid, sizeof(cid), "rtthread%d", rt_tick_get());
        /* config connect param */
        memcpy(&client.condata, &condata, sizeof(condata));
        client.condata.clientID.cstring = cid;
        client.condata.keepAliveInterval = 60000;
        client.condata.cleansession = 1;
//        client.condata.username.cstring = MQTT_USERNAME;
//        client.condata.password.cstring = MQTT_PASSWORD;

        /* config MQTT will param. */
        client.condata.willFlag = 1;
        client.condata.will.qos = 1;
        client.condata.will.retained = 0;
        client.condata.will.topicName.cstring = MQTT_PUBTOPIC;
        client.condata.will.message.cstring = MQTT_WILLMSG;

        /* malloc buffer. */
        client.buf_size = client.readbuf_size = 1024;
        client.buf = rt_calloc(1, client.buf_size);
        client.readbuf = rt_calloc(1, client.readbuf_size);
        if (!(client.buf && client.readbuf))
        {
            LOG_E("no memory for MQTT client buffer!");
            return -1;
        }

        /* set event callback function */
        client.connect_callback = mqtt_connect_callback;
        client.online_callback = mqtt_online_callback;
        client.offline_callback = mqtt_offline_callback;

        /* set subscribe table and event callback */
        client.messageHandlers[0].topicFilter = rt_strdup(mqtt_sub_topic);
        client.messageHandlers[0].callback = mqtt_sub_callback;
        client.messageHandlers[0].qos = QOS1;

        /* set default subscribe event callback */
        client.defaultMessageHandler = mqtt_sub_default_callback;
    }

    /* run mqtt client */
    paho_mqtt_start(&client);
    return 0;
}

int mqtt_user_publish(char *msg)
{
    return rt_mq_send(mq_mqtt_pub, msg, strlen(msg)+1);
}

static void mqtt_publish_handle_thread(void * params)
{
    char msg[128];
    char value[128];

    paho_mqtt_publish(&client, QOS1, MQTT_PUBTOPIC, MQTT_FREE);

    while(1)
    {
        rt_mq_recv(mq_mqtt_pub, msg, sizeof(msg), RT_WAITING_FOREVER);
        //rt_kprintf("Data: %s\n", msg);
        rt_sprintf(value, "%s", msg);
        paho_mqtt_publish(&client, QOS1, MQTT_PUBTOPIC, msg);
        rt_thread_mdelay(50);
    }
}

static void mqtt_recieve_handle_thread(void * params)
{
    char msg[128];
    char value_str[10];
    char mqtt_msg[100];

    int res;
    int mode;
    int value;
    int key;
    jsmn_parser p;
    jsmntok_t t[128] = {0};

    while(1)
    {
        jsmn_init(&p);
        mode = 0, value = 0, key = 0;
        rt_mq_recv(mq_mqtt_recv, msg, sizeof(msg), RT_WAITING_FOREVER);
        res = jsmn_parse(&p, msg, strlen(msg), t, sizeof(t) / sizeof(t[0]));
        if (res < 0)
        {
            rt_kprintf("Failed to parse JSON: %d\n", res);
            continue;
        }

        if (res < 1 || t[0].type != JSMN_OBJECT)
        {
            rt_kprintf("Object expected\n");
            continue;
        }

        if (res < 3)
        {
            continue;
        }

        if (fetch_key(msg, &t[1], "mode") == 0)
        {
            sprintf(value_str, "%.*s", t[2].end - t[2].start, msg + t[2].start);
            mode = atoi(value_str);
            rt_kprintf("Mode: %d\n", mode);
        }else if (fetch_key(msg, &t[1], "connect") == 0)
        {
            sprintf(value_str, "%.*s", t[2].end - t[2].start, msg + t[2].start);
            mode = atoi(value_str);
            connectHandle(mode);
            if (getConnectState() == connected)
            {
                sprintf(mqtt_msg, "{\"%s/status\":\"connected\", \"%s/code\":\"%d\"}", OBD_ID, OBD_ID, getConnectCode());
                mqtt_user_publish(mqtt_msg);
                LOG_D("Connected with: %d, %d\n", getConnectState(), getConnectCode());
            }
            continue;
        }if (fetch_key(msg, &t[1], "disconnect") == 0)
        {
            sprintf(value_str, "%.*s", t[2].end - t[2].start, msg + t[2].start);
            mode = atoi(value_str);
            if (mode == getConnectCode())
            {
                disconnectHandle();
                sprintf(mqtt_msg, "{\"%s/status\":\"free\", \"%s/code\":\"%d\"}", OBD_ID, OBD_ID, getConnectCode());
                mqtt_user_publish(mqtt_msg);
                LOG_D("Disconnected with: %d, %d\n", getConnectState(), getConnectCode());
            }
            continue;
        }

        if (res >= 5)
        {
            if ((fetch_key(msg, &t[3], "value") == 0))
            {
                sprintf(value_str, "%.*s", t[4].end - t[4].start, msg + t[4].start);
                value = atoi(value_str);
                rt_kprintf("Value: %d\n", value);
            }
        };

        if (res >= 7)
        {
            if ((fetch_key(msg, &t[5], "key") == 0))
            {
                sprintf(value_str, "%.*s", t[6].end - t[6].start, msg + t[6].start);
                key = atoi(value_str);
                rt_kprintf("Key: %d\n", key);
            }
        }

        obd_handle_request(mode, value, key);
    }
}

static int mqtt_control_init(void)
{
    mq_mqtt_recv = rt_mq_create("mq_mqtt_recv", 128, 10, RT_IPC_FLAG_FIFO);
    mq_mqtt_pub = rt_mq_create("mq_mqtt_pub", 128, 30, RT_IPC_FLAG_FIFO);

    thread_mqtt_pub_id = rt_thread_create("MQTT pub", mqtt_publish_handle_thread, NULL, 2048, 25, 5);

    if (thread_mqtt_pub_id != RT_NULL)
    {
        rt_thread_startup(thread_mqtt_pub_id);
    }else{
        rt_kprintf("Failed mqtt thread init: %d\n", thread_mqtt_pub_id->error);
        return -RT_ERROR;
    }

    thread_mqtt_recv_id = rt_thread_create("MQTT recv", mqtt_recieve_handle_thread, NULL, 3500, 25, 5);

    if (thread_mqtt_recv_id != RT_NULL)
    {
        rt_thread_startup(thread_mqtt_recv_id);
    }else{
        rt_kprintf("Failed mqtt thread init: %d\n", thread_mqtt_recv_id->error);
        return -RT_ERROR;
    }

    return RT_EOK;
}

int mqtt_get_status(void)
{
    return getConnectState();
}

int mqtt_system_init(void)
{
    int ret = mqtt_connect_init();
    ret = mqtt_control_init();
    return ret;
}

#ifndef PKG_USING_CMUX
//INIT_APP_EXPORT(mqtt_connect_init);
//INIT_APP_EXPORT(mqtt_control_init);
#endif

static int mqtt_send(int argc, const char **argv)
{
    mqtt_user_publish(MQTT_FREE);
    return RT_EOK;
}

#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT(mqtt_send, mqtt_send_data);
#endif /* FINSH_USING_MSH */


