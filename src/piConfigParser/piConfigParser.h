/*
 * SPDX-FileCopyrightText: 2024-2026 KUNBUS GmbH
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/*!
 *
 * Project: piModbusServer
 * (C)    : KUNBUS GmbH, Magirusstraße 7, 73760 Ostfildern, Germany
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


#define PICONFIG_FILE					"/etc/revpi/config.rsc"

//TODO: use defines from common_define.h
#define MODBUS_SERVER_TCP_PI_PRODUCT_TYPE    "24577"
#define MODBUS_SERVER_RTU_PI_PRODUCT_TYPE    "24578"
#define MODBUS_CLIENT_TCP_PI_PRODUCT_TYPE   "24579"
#define MODBUS_CLIENT_RTU_PI_PRODUCT_TYPE   "24580"

#define MODBUS_SERVER_PARAMETER_ARRAY_POSITION 1

#define MODBUS_CLIENT_TCP_JSON_KEY_IP_ADDRESS           "0"
#define MODBUS_CLIENT_TCP_JSON_KEY_TCP_PORT             "1"
#define MODBUS_CLIENT_TCP_JSON_KEY_TCP_MAX_CONNECTIONS  "2"

#define MODBUS_SERVER_TCP_JSON_KEY_TCP_PORT              "0"
#define MODBUS_SERVER_TCP_JSON_KEY_TCP_MAX_CONNECTIONS   "1"

#define MODBUS_SERVER_RTU_JSON_KEY_DEVICE_PATH "0"
#define MODBUS_SERVER_RTU_JSON_KEY_BAUDRATE "1"
#define MODBUS_SERVER_RTU_JSON_KEY_PARITY "2"
#define MODBUS_SERVER_RTU_JSON_KEY_DATABITS "3"
#define MODBUS_SERVER_RTU_JSON_KEY_STOPBITS "4"
#define MODBUS_SERVER_RTU_MODBUS_ADDRESS	"5"
#define MODBUS_SERVER_RTU_JSON_PARITY_VALUE_NONE '0'
#define MODBUS_SERVER_RTU_JSON_PARITY_VALUE_EVEN '1'
#define MODBUS_SERVER_RTU_JSON_PARITY_VALUE_ODD '2'


void get_server_device_config_list(struct TMBServerConfHead *p_mbServerConfHead_p);
void get_client_device_config_list(struct TMBClientConfHead *p_mbClientConfHead_p);
void free_config_buffer(void);
void free_modbus_client_config_data(struct TMBClientConfHead *p_mbClientConfHead_p);

#endif /*PI_CONFIG_PARSER_H_*/
