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
#ifndef PI_PROCESS_IMAGE_ACCESS_H_
#define PI_PROCESS_IMAGE_ACCESS_H_

#include "modbusconfig.h"

int32_t writeModbusDataToProcessImage(modbus_mapping_t* mbMapping, TProcessImageConfiguration* ptrSPiProcessImageOffsets);
int32_t readModbusDataFromProcessImage(modbus_mapping_t* mbMapping, TProcessImageConfiguration* ptrSPiProcessImageOffsets);


#endif /* PI_PROCESS_IMAGE_ACCESS_H_ */
