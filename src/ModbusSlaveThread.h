/*!
 *
 * Project: piModbusSlave
 * (C)    : KUNBUS GmbH, Heerweg 15C, 73370 Denkendorf, Germany
 *
 */

#ifndef MODBUS_SLAVE_THREAD_H_
#define MODBUS_SLAVE_THREAD_H_

#include <modbusconfig.h>

void *startTcpSlaveThread(void *arg);
void *startRtuSlaveThread(void *arg);
int32_t process_modbus_request(modbus_t *mb_slave_p, modbus_mapping_t* mbMapping_p, TModbusSlaveConfiguration *psModbusConfiguration_p);

#endif /* MODBUS_SLAVE_THREAD_H_ */