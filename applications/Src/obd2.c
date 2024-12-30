/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-12-16     Admin       the first version
 */
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <can_user.h>

#include <drivers/pin.h>
#include <drivers/can.h>
#include <rthw.h>

#include <string.h>

#include "obd2.h"
#include "mqtt.h"
#include "connection_manager.h"



static rt_thread_t thread_single_handle_id = RT_NULL;
static rt_thread_t thread_multi_handle_id = RT_NULL;
static rt_thread_t thread_handle_multi_msg_id = RT_NULL;
static rt_thread_t thread_reques_handle = RT_NULL;

rt_mq_t mq_handle_multi_frame = RT_NULL;

int service_1_value = 0;
int service = 0;
int uds_mode = 0;
int uds_key = 0;
int injector_num = 0;

int mode6_sid[6] = {0x01, 0x21, 0xa2, 0xa3, 0xa4, 0xa5};

uint8_t SERVICE_1_PID[32] =
{
        0x04, 0x05, 0x06, 0x07, 0x0b, 0x0c, 0x0d, 0x0e,
        0x0f, 0x11, 0x14, 0x15, 0x1f, 0x21, 0x2e, 0x30,
        0x31, 0x33, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
        0x49, 0x4a, 0x4c, 0x51, 0x5a, 0, 0, 0
};

static void can_request_handle_thread(void * params)
{
    struct rt_can_msg msg;
    msg.id = 0x7e0;
    msg.ide = CAN_ID_STD;
    msg.rtr = CAN_RTR_DATA;
    msg.len = 8;
    msg.data[0] = 0;
    msg.data[1] = 0;
    msg.data[2] = 0;
    msg.data[3] = 0;
    msg.data[4] = 0;
    msg.data[5] = 0;
    msg.data[6] = 0;
    msg.data[7] = 0;

    while (1)
    {
        while(getConnectState() != connected);

        msg.id = 0x7e0;

        if (service == 1)
        {
            //rt_kprintf("Data: %d\n", service_1_value);
            msg.data[0] = 0x02;
            msg.data[1] = 0x01;

            for (uint8_t i = 0; i < 29; i++)
            {
                if (service_1_value & (1 << i))
                {
                    msg.data[2] = SERVICE_1_PID[i];
                    //printf("i = %d\n", i);
                    rt_mq_send(mq_can_send, &msg, sizeof(msg));
                    rt_thread_mdelay(50);
                    if (uds_mode == 0x30)
                    {
                        msg.data[0] = 0x03;
                        msg.data[1] = 0x30;
                        msg.data[2] = uds_key;
                        msg.data[3] = 0x01;
                        rt_mq_send(mq_can_send, &msg, sizeof(msg));
                        rt_thread_mdelay(50);
                    }
                }
            }
        }

        if (uds_mode == 0x30)
        {
            msg.data[0] = 0x03;
            msg.data[1] = 0x30;
            msg.data[2] = uds_key;
            msg.data[3] = 0x01;
            rt_mq_send(mq_can_send, &msg, sizeof(msg));
            rt_thread_mdelay(50);
        }

        if (service == 0x06)
        {
            for (uint8_t i = 0; i < 6; i++)
            {
                msg.data[0] = 0x02;
                msg.data[1] = 0x06;
                msg.data[2] = mode6_sid[i];
                msg.data[3] = 0x00;
                rt_mq_send(mq_can_send, &msg, sizeof(msg));
                rt_thread_mdelay(50);
            }
        }

        if (uds_mode == 0x21)
        {
            msg.id = 0x796;
            msg.data[0] = 0x02;
            msg.data[1] = uds_mode;
            msg.data[2] = 0x33;
            msg.data[3] = 0x00;
            rt_mq_send(mq_can_send, &msg, sizeof(msg));
            rt_thread_mdelay(50);
        }
    }
}

