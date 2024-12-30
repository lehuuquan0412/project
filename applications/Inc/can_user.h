/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-10-08     Admin       the first version
 */
#ifndef APPLICATIONS_INC_CAN_USER_H_
#define APPLICATIONS_INC_CAN_USER_H_

extern rt_mq_t mq_can_send;
extern rt_mq_t mq_can_read_single_frame;
extern rt_mq_t mq_can_read_multi_frame;

extern rt_device_t can;

int can_1_set_filter(uint32_t filter_id, uint8_t filter_mode);
int can_system_init(void);

#endif /* APPLICATIONS_INC_CAN_USER_H_ */
