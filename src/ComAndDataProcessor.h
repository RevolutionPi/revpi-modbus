/*!
 *
 * Project: piModbusSlave
 * (C)    : KUNBUS GmbH, Heerweg 15C, 73370 Denkendorf, Germany
 *
 */

#ifndef MODBUS_PROCESSOR_H_
#define MODBUS_PROCESSOR_H_

#include "Scheduler.h"
#include <modbus/modbus.h>


int32_t processModbusAction(modbus_t *pModbusContext, tModbusEvent* nextEvent, uint8_t* buffer);
int32_t writeErrorMessage(uint32_t status_byte_pi_offset_p, uint8_t modbus_error_code_p);
int32_t reset_modbus_action_status(uint32_t status_reset_byte_offset_p,
								   uint8_t status_reset_bit_offset_p,
								   uint32_t status_byte_pi_offset_p);
int32_t reset_modbus_master_status(uint32_t status_reset_byte_offset_p,
								   uint32_t status_byte_pi_offset_p);

#endif /*MODBUS_PROCESSOR_H_*/