void obd_handle_request(int mode, int value, int key)
{
    struct rt_can_msg can_trans;
    can_trans.ide = 0;
    can_trans.id = 0x7e0;
    can_trans.rtr = 0;
    can_trans.len = 8;
    can_1_set_filter(0x7e8, CAN_FILTERMODE_IDMASK);

    switch(mode)
    {
    case 0x00:
        service_1_value = 0;
        service = 0;
        uds_mode = 0;
        uds_key = 0;
        can_1_set_filter(0x7FF, CAN_FILTERMODE_IDLIST);
        break;
    case 0x01:
        service = 0x01;
        service_1_value = value;
        break;
    case 0x03:
    case 0x04:
    case 0x07:
        can_trans.data[0] = 0x01;
        can_trans.data[1] = mode;
        rt_mq_send(mq_can_send, &can_trans, sizeof(can_trans));
        break;
    case 0x06:
        service = 0x06;
        break;
    case 0x09:
        can_trans.data[0] = 0x02;
        can_trans.data[1] = 0x09;
        can_trans.data[2] = 0x02;
        rt_mq_send(mq_can_send, &can_trans, sizeof(can_trans));
        break;
    case 0x30:
        uds_mode = 0x30;
        uds_key = value;
        can_trans.data[0] = 0x04;
        can_trans.data[1] = mode;
        can_trans.data[2] = value;
        can_trans.data[3] = 0x07;
        can_trans.data[4] = key;
        if (value == 0x10)
        {
            injector_num = key;
        }
        rt_mq_send(mq_can_send, &can_trans, sizeof(can_trans));
        break;
    case 0x21:
        can_1_set_filter(0x797, CAN_FILTERMODE_IDLIST);
        uds_mode = 0x21;
        break;
    }
}

static int dtc_decode(uint8_t byte_a, uint8_t byte_b, char *result)
{
    char header;

    memset(result, 0, strlen(result));

    switch ((byte_a >> 6)&0x3) {
        case 0:
            header = 'P';
            break;
        case 1:
            header = 'C';
            break;
        case 2:
            header = 'B';
            break;
        default:
            header = 'U';
            break;
    }

    sprintf(result, "%c%d%d%d%d", header, (byte_a >> 4)&0x03, byte_a&0x0F, (byte_b >> 4)&0x0F, byte_b&0x0F);

    return strlen(result);
}

