/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-09-24     Admin       the first version
 */
#ifndef APPLICATIONS_INC_MQTT_H_
#define APPLICATIONS_INC_MQTT_H_

#include <stdlib.h>

#include "paho_mqtt.h"

int mqtt_connect_init(void);

int mqtt_get_status(void);

int mqtt_user_publish(char *msg);

int mqtt_system_init(void);

#endif /* APPLICATIONS_INC_MQTT_H_ */
