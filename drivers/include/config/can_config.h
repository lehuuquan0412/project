/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-08-20     Admin       the first version
 */
#ifndef __CAN_CONFIG_H__
#define __CAN_CONFIG_H__

#include <rtthread.h>
#include <board.h>


#ifdef __cplusplus
extern "C" {
#endif

#if defined(BSP_USING_CAN1)
#ifndef CAN1_CONFIG
#define CAN1_CONFIG                                                \
    {                                                               \
        .name = "can1",                                              \
        .Instance = CAN1,                                         \
        .tx_pin_name = BSP_CAN1_TX_PIN,                            \
        .rx_pin_name = BSP_CAN1_RX_PIN,                            \
    }
#endif /* CAN1_CONFIG */
#endif

#if defined(BSP_USING_CAN2)
#ifndef CAN1_CONFIG
#define CAN1_CONFIG                                                \
    {                                                               \
        .name = "can2",
        .Instance = CAN2,                                         \                                    \
        .tx_pin_name = BSP_CAN2_TX_PIN,                            \
        .rx_pin_name = BSP_CAN2_RX_PIN,                            \
    }
#endif /* CAN1_CONFIG */


#endif

#ifdef __cplusplus
}
#endif

#endif /* __CAN_CONFIG_H_ */
