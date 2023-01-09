/*!
 *
 * Project: piModbusSlave
 * (C)    : KUNBUS GmbH, Heerweg 15C, 73370 Denkendorf, Germany
 *
 */

#ifndef PIMODBUSSLAVE_H_
#define PIMODBUSSLAVE_H_

#include <stdint.h>

typedef struct
{
	uint32_t i32uCoilsOffset;	// piProcessImage offset for configured Modbus coils
	uint32_t i32uDiscreteInputsOffset;	// piProcessImage offset for configured Modbus discrete inputs
	uint32_t i32uHoldingRegistersOffset;   // piProcessImage offset for configured Modbus holding registers
	uint32_t i32uInputRegistersOffset;	// piProcessImage offset for configured Modbus input registers
} SPiProcessImageOffsets;

#endif /* PIMODBUSSLAVE_H_ */