static void can_single_frame_handle_thread(void * params)
{
    char result[40];
    char dtc[10];
    struct rt_can_msg msg;
    while(1)
    {
        if(rt_mq_recv(mq_can_read_single_frame, &msg, sizeof(msg), RT_WAITING_FOREVER) == RT_EOK)
        {
            switch (msg.data[1]) {
                case 0x41:
                    switch (msg.data[2]) {
                        case 0x04:
                            sprintf(result, "{\"%s/41/CEL\":%.2f}", OBD_ID, msg.data[3]/(2.55));
                            break;
                        case 0x05:
                            sprintf(result, "{\"%s/41/ET\":%d}", OBD_ID, msg.data[3]-40);
                            break;
                        case 0x06:
                            sprintf(result, "{\"%s/41/STFT1\":%.2f}", OBD_ID, msg.data[3]/(1.28) - 100);
                            break;
                        case 0x07:
                            sprintf(result, "{\"%s/41/LTFT1\":%.2f}", OBD_ID, msg.data[3]/(1.28) - 100);
                            break;
                        case 0x0B:
                            sprintf(result, "{\"%s/41/IMAP\":%d}", OBD_ID, msg.data[3]);
                            break;
                        case 0x0C:
                            sprintf(result, "{\"%s/41/ES\":%d}", OBD_ID, ((msg.data[3]<<8)|msg.data[4])/4);
                            break;
                        case 0x0D:
                            sprintf(result, "{\"%s/41/VS\":%d}", OBD_ID, msg.data[3]);
                            break;
                        case 0x0E:
                            sprintf(result, "{\"%s/41/TA\":%d}", OBD_ID, msg.data[3]/2 - 64);
                            break;
                        case 0x0F:
                            sprintf(result, "{\"%s/41/IAT\":%d}", OBD_ID, msg.data[3] - 40);
                            break;
                        case 0x11:
                            sprintf(result, "{\"%s/41/TP\":%.2f}", OBD_ID, msg.data[3]/2.55);
                            break;
                        case 0x13:
                            sprintf(result, "{\"%s/41/OSP\":%d}", OBD_ID, msg.data[3]);
                            break;
                        case 0x14:
                            sprintf(result, "{\"%s/41/OSV1\":%.3f, \"%s/41/STFT1\":%.2f}", OBD_ID, (double)msg.data[3]/200, OBD_ID, (double)msg.data[4]/(1.28) - 100);
                            break;
                        case 0x15:
                            sprintf(result, "{\"%s/41/OSV2\":%.3f, \"%s/41/STFT2\":%.2f}", OBD_ID, (double)msg.data[3]/200, OBD_ID, (double)msg.data[4]/(1.28) - 100);
                            break;
                        case 0x1C:
                            sprintf(result, "{\"%s/41/OBDS\":%d}", OBD_ID, msg.data[3]);
                            break;
                        case 0x1F:
                            sprintf(result, "{\"%s/41/RTES\":%d}", OBD_ID, (msg.data[3] << 8)|msg.data[4]);
                            break;
                        case 0x21:
                            sprintf(result, "{\"%s/41/DT\":%d}", OBD_ID, (msg.data[3] << 8)|msg.data[4]);
                            break;
                        case 0x2E:
                            sprintf(result, "{\"%s/41/CEP\":%.2f}", OBD_ID, (double)msg.data[3]/2.55);
                            break;
                        case 0x30:
                            sprintf(result, "{\"%s/41/WSCC\":%d}", OBD_ID, msg.data[3]);
                            break;
                        case 0x31:
                            sprintf(result, "{\"%s/41/DTCC\":%d}", OBD_ID, (msg.data[3] << 8)|msg.data[4]);
                            break;
                        case 0x33:
                            sprintf(result, "{\"%s/41/ABP\":%d}", OBD_ID, msg.data[3]);
                            break;
                        case 0x42:
                            sprintf(result, "{\"%s/41/CMV\":%.2f}", OBD_ID, (double)((msg.data[3] << 8)|msg.data[4])/1000);
                            break;
                        case 0x43:
                            sprintf(result, "{\"%s/41/ALV\":%.2f}", OBD_ID, ((msg.data[3] << 8)|msg.data[4])/2.55);
                            break;
                        case 0x44:
                            sprintf(result, "{\"%s/41/CAFER\":%.2f}", OBD_ID, (double)2*((msg.data[3] << 8)|msg.data[4])/65536);
                            break;
                        case 0x45:
                            sprintf(result, "{\"%s/41/RTP\":%.2f}", OBD_ID, (msg.data[3])/2.55);
                            break;
                        case 0x46:
                            sprintf(result, "{\"%s/41/AAT\":%d}", OBD_ID, msg.data[3] - 40);
                            break;
                        case 0x47:
                            sprintf(result, "{\"%s/41/ATPB\":%.2f}", OBD_ID, (msg.data[3])/2.55);
                            break;
                        case 0x49:
                            sprintf(result, "{\"%s/41/APPD\":%.2f}", OBD_ID, (msg.data[3])/2.55);
                            break;
                        case 0x4A:
                            sprintf(result, "{\"%s/41/APPE\":%.2f}", OBD_ID, (msg.data[3])/2.55);
                            break;
                        case 0x4C:
                            sprintf(result, "{\"%s/41/CTA\":%.2f}", OBD_ID, (msg.data[3])/2.55);
                            break;
                        case 0x51:
                            sprintf(result, "{\"%s/41/FT\":%d}", OBD_ID, (msg.data[3]));
                            break;
                        case 0x5A:
                            sprintf(result, "{\"%s/41/RAPP\":%.2f}", OBD_ID, (msg.data[3])/2.55);
                            break;
                        default:
                            sprintf(result, "{\"%s/41/TP\":%.2f}", OBD_ID, msg.data[3]/2.55);
                            break;
                    }
                    mqtt_user_publish(result);
                    break;
                case 0x70:
                    switch (msg.data[2]) {
                        case 0x10:
                            switch (msg.data[4]) {
                                case 0:
                                    sprintf(result, "{\"%s/70/IJ%d\":%d}", OBD_ID, injector_num, msg.data[4]);
                                    uds_mode = 0;
                                    break;
                                case 0xFF:
                                    sprintf(result, "{\"%s/70/IJ%d\":%d}", OBD_ID, injector_num, msg.data[4]);
                                    break;
                                default:
                                    injector_num = msg.data[4];
                                    break;
                            }
                            break;
                        default:
                            sprintf(result, "{\"%s/70/ACT/%x\":%d}", OBD_ID, msg.data[2], msg.data[4]);
                            if (msg.data[4] == 0)
                            {
                                uds_mode = 0;
                            }
                            break;
                    }
                    mqtt_user_publish(result);
                    break;
                case 0x43:
                case 0x47:
                    sprintf(result, "{\"%s/%x/DTC/num\":%d}", OBD_ID, msg.data[1], msg.data[2]);
                    mqtt_user_publish(result);
                    if (msg.data[2] >= 1)
                    {
                        dtc_decode(msg.data[3], msg.data[4], dtc);
                        sprintf(result, "{\"%s/%x/DTC/1\":\"%s\"}", OBD_ID, msg.data[1], dtc);
                        mqtt_user_publish(result);
                    }
                    if (msg.data[2] >= 2)
                    {
                        dtc_decode(msg.data[5], msg.data[6], dtc);
                        sprintf(result, "{\"%s/%x/DTC/2\":\"%s\"}", OBD_ID, msg.data[1], dtc);
                        mqtt_user_publish(result);
                    }
                    break;
                default:
                    sprintf(result, "no");
                    break;
            }
        }else{
            rt_kprintf("Error!\n");
        }
    }
}

