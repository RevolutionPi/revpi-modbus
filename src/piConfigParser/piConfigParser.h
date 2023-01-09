/* SPDX-License-Identifier: GPL-2.0-or-later */

/*!
 *
 * Project: piModbusSlave
 * (C)    : KUNBUS GmbH, Heerweg 15C, 73370 Denkendorf, Germany
 *
 */

#ifndef PI_CONFIG_PARSER_H_
#define PI_CONFIG_PARSER_H_

#include <sys/types.h>
#include <stdint.h>
#include <json-c/json.h>
#include "../modbusconfig.h"
#include <sys/queue.h>

#define VARIABLE_NAME_ARRAY_POSITION 0
#define PROCESS_IMAGE_VARIABLE_BIT_LENGTH_ARRAY_POSITION 2
#define RELATIVE_PROCESS_IMAGE_VARIABLE_BYTE_OFFSET_ARRAY_POSITION 3
#define RELATIVE_PROCESS_IMAGE_VARIABLE_BIT_OFFSET_ARRAY_POSITION 7



//TODO: use defines from common_define.h
#define MODBUS_SLAVE_TCP_PI_PRODUCT_TYPE    "24577"
#define MODBUS_SLAVE_RTU_PI_PRODUCT_TYPE    "24578"
#define MODBUS_MASTER_TCP_PI_PRODUCT_TYPE   "24579"
#define MODBUS_MASTER_RTU_PI_PRODUCT_TYPE   "24580"

#define MODBUS_SLAVE_PARAMETER_ARRAY_POSITION 1

#define MODBUS_MASTER_TCP_JSON_KEY_IP_ADDRESS           "0"
#define MODBUS_MASTER_TCP_JSON_KEY_TCP_PORT             "1"
#define MODBUS_MASTER_TCP_JSON_KEY_TCP_MAX_CONNECTIONS  "2"

#define MODBUS_SLAVE_TCP_JSON_KEY_TCP_PORT              "0"
#define MODBUS_SLAVE_TCP_JSON_KEY_TCP_MAX_CONNECTIONS   "1"

#define MODBUS_SLAVE_RTU_JSON_KEY_DEVICE_PATH "0"
#define MODBUS_SLAVE_RTU_JSON_KEY_BAUDRATE "1"
#define MODBUS_SLAVE_RTU_JSON_KEY_PARITY "2"
#define MODBUS_SLAVE_RTU_JSON_KEY_DATABITS "3"
#define MODBUS_SLAVE_RTU_JSON_KEY_STOPBITS "4"
#define MODBUS_SLAVE_RTU_MODBUS_ADDRESS	"5"
#define MODBUS_SLAVE_RTU_JSON_PARITY_VALUE_NONE '0'
#define MODBUS_SLAVE_RTU_JSON_PARITY_VALUE_EVEN '1'
#define MODBUS_SLAVE_RTU_JSON_PARITY_VALUE_ODD '2'


void get_slave_device_config_list(struct TMBSlaveConfHead *p_mbSlaveConfHead_p);
void get_master_device_config_list(struct TMBMasterConfHead *p_mbMasterConfHead_p);
void free_config_buffer(void);
void free_modbus_master_config_data(struct TMBMasterConfHead *p_mbMasterConfHead_p);

#endif /*PI_CONFIG_PARSER_H_*/
