/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-10-10     Admin       the first version
 */

#include <stdio.h>
#include <stdlib.h>

#include <connection_manager.h>

connectState machineState = offline;

int connectCode = 0;

int connectHandle(int code)
{
    if ((machineState == offline)||(machineState == connected))
    {
        return -1;
    }
    machineState = connected;
    connectCode = code;
    return machineState;
}


int disconnectHandle(void)
{
    if ((machineState == offline)||(machineState == empty))
    {
        return -1;
    }

    machineState = empty;
    connectCode = 0;
    return -1;
}

int getConnectCode(void)
{
    return connectCode;
}

void setConnectCode(int code)
{
    connectCode = code;
}

void setConnectState(connectState state)
{
    machineState = state;
}

connectState getConnectState(void)
{
    return machineState;
}


