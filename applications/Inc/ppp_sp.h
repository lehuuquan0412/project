/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-10-08     Admin       the first version
 */
#ifndef APPLICATIONS_INC_PPP_SP_H_
#define APPLICATIONS_INC_PPP_SP_H_

rt_device_t ppp_device_get(void);
int ppp_get_status_connect(rt_device_t ppp);

#endif /* APPLICATIONS_INC_PPP_SP_H_ */
