/*
 * SPDX-FileCopyrightText: 2023-2026 KUNBUS GmbH
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/*!
 *
 * Project: piModbusClient
 * (C)    : KUNBUS GmbH, Magirusstraße 7, 73760 Ostfildern, Germany
 *
 */

#ifndef MODBUS_CLIENT_THREAD_H_
#define MODBUS_CLIENT_THREAD_H_

#include "modbusconfig.h"

void *startTcpClientThread(void *arg);
void *startRtuClientThread(void *arg);

#endif /* MODBUS_CLIENT_THREAD_H_ */
