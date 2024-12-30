/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-10-08     Admin       the first version
 */

#include <rtthread.h>
#include <ppp_device.h>
#include <board.h>
#include <rtdevice.h>


rt_device_t ppp_device_get(void)
{
    return rt_device_find(PPP_DEVICE_NAME);
}

int ppp_get_status_connect(rt_device_t ppp)
{
    return ((struct ppp_device*)ppp)->state;
}

#ifndef PKG_USING_CMUX
//INIT_APP_EXPORT(ppp_device_get);
#endif