static void can_multi_frame_handle_thread(void * params)
{
    struct rt_can_msg msg;
    int num = 0;
    uint8_t mode = 0;

    uint8_t msg_trans[112] = {0};

    while(1)
    {
        rt_mq_recv(mq_can_read_multi_frame, &msg, sizeof(msg), RT_WAITING_FOREVER);
        if (msg.data[0] == 0x10)
        {
            mode = msg.data[2];
            num = msg.data[1];
        }

        for (uint8_t i = 0; i < 7; i++)
        {
            msg_trans[((msg.data[0])&0x0F)*7+i] = msg.data[i+1];
        }

        num -= 8;

        if ((num <= 0)&&(mode > 0))
        {
            rt_mq_send(mq_handle_multi_frame, msg_trans, 112);
        }
    }
}



static void thread_decode_multi_frame(void * params)
{
    uint8_t msg[112] = {0};
    char result[20];
    char mqtt_msg[50];
    uint8_t count = 0;

    while(1)
    {
        rt_mq_recv(mq_handle_multi_frame, msg, 112, RT_WAITING_FOREVER);
        switch (msg[1]) {
            case 0x43:
            case 0x47:
                sprintf(mqtt_msg, "{\"%s/%x/DTC/num\":%d}", OBD_ID, msg[1], msg[2]);
                mqtt_user_publish(mqtt_msg);
                while(count < msg[2])
                {
                    dtc_decode(msg[3+count*2], msg[4+2*count], result);
                    sprintf(mqtt_msg, "{\"%s/%x/DTC/%d\":\"%s\"}", OBD_ID, msg[1], count+1, result);
                    mqtt_user_publish(mqtt_msg);
                    count++;
                }
                count = 0;
                break;
            case 0x49:
                sprintf(result, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
                        msg[4],
                        msg[5],
                        msg[6],
                        msg[7],
                        msg[8],
                        msg[9],
                        msg[10],
                        msg[11],
                        msg[12],
                        msg[13],
                        msg[14],
                        msg[15],
                        msg[16],
                        msg[17],
                        msg[18],
                        msg[19],
                        msg[20]
                );
                sprintf(mqtt_msg, "{\"%s/%x/VIN\":\"%s\"}", OBD_ID, msg[1], result);
                mqtt_user_publish(mqtt_msg);
                break;
            case 0x46:
                switch (msg[2]) {
                    case 0x01:
                        sprintf(mqtt_msg, "{\"%s/46/OSBS/min\":\"%d\"}", OBD_ID, ((msg[5]<<8)|msg[6]));
                        mqtt_user_publish(mqtt_msg);
                        sprintf(mqtt_msg, "{\"%s/46/OSBS/test_value\":\"%d\"}", OBD_ID, ((msg[7]<<8)|msg[8]));
                        mqtt_user_publish(mqtt_msg);
                        sprintf(mqtt_msg, "{\"%s/46/OSBS/max\":\"%d\"}", OBD_ID, ((msg[9]<<8)|msg[10]));
                        mqtt_user_publish(mqtt_msg);
                        break;
                    case 0x21:
                        sprintf(mqtt_msg, "{\"%s/46/CMB/min\":\"%d\"}", OBD_ID, ((msg[5]<<8)|msg[6]));
                        mqtt_user_publish(mqtt_msg);
                        sprintf(mqtt_msg, "{\"%s/46/CMB/test_value\":\"%d\"}", OBD_ID, ((msg[7]<<8)|msg[8]));
                        mqtt_user_publish(mqtt_msg);
                        sprintf(mqtt_msg, "{\"%s/46/CMB/max\":\"%d\"}", OBD_ID, ((msg[9]<<8)|msg[10]));
                        mqtt_user_publish(mqtt_msg);
                        break;
                    case 0xa2:
                    case 0xa3:
                    case 0xa4:
                    case 0xa5:
                        sprintf(mqtt_msg, "{\"%s/46/MCD%d/0B/min\":\"%d\"}", OBD_ID, msg[2] - 0xa1, ((msg[5]<<8)|msg[6]));
                        mqtt_user_publish(mqtt_msg);
                        sprintf(mqtt_msg, "{\"%s/46/MCD%d/0B/test_value\":\"%d\"}", OBD_ID, msg[2] - 0xa1, ((msg[7]<<8)|msg[8]));
                        mqtt_user_publish(mqtt_msg);
                        sprintf(mqtt_msg, "{\"%s/46/MCD%d/0B/max\":\"%d\"}", OBD_ID, msg[2] - 0xa1, ((msg[9]<<8)|msg[10]));
                        mqtt_user_publish(mqtt_msg);
                        sprintf(mqtt_msg, "{\"%s/46/MCD%d/0C/min\":\"%d\"}", OBD_ID, msg[2] - 0xa1, ((msg[14]<<8)|msg[15]));
                        mqtt_user_publish(mqtt_msg);
                        sprintf(mqtt_msg, "{\"%s/46/MCD%d/0C/test_value\":\"%d\"}", OBD_ID, msg[2] - 0xa1, ((msg[16]<<8)|msg[17]));
                        mqtt_user_publish(mqtt_msg);
                        sprintf(mqtt_msg, "{\"%s/46/MCD%d/0C/max\":\"%d\"}", OBD_ID, msg[2] - 0xa1, ((msg[18]<<8)|msg[19]));
                        mqtt_user_publish(mqtt_msg);
                        break;
                    default:
                        break;
                }
                break;
            case 0x61:
                sprintf(mqtt_msg, "{\"%s/61/STA\":\"%d\"}", OBD_ID, ((msg[5]<<8)|msg[6])/2 - 2048);
                mqtt_user_publish(mqtt_msg);
                break;
            default:
                break;
        }
    }
}


