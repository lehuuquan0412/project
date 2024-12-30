/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-12-16     Admin       the first version
 */
#ifndef APPLICATIONS_INC_OBD2_H_
#define APPLICATIONS_INC_OBD2_H_

#define OBD_ID          "2000"

void obd_handle_request(int mode, int value, int key);
int obd_init(void);

#endif /* APPLICATIONS_INC_OBD2_H_ */
