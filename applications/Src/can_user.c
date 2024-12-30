/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-09-24     Admin       the first version
 */

#include <stdlib.h>
#include <string.h>

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#include <drivers/can.h>
#include <drivers/pin.h>

#include "can_user.h"

#define DBG_ENABLE
#define DBG_SECTION_NAME    "can.user"
#define DBG_LEVEL           DBG_LOG
#define DBG_COLOR
#include <rtdbg.h>


#define CAN_NAME                "can1"
#define CAN_TRANS_SPEED         50

rt_device_t can;
rt_base_t led_can_rx;
rt_sem_t sem_can_rx = RT_NULL;

rt_thread_t thread_can_transmit_id = RT_NULL;
rt_thread_t thread_can_recieve_id = RT_NULL;

rt_mq_t mq_can_send = RT_NULL;
rt_mq_t mq_can_read_single_frame = RT_NULL;
rt_mq_t mq_can_read_multi_frame = RT_NULL;

uint8_t led_can_rx_status = 0;

static rt_err_t can_input(rt_device_t dev, rt_size_t size)
{
    rt_sem_release(sem_can_rx);
    rt_pin_write(led_can_rx, led_can_rx_status);
    led_can_rx_status = !led_can_rx_status;
    return RT_EOK;
}


static int can_init(void)
{
    struct can_configure config = CANDEFAULTCONFIG;
    config.baud_rate = CAN500kBaud;

    struct rt_can_filter_item filter[] = {
            RT_CAN_FILTER_ITEM_INIT(0x7FF, 0, 0, CAN_FILTERMODE_IDMASK, 0x000007FF, NULL, NULL),
    };

    filter[0].hdr_bank = 0;

    struct rt_can_filter_config filter_config;
    filter_config.items = filter;
    filter_config.count = 1;
    filter_config.actived = ENABLE;
    can = rt_device_find(CAN_NAME);
    rt_device_open(can, RT_DEVICE_FLAG_INT_TX|RT_DEVICE_FLAG_INT_RX);

    rt_device_control(can, RT_CAN_CMD_SET_FILTER, &filter_config);
    LOG_D("Set filter id: 0x7e0");
    rt_device_control(can, RT_DEVICE_CTRL_CONFIG, &config);
    LOG_D("Set speed: 500000 bit/s");
    rt_device_set_rx_indicate(can, can_input);
    LOG_D("CAN enable successfull!");
    return RT_EOK;
}

int can_1_set_filter(uint32_t filter_id, uint8_t filter_mode)
{
    struct rt_can_filter_item filter[] = {
            RT_CAN_FILTER_ITEM_INIT(filter_id, 0, 0, filter_mode, filter_id, NULL, NULL),
    };

    filter[0].hdr_bank = 0;

    struct rt_can_filter_config filter_config;
    filter_config.items = filter;
    filter_config.count = 1;
    filter_config.actived = ENABLE;

    rt_device_control(can, RT_CAN_CMD_SET_FILTER, &filter_config);
    return RT_EOK;
}

static int led_rx_init(void)
{
    led_can_rx = GET_PIN(A, 6);
    rt_pin_mode(led_can_rx, PIN_MODE_OUTPUT);
    rt_pin_write(led_can_rx, led_can_rx_status);
    return RT_EOK;
}

static void can_transmit_thread(void * params)
{
    struct rt_can_msg can_trans_msg;

    while(1)
    {
        if(rt_mq_recv(mq_can_send, &can_trans_msg, sizeof(can_trans_msg), RT_WAITING_FOREVER) == RT_EOK)
        {
            rt_device_write(can, 0, &can_trans_msg, sizeof(can_trans_msg));
            //rt_kprintf("Send: %x\n", can_trans_msg.data[2]);
            rt_thread_mdelay(CAN_TRANS_SPEED);
        }

    }
}

static void can_recieve_thread(void * params)
{
    struct rt_can_msg can_rcv_msg, msg;
    msg.id = 0x7e0;
    msg.ide = CAN_ID_STD;
    msg.rtr = CAN_RTR_DATA;
    msg.len = 8;
    msg.data[0] = 0x30;
    msg.data[1] = 0;
    msg.data[2] = 0;
    msg.data[3] = 0;
    msg.data[4] = 0;
    msg.data[5] = 0;
    msg.data[6] = 0;
    msg.data[7] = 0;

    while(1)
    {
        rt_sem_take(sem_can_rx, RT_WAITING_FOREVER);
        rt_device_read(can, 0, &can_rcv_msg, sizeof(can_rcv_msg));
        switch (can_rcv_msg.data[0]) {
            case 0x10:
                switch (can_rcv_msg.id) {
                    case 0x7e8:
                        msg.id = 0x7e0;
                        break;
                    case 0x797:
                        msg.id = 0x796;
                        break;
                }
                rt_device_write(can, 0, &msg, sizeof(msg));
                rt_mq_send(mq_can_read_multi_frame, &can_rcv_msg, sizeof(can_rcv_msg));
                break;
            case 0x21:
            case 0x22:
            case 0x23:
            case 0x24:
            case 0x25:
            case 0x26:
            case 0x27:
            case 0x28:
            case 0x29:
            case 0x2A:
            case 0x2B:
            case 0x2C:
            case 0x2D:
            case 0x2E:
            case 0x2F:
                rt_mq_send(mq_can_read_multi_frame, &can_rcv_msg, sizeof(can_rcv_msg));
                break;
            default:
                rt_mq_send(mq_can_read_single_frame, &can_rcv_msg, sizeof(can_rcv_msg));
                break;
        }
    }
}

static int can_transmit_receive_controller_init(void)
{
    sem_can_rx = rt_sem_create("sem rx", 0, RT_IPC_FLAG_FIFO);

    mq_can_send = rt_mq_create("mq send message", sizeof(struct rt_can_msg), 10, RT_IPC_FLAG_FIFO);
    mq_can_read_single_frame = rt_mq_create("mq read message single frame ", sizeof(struct rt_can_msg), 10, RT_IPC_FLAG_FIFO);
    mq_can_read_multi_frame = rt_mq_create("mq read message multi frame ", sizeof(struct rt_can_msg), 17, RT_IPC_FLAG_FIFO);

    thread_can_transmit_id = rt_thread_create("Can send", can_transmit_thread, NULL, 1024, 25, 5);

    if (thread_can_transmit_id != RT_NULL)
    {
        rt_thread_startup(thread_can_transmit_id);
    }else{
        rt_kprintf("Failed mqtt thread init: %d\n", thread_can_transmit_id->error);
        return -RT_ERROR;
    }

    thread_can_recieve_id = rt_thread_create("Can receive", can_recieve_thread, NULL, 1024, 25, 5);

    if (thread_can_transmit_id != RT_NULL)
    {
        rt_thread_startup(thread_can_recieve_id);
    }else{
        rt_kprintf("Failed mqtt thread init: %d\n", thread_can_transmit_id->error);
        return -RT_ERROR;
    }

    return RT_EOK;
}

int can_system_init(void)
{
    led_rx_init();
    can_init();
    can_transmit_receive_controller_init();
    return 0;
}

#ifndef PKG_USING_CMUX
//INIT_APP_EXPORT(led_rx_init);
//INIT_APP_EXPORT(can_init);
//INIT_APP_EXPORT(can_transmit_receive_controller_init);
#endif