static int obd_control_init(void)
{
    while(!mqtt_get_status());
    mq_handle_multi_frame = rt_mq_create("mq_handle_multi_frame", 112, 5, RT_IPC_FLAG_FIFO);
    thread_single_handle_id = rt_thread_create("can single response", can_single_frame_handle_thread, NULL, 1024, 25, 5);

    if (thread_single_handle_id != RT_NULL)
    {
        rt_thread_startup(thread_single_handle_id);
    }else{
        rt_kprintf("Failed mqtt thread init: %d\n", thread_single_handle_id->error);
        return -RT_ERROR;
    }

    thread_multi_handle_id = rt_thread_create("can multi response", can_multi_frame_handle_thread, NULL, 1024, 25, 5);

    if (thread_multi_handle_id != RT_NULL)
    {
        rt_thread_startup(thread_multi_handle_id);
    }else{
        rt_kprintf("Failed mqtt thread init: %d\n", thread_multi_handle_id->error);
        return -RT_ERROR;
    }

    thread_handle_multi_msg_id = rt_thread_create("multi decode", thread_decode_multi_frame, NULL, 3500, 25, 5);

    if (thread_multi_handle_id != RT_NULL)
    {
        rt_thread_startup(thread_handle_multi_msg_id);
    }else{
        rt_kprintf("Failed mqtt thread init: %d\n", thread_handle_multi_msg_id->error);
        return -RT_ERROR;
    }

    thread_reques_handle = rt_thread_create("handle_request", can_request_handle_thread, NULL, 1024, 25, 5);

    if (thread_reques_handle != RT_NULL)
    {
        rt_thread_startup(thread_reques_handle);
    }else{
        rt_kprintf("Failed mqtt thread init: %d\n", thread_reques_handle->error);
        return -RT_ERROR;
    }
    return RT_EOK;
}

static void obd_confirm_ecu_thread(void * params)
{
    struct rt_can_msg msg;
    msg.id = 0x7df;
    msg.ide = 0;
    msg.rtr = 0;
    msg.len = 8;
    msg.data[0] = 0x02;
    msg.data[1] = 0x3E;
    msg.data[2] = 0x02;
    msg.data[3] = 0;
    msg.data[4] = 0;
    msg.data[5] = 0;
    msg.data[6] = 0;
    msg.data[7] = 0;

    while(1)
    {
        rt_device_write(can, 0, &msg, sizeof(msg));
        rt_thread_mdelay(100);
    }
}

int obd_init(void)
{
    rt_thread_t thread_con_ecu_id = RT_NULL;

    thread_con_ecu_id = rt_thread_create("thread-confirm ecu", obd_confirm_ecu_thread, NULL, 512, 25, 5);

    obd_control_init();

    if (thread_con_ecu_id != RT_NULL)
    {
        rt_thread_startup(thread_con_ecu_id);
    }else{
        rt_kprintf("Failed mqtt thread init: %d\n", thread_con_ecu_id->error);
        return -RT_ERROR;
    }

    return RT_EOK;
}

#ifndef PKG_USING_CMUX
//INIT_APP_EXPORT(obd_control_init);
#endif



