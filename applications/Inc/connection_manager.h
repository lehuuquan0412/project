/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-10-10     Admin       the first version
 */
#ifndef APPLICATIONS_INC_CONNECTION_MANAGER_H_
#define APPLICATIONS_INC_CONNECTION_MANAGER_H_


#include <stdio.h>
#include <stdlib.h>

typedef enum
{
    offline,
    empty,
    connected,
}connectState;

int connectHandle(int connectCode);
int disconnectHandle(void);

int getConnectCode(void);

void setConnectCode(int code);

void setConnectState(connectState state);

connectState getConnectState(void);

#endif /* APPLICATIONS_INC_CONNECTION_MANAGER_H_ */
