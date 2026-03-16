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

#ifndef PIMODBUSSERVER_H_
#define PIMODBUSSERVER_H_

#include <stdint.h>

typedef struct
{
	uint32_t i32uCoilsOffset;	// piProcessImage offset for configured Modbus coils
	uint32_t i32uDiscreteInputsOffset;	// piProcessImage offset for configured Modbus discrete inputs
	uint32_t i32uHoldingRegistersOffset;   // piProcessImage offset for configured Modbus holding registers
	uint32_t i32uInputRegistersOffset;	// piProcessImage offset for configured Modbus input registers
} SPiProcessImageOffsets;

#endif /* PIMODBUSSERVER_H_ */
