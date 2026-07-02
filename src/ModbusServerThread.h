/*
 * SPDX-FileCopyrightText: 2023-2026 KUNBUS GmbH
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/*!
 *
 * Project: piModbusServer
 * (C)    : KUNBUS GmbH, Magirusstraße 7, 73760 Ostfildern, Germany
 *
 */

#ifndef MODBUS_SERVER_THREAD_H_
#define MODBUS_SERVER_THREAD_H_

#include "modbusconfig.h"

void *startTcpServerThread(void *arg);
void *startRtuServerThread(void *arg);
int32_t process_modbus_request(modbus_t *mb_server_p, modbus_mapping_t* mbMapping_p, TModbusServerConfiguration *psModbusConfiguration_p);

#endif /* MODBUS_SERVER_THREAD_H_ */
