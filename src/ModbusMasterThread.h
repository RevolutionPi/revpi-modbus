/* SPDX-License-Identifier: GPL-2.0-or-later */

/*!
 *
 * Project: piModbusMaster
 * (C)    : KUNBUS GmbH, Heerweg 15C, 73370 Denkendorf, Germany
 *
 */

#ifndef MODBUS_MASTER_THREAD_H_
#define MODBUS_MASTER_THREAD_H_

#include "modbusconfig.h"

void *startTcpMasterThread(void *arg);
void *startRtuMasterThread(void *arg);

#endif /* MODBUS_MASTER_THREAD_H_ */
