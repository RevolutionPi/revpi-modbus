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
char* read_config_file(void);
void free_config_buffer(void);
int32_t get_json_devices_array(const char* pc8_pi_config_data_p, struct array_list **pp_devices_array_p);
int32_t parse_modbus_slaves_config_data(const char* pc8_pi_config_data_p, struct TMBSlaveConfHead *p_mbSlaveConfHead_p);
int32_t parse_modbus_master_config_data(const char* pc8_pi_config_data_p, struct TMBMasterConfHead *p_mbMasterConfHead_p);
void free_modbus_master_config_data(struct TMBMasterConfHead *p_mbMasterConfHead_p);
int32_t parse_device_modbus_configuration(json_object *json_device_parameter_object_p, TModbusDeviceConfiguration* modbusDeviceConfig_p);
int32_t parse_modbus_master_action_list(json_object *json_pi_device_p, struct TMBActionListHead *tModbusActionListHead_p);
int32_t parse_modbus_slave_device_process_image_config(json_object *pi_device_p, TModbusSlaveConfiguration* modbusSlaveConfiguration_p);
int32_t get_device_product_type(json_object *pi_device, const char **ppc8_productType);
int32_t get_variable_parameters(json_object *json_pi_device_p,
	const char* json_parameter_name,
	uint32_t *byte_offset_p,
	uint32_t *bit_offset_p);
int32_t get_variable_relative_offsets(json_object *json_in_out_section,
	const char* json_parameter_name,
	uint32_t *byte_offset_p,
	uint32_t *bit_offset_p);
int32_t get_process_image_device_offset(json_object *json_pi_device_p);

int32_t get_device_process_image_parameter(json_object *json_process_image_object_p, uint32_t* u32_absolute_process_image_offset_p, uint32_t* u32_process_image_length_p);

int32_t get_device_modbus_configuration(json_object *json_device_parameter_object_p, TModbusDeviceConfiguration* modbusDeviceConfig_p);
const char* get_device_string_parameter(json_object *json_device_config_parameters_p, const char* json_key_p);
const char* get_modbus_action_matching_name_string_value(
	struct json_object *json_modbus_actions_p,
	const char* action_parameter_prefix_p,
	const char* action_identifier_p);

#endif /*PI_CONFIG_PARSER_H_*/
