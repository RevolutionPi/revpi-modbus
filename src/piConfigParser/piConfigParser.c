/*!
 *
 * Project: piModbusSlave
 * (C)    : KUNBUS GmbH, Heerweg 15C, 73370 Denkendorf, Germany
 *
 */

#include "project.h"

#include "piConfigParser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <syslog.h>
#include <sys/types.h>
#include <regex.h>

#include <piControl.h>
#include <piTest/piControlIf.h>


typedef enum
{
    ACTION_ADDRESS_WRONG_FORMAT = -100,
    ACTION_DATA_SECTION_NOT_FOUND,
    ACTION_FUNCTION_CODE_WRONG_FORMAT,
    ACTION_ID_WRONG_FORMAT,
    ACTION_INTERVALL_WRONG_FORMAT,
    ACTION_REGISTER_ADDRESS_WRONG_FORMAT,
    ACTION_REGISTER_QUANTITY_WRONG_FORMAT,
    DEVICE_RESET_ENTRIES_NOT_FOUND,
    DEVICES_SECTION_EMPTY,
    DEVICES_SECTION_NOT_FOUND,
    EXTEND_SECTION_NOT_FOUND,
    GENERAL_EXCEPTION,
    INPUT_PARAMETER_SECTION_NOT_FOUND,
    INTERFACE_SECTION_NOT_FOUND,
    LENGTH_POSITION_PARAMETER_NOT_FOUND,
    LENGTH_POSITION_PARAMETER_WRONG_TYPE,
    MAX_CONNECTIONS_LIMIT_EXCEEDED,
    MAX_CONNECTIONS_NOT_FOUND,
    OFFSET_POSITION_PARAMETER_NOT_FOUND,
    OFFSET_POSITION_PARAMETER_WRONG_TYPE,
    OUTPUT_PARAMETER_SECTION_NOT_FOUND,
    PROCESS_IMAGE_OBJECT_EMPTY,
    PROCESS_IMAGE_WRONG_TYPE,
    PRODUCT_NAME_NOT_FOUND,
    PRODUCT_TYPE_SECTION_EMPTY,
    PRODUCT_TYPE_SECTION_NOT_FOUND,
    RTU_BAUDRATE_NOT_FOUND,
    RTU_BAUDRATE_WRONG_FORMAT,
    RTU_DATABITS_NOT_FOUND,
    RTU_DATABITS_SIZE,
    RTU_DEVICE_PATH_LENGTH_EXCEEDED,
    RTU_DEVICE_PATH_NOT_FOUND,
    RTU_MODBUS_ADDRESS_NOT_FOUND,
    RTU_MODBUS_ADDRESS_SIZE,
    RTU_PARITY_LENGTH_EXCEEDED,
    RTU_PARITY_NOT_FOUND,
    RTU_PARITY_WRONG_FORMAT,
    RTU_STOPBITS_NOT_FOUND,
    RTU_STOPBITS_SIZE,
    TCP_ADDRESS_NOT_FOUND,
    TCP_ADDRESS_WRONG_FORMAT,
    TCP_PORT_NOT_FOUND,
    TCP_PORT_WRONG_FORMAT,
    UNKNOWN_MODBUS_DEVICE,
    SUCCESS = 0
} parsing_error;


parsing_error get_json_devices_array(const char* pc8_pi_config_data_p, struct array_list **pp_devices_array_p);
parsing_error parse_modbus_slaves_config_data(const char* pc8_pi_config_data_p, struct TMBSlaveConfHead *p_mbSlaveConfHead_p);
parsing_error parse_modbus_master_config_data(const char* pc8_pi_config_data_p, struct TMBMasterConfHead *p_mbMasterConfHead_p);
parsing_error parse_device_modbus_configuration(json_object *json_device_parameter_object_p, TModbusDeviceConfiguration* modbusDeviceConfig_p);
parsing_error parse_modbus_master_action_list(json_object *json_pi_device_p, struct TMBActionListHead *tModbusActionListHead_p);
parsing_error parse_modbus_slave_device_process_image_config(json_object *pi_device_p, TModbusSlaveConfiguration* modbusSlaveConfiguration_p);
parsing_error get_device_product_type(json_object *pi_device, const char **ppc8_productType);
parsing_error get_variable_parameters(json_object *json_pi_device_p,
                                      const char* json_parameter_name,
                                      uint32_t *byte_offset_p,
                                      uint32_t *bit_offset_p);
parsing_error get_variable_relative_offsets(json_object *json_in_out_section,
                                            const char* json_parameter_name,
                                            uint32_t *byte_offset_p,
                                            uint32_t *bit_offset_p);
parsing_error get_process_image_device_offset(json_object *json_pi_device_p);
parsing_error get_device_process_image_parameter(json_object *json_process_image_object_p,
                                                 uint32_t* u32_absolute_process_image_offset_p,
                                                 uint32_t* u32_process_image_length_p);

char* read_config_file(void);
const char* get_device_string_parameter(json_object *json_device_config_parameters_p, const char* json_key_p);
const char* get_modbus_action_matching_name_string_value(
    struct json_object *json_modbus_actions_p,
    const char* action_parameter_prefix_p,
    const char* action_identifier_p);

const char MODBUS_MASTER_ACTION_ID_KEY[]                        = "ActionId";
const char MODBUS_MASTER_SLAVE_ADDRESS_KEY[]                    = "SlaveAddress";
const char MODBUS_MASTER_FUNCTON_CODE_KEY[]	                    = "FunctionCode";
const char MODBUS_MASTER_REGISTER_ADDRESS_KEY[]	                = "RegisterAddress";
const char MODBUS_MASTER_QUANTITY_OF_REGISTERS_KEY[]            = "QuantityOfRegisters";
const char MODBUS_MASTER_ACTION_INTERVAL_KEY[]                  = "ActionInterval";
const char MODBUS_MASTER_PROCESS_IMAGE_VARIABLE_NAME_KEY[]      = "DeviceValue";
const char MODBUS_MASTER_ACTION_STATUS_BYTE[]                   = "ModbusActionStatus";
const char MODBUS_MASTER_ACTION_STATUS_RESET[]                  = "ActionStatusReset";

const char MODBUS_MASTER_MASTER_STATUS_BYTE[]                   = "ModbusMasterStatus";
//const char MODBUS_MASTER_MASTER_STATUS_BYTE_VAR_NAME[]          = "Modbus_Master_Status";
const char MODBUS_MASTER_MASTER_STATUS_RESET_BYTE[]             = "MasterStatusReset";
//const char MODBUS_MASTER_MASTER_STATUS_RESET_BYTE_VAR_NAME[]    = "Master_Status_Reset";

const int MODBUS_MASTER_MASTER_STATUS_BYTE_OFFSET               = 100;
const int MODBUS_MASTER_MASTER_STATUS_RESET_BYTE_OFFSET         = 173;

TModbusSlaveConfiguration *tModbusSlaveConfig;
static char *c8PiConfigData_g = NULL;


/******************************************************************************/
/** @ brief translates error codes of this module to printable string
 *
 *	@param[in] error_code
 *
 *	@return constant character array
 */
/*****************************************************************************/
static const char* get_parsing_error_translation(parsing_error error_code)
{
    switch (error_code)
    {
        case ACTION_ADDRESS_WRONG_FORMAT:
            return "Action address has wrong format";
        case ACTION_DATA_SECTION_NOT_FOUND:
            return "Action data object not found";
        case ACTION_FUNCTION_CODE_WRONG_FORMAT:
            return "Action fuction code has wrong format";
        case ACTION_ID_WRONG_FORMAT:
            return "Action data object id has wrong format";
        case ACTION_INTERVALL_WRONG_FORMAT:
            return "Action intervall has wrong format";
        case ACTION_REGISTER_ADDRESS_WRONG_FORMAT:
            return "Action register address has wrong format";
        case ACTION_REGISTER_QUANTITY_WRONG_FORMAT:
            return "Action register quantity has wrong format";
        case DEVICE_RESET_ENTRIES_NOT_FOUND:
            return "Device reset object not found";
        case DEVICES_SECTION_NOT_FOUND:
            return "Device object not found";
        case DEVICES_SECTION_EMPTY:
            return "Device list empty";
        case EXTEND_SECTION_NOT_FOUND:
            return "Configuration object not found";
        case GENERAL_EXCEPTION:
            return "Unknown error";
        case INPUT_PARAMETER_SECTION_NOT_FOUND:
            return "Input parameter object not found";
        case INTERFACE_SECTION_NOT_FOUND:
            return "Interface object not found";
        case LENGTH_POSITION_PARAMETER_NOT_FOUND:
            return "Device position parameter not found";
        case LENGTH_POSITION_PARAMETER_WRONG_TYPE:
            return "Device position parameter has wrong type";
        case MAX_CONNECTIONS_NOT_FOUND:
            return "Max connections object not found";
        case MAX_CONNECTIONS_LIMIT_EXCEEDED:
            return "Limit of connections exceeded";
        case OFFSET_POSITION_PARAMETER_NOT_FOUND:
            return "Offset position parameter not found";
        case OFFSET_POSITION_PARAMETER_WRONG_TYPE:
            return "Offset position parameter has wrong type";
        case OUTPUT_PARAMETER_SECTION_NOT_FOUND:
            return "Output parameter object not found";
        case PROCESS_IMAGE_OBJECT_EMPTY:
            return "Process image object empty";
        case PROCESS_IMAGE_WRONG_TYPE:
            return "Process image image has wrong type";
        case PRODUCT_NAME_NOT_FOUND:
            return "Product type not found";
        case PRODUCT_TYPE_SECTION_EMPTY:
            return "Product name not found";
        case PRODUCT_TYPE_SECTION_NOT_FOUND:
            return "Product type empty";
        case TCP_ADDRESS_NOT_FOUND:
            return "IP address not found";
        case TCP_ADDRESS_WRONG_FORMAT:
            return "IP address has wrong format";
        case TCP_PORT_NOT_FOUND:
            return "TCP port not found";
        case TCP_PORT_WRONG_FORMAT:
            return "TCP port has wrong format";
        case RTU_BAUDRATE_NOT_FOUND:
            return "Baud rate for RTU connection not found";
        case RTU_BAUDRATE_WRONG_FORMAT:
            return "Baud rate for RTU connection has wrong format";
        case RTU_DATABITS_NOT_FOUND:
            return "Data bits for RTU connection not found";
        case RTU_DATABITS_SIZE:
            return "Data bits for RTU connection has wrong size";
        case RTU_DEVICE_PATH_NOT_FOUND:
            return "Device path for RTU connection not found";
        case RTU_DEVICE_PATH_LENGTH_EXCEEDED:
            return "Device path for RTU connection length exceeded";
        case RTU_PARITY_LENGTH_EXCEEDED:
            return "Parity for RTU connection length exceeded";
        case RTU_PARITY_NOT_FOUND:
            return "Parity for RTU connection not found";
        case RTU_PARITY_WRONG_FORMAT:
            return "Parity for RTU connection has wrong format";
        case RTU_STOPBITS_NOT_FOUND:
            return "Stop bits for RTU connection not found";
        case RTU_STOPBITS_SIZE:
            return "Stop bits for RTU connection has wrong size";
        case RTU_MODBUS_ADDRESS_NOT_FOUND:
            return "Modbus address for RTU connection not found";
        case RTU_MODBUS_ADDRESS_SIZE:
            return "Modbus address for RTU connection has wrong size";
        case UNKNOWN_MODBUS_DEVICE:
            return "Modbus device is unknown";
        case SUCCESS:
            return "Success";
    }
    return "Unknown error"; // i.e. GENERAL_EXCEPTION
}


/******************************************************************************/
/** @ brief print syslog in case of error
 *
 *	@param[in] error_code
 *
 *	@return void
 */
/*****************************************************************************/
static void print_err(parsing_error error_code)
{
    syslog(LOG_ERR, "Parsing config file failed: %s\n", get_parsing_error_translation(error_code));
}


/******************************************************************************/
/** @ brief creates a list of all matching devices
 *          (modbus tcp slave or modbus rtu slave)
 *
 *	@param[out] *p_mbSlaveConfHead_p pointer to the list head
 *
 *	@return void
 */
/*****************************************************************************/
void get_slave_device_config_list(struct TMBSlaveConfHead *p_mbSlaveConfHead_p)
{
    c8PiConfigData_g = read_config_file();
    if (c8PiConfigData_g != NULL)
    {
        int32_t success = parse_modbus_slaves_config_data(c8PiConfigData_g, p_mbSlaveConfHead_p);
        if(success < 0)
        {
            print_err(success);
        }
    }
}


/******************************************************************************/
/** @ brief creates a list of all matching devices
 *          (modbus tcp master or modbus rtu master)
 *
 *	@param[out] *p_mbSlaveConfHead_p pointer to the list head
 *
 *	@return void
 */
/*****************************************************************************/
void get_master_device_config_list(struct TMBMasterConfHead *p_mbMasterConfHead_p)
{
    c8PiConfigData_g = read_config_file();
    if (c8PiConfigData_g != NULL)
    {
        int32_t success = parse_modbus_master_config_data(c8PiConfigData_g, p_mbMasterConfHead_p);
        if (success < 0)
        {
            print_err(success);
        }
    }
}


void free_config_buffer(void)
{
    if (c8PiConfigData_g)
    {
        free(c8PiConfigData_g);
        c8PiConfigData_g = NULL;
    }
}


/************************************************************************/
/** @ brief reads the config file specified
 *
 *	@param[in] pc8_config_file_path_p pointer to config file path
 *
 *	@return pointer to the config file data or
 *          NULL if file doesn't exist or is empty or a read eror occured
 *
 */
/************************************************************************/
char* read_config_file(void)
{
    char* c8PiConfigData = NULL;
    FILE *config_file = NULL;

    config_file = fopen(PICONFIG_FILE, "r");
    if (config_file == NULL)
    {
        // try old filename/location
        config_file = fopen(PICONFIG_FILE_WHEEZY, "r");
    }

    if (config_file != NULL)
    {
        if (fseek(config_file, 0L, SEEK_END) == 0)
        {
            long buffer_size = ftell(config_file);
            if (buffer_size <= 0)
            {
                fclose(config_file);
                return c8PiConfigData; // == NULL
            }

            c8PiConfigData = calloc((buffer_size + 1), sizeof(char));
            if (c8PiConfigData == NULL)
            {
                fclose(config_file);
                return c8PiConfigData; // == NULL
            }

            if (fseek(config_file, 0L, SEEK_SET) != 0)
            {
                free(c8PiConfigData);
                fclose(config_file);
                return NULL;
            }

            size_t len = fread(c8PiConfigData, (sizeof(char)), buffer_size, config_file);
            assert(buffer_size > 0);
            if (len < (size_t)buffer_size)
            {
                free(c8PiConfigData);
                fclose(config_file);
                return NULL;
            }
            c8PiConfigData[len] = '\0';
        }
        fclose(config_file);
    }
    return c8PiConfigData;
}


/************************************************************************/
/** @ brief parse the json config data of a modbus device
 *
 *	@param[in]  pi_device_p pointer to json object which contains
 *              the device configuration
 *	@param[out] modbusSlaveConfiguration_p pointer to data structure
 *              which stores the modbus device config data
 *
 *	@return '0' if processing was successful, otherwise a negative value
 */
/************************************************************************/
int32_t parse_modbus_slave_device_process_image_config(json_object *pi_device_p,
    TModbusSlaveConfiguration* modbusSlaveConfiguration_p)
{
    int32_t processImageDeviceOffset = -1;

    //process image device offset
    processImageDeviceOffset = get_process_image_device_offset(pi_device_p);
    if (processImageDeviceOffset < 0)
    {
        return processImageDeviceOffset;
    }

    uint32_t u32_process_image_input_start_address = 0;
    uint32_t u32_process_image_input_size = 0;
    uint32_t u32_process_image_output_start_address = 0;
    uint32_t u32_process_image_output_size = 0;
    int32_t successful = -1;

    //process image holding registers input data config
    json_object *json_process_image_input_params = NULL;
    if (!(json_object_object_get_ex(pi_device_p, "inp", &json_process_image_input_params)))
    {
        return INPUT_PARAMETER_SECTION_NOT_FOUND;
    }

    successful = get_device_process_image_parameter(json_process_image_input_params,
        &u32_process_image_input_start_address,
        &u32_process_image_input_size);
    if (successful != 0)
    {
        print_err(successful);
    }
    assert((u32_process_image_input_size % 2) == 0);	//for word size holding and input registers
    modbusSlaveConfiguration_p->tProcessImageConfig.u32HoldingRegistersLength = u32_process_image_input_size;
    modbusSlaveConfiguration_p->tProcessImageConfig.u32HoldingRegistersInputOffset = u32_process_image_input_start_address + processImageDeviceOffset;
    modbusSlaveConfiguration_p->tModbusDataConfig.u16HoldingRegisters = u32_process_image_input_size / 2;

    //process image output input registers data config
    json_object *json_process_image_output_params = NULL;
    if (!(json_object_object_get_ex(pi_device_p, "out", &json_process_image_output_params)))
    {
        return OUTPUT_PARAMETER_SECTION_NOT_FOUND;
    }

    successful = get_device_process_image_parameter(json_process_image_output_params,
        &u32_process_image_output_start_address,
        &u32_process_image_output_size);
    if (successful != 0)
    {
        print_err(successful);
    }
    assert((u32_process_image_output_size % 2) == 0);	//for word size holding and input registers
    modbusSlaveConfiguration_p->tProcessImageConfig.u32InputRegistersLength = u32_process_image_output_size;
    modbusSlaveConfiguration_p->tProcessImageConfig.u32InputRegistersOffset = u32_process_image_output_start_address + processImageDeviceOffset;
    modbusSlaveConfiguration_p->tModbusDataConfig.u16InputRegisters = u32_process_image_output_size / 2;

    //config coils
    //config discrete inputs

    return 0;
}


/*****************************************************************************/
/** @ brief get json devices array from config.rsc
 *
 *	@param[in] pc8_pi_config_data_p pointer to config data
 *	@param[out] p_devices_array_p pointer to json aray of all devices in config file
 *
 *	@return '0' if processing was successful, otherwise a negative value
 *
 */
/*****************************************************************************/
int32_t get_json_devices_array(const char* pc8_pi_config_data_p, struct array_list **pp_devices_array_p)
{
    json_object *json_config = NULL;
    json_object *json_devices = NULL;

    json_config = json_tokener_parse(pc8_pi_config_data_p);
    if (json_config == NULL)
    {
        return GENERAL_EXCEPTION;
    }

    if (!(json_object_object_get_ex(json_config, "Devices", &json_devices)))
    {
        return DEVICES_SECTION_NOT_FOUND;
    }

    *pp_devices_array_p = json_object_get_array(json_devices);
    if (*pp_devices_array_p == NULL)
    {
        return DEVICES_SECTION_EMPTY;
    }
    return 0;
}


/*****************************************************************************/
/** @ brief get the string of the product type from config.rsc
 *
 *	@param[in] pi_device pointer to json object which contains the device information
 *	@param[out] pc8_productType pointer to the product type
 *
 *	@return '0' if processing was successful, otherwise a negative value
 *
 */
/*****************************************************************************/
int32_t get_device_product_type(json_object *pi_device, const char **ppc8_productType)
{
    json_object *json_pi_product_type = NULL;

    if (!(json_object_object_get_ex(pi_device, "productType", &json_pi_product_type)))
    {
        return PRODUCT_TYPE_SECTION_EMPTY;
    }
    if (json_object_get_type(json_pi_product_type) != json_type_string)
    {
        return PRODUCT_TYPE_SECTION_EMPTY;
    }
    *ppc8_productType = json_object_get_string(json_pi_product_type);
    return 0;
}


/*****************************************************************************/
/** @ brief parse the json config.rsc data for virtual device modbus masters
 *
 *	@param[in] pc8_pi_config_data_p pointer to config.rsc data
 *	@param[out] p_mbMasterConfHead_p head to master config list
 *
 *	@return '0' if processing was successful, otherwise a negative value
 *
 */
/*****************************************************************************/
int32_t parse_modbus_master_config_data(const char* pc8_pi_config_data_p, struct TMBMasterConfHead *p_mbMasterConfHead_p)
{
    struct array_list *devices_array = NULL;
    int32_t success = get_json_devices_array(pc8_pi_config_data_p, &devices_array);
    if (success < 0)
    {
        // this would lead to consequential errors
        return success;
    }

    //search for matching device types
    for (size_t i = 0; i < array_list_length(devices_array); i++)
    {
        json_object *json_pi_device = NULL;
        const char* productType = NULL;
        json_pi_device = (json_object*)(array_list_get_idx(devices_array, i));
        int32_t success = get_device_product_type(json_pi_device, &productType);
        if (success < 0)
        {
            return success;
        }

        if ((memcmp(productType, MODBUS_MASTER_TCP_PI_PRODUCT_TYPE, strlen(productType)) == 0) ||
                (memcmp(productType, MODBUS_MASTER_RTU_PI_PRODUCT_TYPE, strlen(productType)) == 0))
        {
            //parse this device
            struct TMBMasterConfigEntry *nextConfig = calloc(1, sizeof(struct TMBMasterConfigEntry));
            if (nextConfig == NULL)
            {
                syslog(LOG_ERR, "parsing modbus configuration failed. Memory allocation failed.\n");
                continue;
            }
            success = parse_device_modbus_configuration(json_pi_device, &(nextConfig->mbMasterConfig.tModbusDeviceConfig));
            if (success < 0)
            {
                print_err(success);
                free(nextConfig);
                continue;
            }

            nextConfig->mbMasterConfig.i32ActionCount = parse_modbus_master_action_list(json_pi_device, &(nextConfig->mbMasterConfig.mbActionListHead));
            if(nextConfig->mbMasterConfig.i32ActionCount < 0)
            {
                print_err(nextConfig->mbMasterConfig.i32ActionCount);
                free(nextConfig);
                continue;
            }

            //search for device status byte offset in inp and out list of device
            //get status byte variable name
            json_object *json_config_extend = NULL;
            json_object *json_modbus_master_status = NULL;
            struct json_object_iter iter;

            if (!(json_object_object_get_ex(json_pi_device, "extend", &json_config_extend)))
            {
                return EXTEND_SECTION_NOT_FOUND;
            }
            if (!(json_object_object_get_ex(json_config_extend, "deviceMisc", &json_modbus_master_status)))
            {
                return DEVICE_RESET_ENTRIES_NOT_FOUND;
            }
#if 1
            uint32_t byte_offset = 0;
            uint32_t bit_offset = 0;

            json_object_object_foreachC(json_modbus_master_status, iter)
            {
                //find the modbus Master status field whos name starts with 'ModbusMasterStatus'
                if(memcmp(MODBUS_MASTER_MASTER_STATUS_BYTE, iter.key, (sizeof(MODBUS_MASTER_MASTER_STATUS_BYTE) / sizeof(char))-1) == 0)
                {
                    int32_t success = get_variable_parameters(json_pi_device, json_object_get_string(iter.val), &byte_offset, &bit_offset);
                    //search for variable name in inp and out list of device
                    if(success == 0)
                    {
                        nextConfig->mbMasterConfig.tModbusDeviceConfig.i32uDeviceStatusByteProcessImageOffset = byte_offset;
                    }
                    else
                    {
                        print_err(success);
                    }
                }
                else if(memcmp(MODBUS_MASTER_MASTER_STATUS_RESET_BYTE, iter.key, (sizeof(MODBUS_MASTER_MASTER_STATUS_RESET_BYTE) / sizeof(char))-1) == 0)
                {
                    int32_t success = get_variable_parameters(json_pi_device, json_object_get_string(iter.val), &byte_offset, &bit_offset);
                    //search for variable name in inp and out list of device
                    if(success == 0)
                    {
                        nextConfig->mbMasterConfig.tModbusDeviceConfig.i32uDeviceStatusResetByteProcessImageByteOffset = byte_offset;
                    }
                    else
                    {
                        print_err(success);
                    }
                }
            }
#else
            int32_t processImageDeviceOffset = get_process_image_device_offset(json_pi_device);
            nextConfig->mbMasterConfig.tModbusDeviceConfig.i32uDeviceStatusByteProcessImageOffset = processImageDeviceOffset + MODBUS_MASTER_MASTER_STATUS_BYTE_OFFSET;
            nextConfig->mbMasterConfig.tModbusDeviceConfig.i32uDeviceStatusResetByteProcessImageByteOffset = processImageDeviceOffset + MODBUS_MASTER_MASTER_STATUS_RESET_BYTE_OFFSET;
#endif
            //insert parsed config to master list
            SLIST_INSERT_HEAD(p_mbMasterConfHead_p, nextConfig, entries);
        }
    }
    return 0;
}


void free_modbus_master_config_data(struct TMBMasterConfHead *p_mbMasterConfHead_p)
{
    while (SLIST_FIRST(p_mbMasterConfHead_p))
    {
        struct TMBMasterConfigEntry *entry = SLIST_FIRST(p_mbMasterConfHead_p);
        SLIST_REMOVE_HEAD(p_mbMasterConfHead_p, entries);
        while (!SLIST_EMPTY(&entry->mbMasterConfig.mbActionListHead))
        {
            struct TMBActionEntry *act = SLIST_FIRST(&entry->mbMasterConfig.mbActionListHead);
            SLIST_REMOVE_HEAD(&entry->mbMasterConfig.mbActionListHead, entries);
            free(act);
        }
        free(entry);
    }
}


/*****************************************************************************/
/** @ brief parse the json config.rsc data for virtual device modbus slaves
 *
 *	@param[in] pc8_pi_config_data_p pointer to config data
 *	@param[out] p_mbSlaveConfHead_p header to linked list for parsed device config data
 *
 *	@return '0' if processing was successful, otherwise a negative value
 *
 */
/*****************************************************************************/
int32_t parse_modbus_slaves_config_data(const char* pc8_pi_config_data_p, struct TMBSlaveConfHead *p_mbSlaveConfHead_p)
{
    struct array_list *devices_array = NULL;
    int32_t success = get_json_devices_array(pc8_pi_config_data_p, &devices_array);
    if (success < 0)
    {
        return success;
    }

    //search for matching device types
    for (size_t i = 0; i < array_list_length(devices_array); i++)
    {
        json_object *pi_device = NULL;
        const char* productType = NULL;
        pi_device = (json_object*)(array_list_get_idx(devices_array, i));
        int32_t success = get_device_product_type(pi_device, &productType);
        if (success < 0)
        {
            return success;
        }

        if ((memcmp(productType, MODBUS_SLAVE_TCP_PI_PRODUCT_TYPE, strlen(productType)) == 0) ||
                (memcmp(productType, MODBUS_SLAVE_RTU_PI_PRODUCT_TYPE, strlen(productType)) == 0))
        {
            //parse this device
            struct TMBSlaveConfigEntry *nextConfig = calloc(1, sizeof(struct TMBSlaveConfigEntry));
            if (nextConfig == NULL)
            {
                syslog(LOG_ERR, "parsing modbus configuration failed. Memory allocation failed.\n");
                continue;
            }

            int32_t success = parse_device_modbus_configuration(pi_device, &(nextConfig->mbSlaveConfig.tModbusDeviceConfig));
            if (success < 0)
            {
                print_err(success);
                free(nextConfig);
                continue;
            }

            success = parse_modbus_slave_device_process_image_config(pi_device, &(nextConfig->mbSlaveConfig));
            if (success < 0)
            {
                print_err(success);
                free(nextConfig);
                continue;
            }
            SLIST_INSERT_HEAD(p_mbSlaveConfHead_p, nextConfig, entries);
        }
    }
    return 0;
}


/*****************************************************************************/
/** @ brief parses the given pi process image json config data section
 *
 *	@param[in] json_process_image_object_p pointer to json object which contains
 *			   the configuration (e.g. Devices.[i].inp or Devices.[i].out)
 *	@param[out] u32_relative_process_image_offset_p the relative process image offset
 *				of the specified json process image object
 *	@param[out] u32_process_image_length_p length of this process image section in bytes
 *
 *	@return '0' if processing was successful, otherwise a negative value
 */
/*****************************************************************************/
int32_t get_device_process_image_parameter(json_object *json_process_image_object_p, uint32_t* u32_relative_process_image_offset_p, uint32_t* u32_process_image_length_p)
{
    uint32_t u32_process_image_start_address = UINT32_MAX;
    uint32_t u32_process_image_size = 0;
    json_object *json_highest_offset_variable_size = NULL;

    if (json_object_get_type(json_process_image_object_p) != json_type_object)
    {
        return PROCESS_IMAGE_WRONG_TYPE;
    }

    json_object_object_foreach(json_process_image_object_p, key, val)
    {
        (void)key;

        // find entry with the smallest relative offset(fourth value in array)
        // find entry with the highest relative offset(fourth value in array)
        struct array_list *input_data_array = json_object_get_array(val);
        if (input_data_array == NULL)
        {
            return PROCESS_IMAGE_OBJECT_EMPTY;
        }

        json_object *json_single_input_param = (json_object*)array_list_get_idx(input_data_array, RELATIVE_PROCESS_IMAGE_VARIABLE_BYTE_OFFSET_ARRAY_POSITION);
        if (json_single_input_param == NULL)
        {
            return OFFSET_POSITION_PARAMETER_NOT_FOUND;
        }

        const char *pst8_relative_input_offset = NULL;
        uint32_t u32_relative_process_image_offset = 0;
        if (json_type_string != json_object_get_type(json_single_input_param))
        {
            return OFFSET_POSITION_PARAMETER_WRONG_TYPE;
        }

        errno = 0;
        pst8_relative_input_offset = json_object_get_string(json_single_input_param);
        u32_relative_process_image_offset = strtoumax(pst8_relative_input_offset, NULL, 10);
        if (errno != 0)
        {
            //error
            syslog(LOG_ERR, "parsing config file failed: %s", strerror(errno));
            return GENERAL_EXCEPTION;
        }

        //start offset
        if (u32_process_image_start_address > u32_relative_process_image_offset)
        {
            u32_process_image_start_address = u32_relative_process_image_offset;
        }
        //highest offset
        if (u32_process_image_size < u32_relative_process_image_offset)
        {
            u32_process_image_size = u32_relative_process_image_offset;
            json_highest_offset_variable_size = (json_object*)array_list_get_idx(input_data_array, PROCESS_IMAGE_VARIABLE_BIT_LENGTH_ARRAY_POSITION);
            if (json_single_input_param == NULL)
            {
                return OFFSET_POSITION_PARAMETER_NOT_FOUND;
            }
        }
    }
    //add variable data length to total length of process image input size
    if (json_type_string != json_object_get_type(json_highest_offset_variable_size))
    {
        return OFFSET_POSITION_PARAMETER_WRONG_TYPE;
    }

    errno = 0;
    const char *pst8_last_variable_bit_size = json_object_get_string(json_highest_offset_variable_size);
    uint32_t u32_last_variable_bit_size = strtoumax(pst8_last_variable_bit_size, NULL, 10);
    if (errno != 0)
    {
        //error
        syslog(LOG_ERR, "parsing config file failed: %s", strerror(errno));
        return GENERAL_EXCEPTION;
    }

    u32_process_image_size = u32_process_image_size + (u32_last_variable_bit_size / 8) - u32_process_image_start_address;

    *u32_relative_process_image_offset_p = u32_process_image_start_address;
    *u32_process_image_length_p = u32_process_image_size;

    return 0;
}


/*****************************************************************************/
/** @ brief returns a device parameter of json type string
 *			specified by a json_object and json_key_value
 *
 *	@param[in] json_device_config_parameters_p pointer to json object which contains
 *			   the configuration data
 *	@param[in] json_key_p key of the required data
 *
 *	@return required data as char pointer if successful, otherwise 'NULL'
 */
/*****************************************************************************/
const char* get_device_string_parameter(json_object *json_device_config_parameters_p, const char* json_key_p)
{
    const char* ret_val = NULL;
    json_object *json_obj_entry = NULL;
    struct array_list *config_parameter = NULL;
    json_object *array_entry = NULL;

    if (json_key_p == NULL)
    {
        return ret_val;
    }
    if (!(json_object_object_get_ex(json_device_config_parameters_p, json_key_p, &json_obj_entry)))
    {
        syslog(LOG_ERR, "parsing config failed, parameter %s\n", json_key_p);
        return ret_val;
    }
    config_parameter = json_object_get_array(json_obj_entry);
    if (config_parameter == NULL)
    {
        syslog(LOG_ERR, "parsing config failed, no device parameter\n");
        return ret_val;
    }

    array_entry = (json_object*)(array_list_get_idx(config_parameter, MODBUS_SLAVE_PARAMETER_ARRAY_POSITION));
    if (json_object_get_type(array_entry) != json_type_string)
    {
        syslog(LOG_ERR, "parsing config failed, wrong parameter type\n");
        return ret_val;
    }
    ret_val = json_object_get_string(array_entry);

    return ret_val;
}


/*****************************************************************************/
/** @ brief parses the json modbus config and stores the data
 *
 *	@param[in]  json_device_object_p pointer to json object
 *              which contains the device configuration
 *	@param modbusDeviceConfig_p[out] pointer to data structure
 *              which stores the modbus device config data
 *
 *	@return '0' if processing was successful, otherwise a negative value
 */
/*****************************************************************************/
int32_t parse_device_modbus_configuration(json_object *json_device_object_p, TModbusDeviceConfiguration* modbusDeviceConfig_p)
{
    const char* productType = NULL;
    json_object *json_pi_product_type = NULL;

    if (!(json_object_object_get_ex(json_device_object_p, "productType", &json_pi_product_type)))
    {
        return PRODUCT_TYPE_SECTION_NOT_FOUND;
    }
    if (json_object_get_type(json_pi_product_type) != json_type_string)
    {
        return PRODUCT_TYPE_SECTION_EMPTY;
    }
    productType = json_object_get_string(json_pi_product_type);
    json_object *json_modbus_config_parameters = NULL;
    if (!(json_object_object_get_ex(json_device_object_p, "mem", &json_modbus_config_parameters)))
    {
        return INTERFACE_SECTION_NOT_FOUND;
    }
    //modbus tcp
    if (memcmp(productType, MODBUS_MASTER_TCP_PI_PRODUCT_TYPE, strlen(productType)) == 0)
    {
        //set TCP in configuration
        modbusDeviceConfig_p->eProtocol = eProtTCP;
        const char *array_content_string = NULL;

        //set ip address
        array_content_string = get_device_string_parameter(json_modbus_config_parameters, MODBUS_MASTER_TCP_JSON_KEY_IP_ADDRESS);
        if (array_content_string == NULL)
        {
            return TCP_ADDRESS_NOT_FOUND;
        }
        if ((strlen(array_content_string)) >= (INET_ADDRSTRLEN))
        {
            return TCP_ADDRESS_WRONG_FORMAT;
        }
        strcpy(modbusDeviceConfig_p->uProt.tTcpConfig.szTcpIpAddress, array_content_string);

        //set TCP port
        array_content_string = get_device_string_parameter(json_modbus_config_parameters, MODBUS_MASTER_TCP_JSON_KEY_TCP_PORT);
        if (array_content_string == NULL)
        {
            return TCP_PORT_NOT_FOUND;
        }
        errno = 0;
        uint32_t tcp_port = strtoumax(array_content_string, NULL, 10);
        if ((errno != 0) || (tcp_port > USHRT_MAX))
        {
            //error
            syslog(LOG_ERR, "parsing config file tcp port failed: %s", strerror(errno));
            return TCP_PORT_WRONG_FORMAT;
        }
        modbusDeviceConfig_p->uProt.tTcpConfig.i32uPort = tcp_port;
    }
    else if ((memcmp(productType, MODBUS_SLAVE_TCP_PI_PRODUCT_TYPE, strlen(productType)) == 0))
    {
        //set TCP in configuration
        modbusDeviceConfig_p->eProtocol = eProtTCP;
        const char *array_content_string = NULL;

        //set ip address
        strcpy(modbusDeviceConfig_p->uProt.tTcpConfig.szTcpIpAddress, "0.0.0.0");

        //set TCP port
        array_content_string = get_device_string_parameter(json_modbus_config_parameters, MODBUS_SLAVE_TCP_JSON_KEY_TCP_PORT);
        if (array_content_string == NULL)
        {
            return TCP_PORT_NOT_FOUND;
        }
        errno = 0;
        uint32_t tcp_port = strtoumax(array_content_string, NULL, 10);
        if ((errno != 0) || (tcp_port > USHRT_MAX))
        {
            //error
            syslog(LOG_ERR, "parsing config file tcp port failed: %s", strerror(errno));
            return TCP_PORT_WRONG_FORMAT;
        }
        modbusDeviceConfig_p->uProt.tTcpConfig.i32uPort = tcp_port;

        if (memcmp(productType, MODBUS_SLAVE_TCP_PI_PRODUCT_TYPE, strlen(productType)) == 0)
        {
            //set max modbus/tcp connections
            array_content_string = get_device_string_parameter(json_modbus_config_parameters, MODBUS_SLAVE_TCP_JSON_KEY_TCP_MAX_CONNECTIONS);
            if (array_content_string == NULL)
            {
                return MAX_CONNECTIONS_NOT_FOUND;
            }
            errno = 0;
            uint32_t tcp_max_connections = strtoumax(array_content_string, NULL, 10);
            if ((errno != 0) || (tcp_max_connections > USHRT_MAX))
            {
                //error
                syslog(LOG_ERR, "parsing config file max modbus connections failed: %s", strerror(errno));
                return MAX_CONNECTIONS_LIMIT_EXCEEDED;
            }
            modbusDeviceConfig_p->uProt.tTcpConfig.maxModbusConnections = tcp_max_connections;
        }

    }
    //modbus rtu
    else if ((memcmp(productType, MODBUS_SLAVE_RTU_PI_PRODUCT_TYPE, strlen(productType)) == 0)
                    || (memcmp(productType, MODBUS_MASTER_RTU_PI_PRODUCT_TYPE, strlen(productType)) == 0))
    {

        //set RTU in configuration
        modbusDeviceConfig_p->eProtocol = eProtRTU;
        const char* array_content_string = NULL;

        //set device path
        array_content_string = get_device_string_parameter(json_modbus_config_parameters, MODBUS_SLAVE_RTU_JSON_KEY_DEVICE_PATH);
        if (array_content_string == NULL)
        {
            return RTU_DEVICE_PATH_NOT_FOUND;
        }
        if (strlen(array_content_string) >= PATH_MAX)
        {
            return RTU_DEVICE_PATH_LENGTH_EXCEEDED;
        }
        strcpy(modbusDeviceConfig_p->uProt.tRtuConfig.sz8DeviceFilePath, array_content_string);

        //set serial baudrate
        array_content_string = get_device_string_parameter(json_modbus_config_parameters, MODBUS_SLAVE_RTU_JSON_KEY_BAUDRATE);
        if (array_content_string == NULL)
        {
            return RTU_BAUDRATE_NOT_FOUND;
        }
        errno = 0;
        uint32_t rtu_baudrate = strtoumax(array_content_string, NULL, 10);
        if (errno != 0)
        {
            //error
            syslog(LOG_ERR, "parsing config file baudrate failed: %s", strerror(errno));
            return RTU_BAUDRATE_WRONG_FORMAT;
        }
        modbusDeviceConfig_p->uProt.tRtuConfig.i32uBaud = rtu_baudrate;

        //set parity for serial connection
        array_content_string = get_device_string_parameter(json_modbus_config_parameters, MODBUS_SLAVE_RTU_JSON_KEY_PARITY);
        if (array_content_string == NULL)
        {
            return RTU_PARITY_NOT_FOUND;
        }
        if (strlen(array_content_string) > sizeof(modbusDeviceConfig_p->uProt.tRtuConfig.cParity))
        {
            return RTU_PARITY_LENGTH_EXCEEDED;
        }
        if (array_content_string[0] == MODBUS_SLAVE_RTU_JSON_PARITY_VALUE_EVEN)
        {
            modbusDeviceConfig_p->uProt.tRtuConfig.cParity = 'E';
        }
        else if (array_content_string[0] == MODBUS_SLAVE_RTU_JSON_PARITY_VALUE_ODD)
        {
            modbusDeviceConfig_p->uProt.tRtuConfig.cParity = 'O';
        }
        else if (array_content_string[0] == MODBUS_SLAVE_RTU_JSON_PARITY_VALUE_NONE)
        {
            modbusDeviceConfig_p->uProt.tRtuConfig.cParity = 'N';
        }
        else
        {
            return RTU_PARITY_WRONG_FORMAT;
        }

        //set number of databits for serial connection
        array_content_string = get_device_string_parameter(json_modbus_config_parameters, MODBUS_SLAVE_RTU_JSON_KEY_DATABITS);
        if (array_content_string == NULL)
        {
            return RTU_DATABITS_NOT_FOUND;
        }
        errno = 0;
        uint32_t databits_count = strtoumax(array_content_string, NULL, 10);
        if ((errno != 0) || (databits_count < MODBUS_RTU_MIN_DATABITS) || (databits_count > MODBUS_RTU_MAX_DATABITS))
        {
            //error
            syslog(LOG_ERR, "parsing config file number of databits for serial connection failed: %s", strerror(errno));
            return RTU_DATABITS_SIZE;
        }
        modbusDeviceConfig_p->uProt.tRtuConfig.i8uDatabits = databits_count;

        //set number of stopbits for serial connection
        array_content_string = get_device_string_parameter(json_modbus_config_parameters, MODBUS_SLAVE_RTU_JSON_KEY_STOPBITS);
        if (array_content_string == NULL)
        {
            return RTU_STOPBITS_NOT_FOUND;
        }
        errno = 0;
        uint32_t stopbits_count = strtoumax(array_content_string, NULL, 10);
        if ((errno != 0) || (stopbits_count < MODBUS_RTU_MIN_STOPBITS) || (stopbits_count > MODBUS_RTU_MAX_STOPBITS))
        {
            //error
            syslog(LOG_ERR, "parsing config file number of databits for serial connection failed: %s", strerror(errno));
            return RTU_STOPBITS_SIZE;
        }
        modbusDeviceConfig_p->uProt.tRtuConfig.i8uStopbits = stopbits_count;

        if (memcmp(productType, MODBUS_SLAVE_RTU_PI_PRODUCT_TYPE, strlen(productType)) == 0)
        {
            //set modbus address
            array_content_string = get_device_string_parameter(json_modbus_config_parameters, MODBUS_SLAVE_RTU_MODBUS_ADDRESS);
            if (array_content_string == NULL)
            {
                return RTU_MODBUS_ADDRESS_NOT_FOUND;
            }
            errno = 0;
            uint32_t modbus_address = strtoumax(array_content_string, NULL, 10);
            if ((errno != 0) || (modbus_address > CHAR_MAX))
            {
                //error
                syslog(LOG_ERR, "parsing config file number of databits for serial connection failed: %s", strerror(errno));
                return RTU_MODBUS_ADDRESS_SIZE;
            }
            modbusDeviceConfig_p->uProt.tRtuConfig.u8DeviceModbusAddress = modbus_address;
        }
    }
    else
    {
        return UNKNOWN_MODBUS_DEVICE;
    }

    return 0;
}


/*****************************************************************************/
/** @ brief parse the modbus action list
 *
 *
 *	@param[in] json_modbus_actions_p
 *
 *	@param[in] tModbusActionListHead_p
 *
 *	@return 0 if successful, otherwise a negative value
 */
/*****************************************************************************/
int32_t parse_modbus_master_action_list(json_object *json_pi_device_p, struct TMBActionListHead *tModbusActionListHead_p)
{
    assert(json_pi_device_p != NULL);
    assert(tModbusActionListHead_p != NULL);
    SLIST_INIT(tModbusActionListHead_p);
    int32_t i32ActionCount = 0;

    /*	The modbus action_parameters_identifier is the char sequence between
     *	the first and the second underscore in the json name (tag)
     *  which is identical for all modbus action parameters of one modbus action
     */
    const char *action_parameters_identifier = NULL;
    const char *val_str_buffer = NULL;
    struct TMBActionEntry *nextAction = NULL;
    struct json_object_iter iter;

    json_object *json_config_extend = NULL;
    json_object *json_modbus_actions = NULL;

    //get action list from device
    if (!(json_object_object_get_ex(json_pi_device_p, "extend", &json_config_extend)))
    {
        return EXTEND_SECTION_NOT_FOUND;
    }
    if (!(json_object_object_get_ex(json_config_extend, "data", &json_modbus_actions)))
    {
        return ACTION_DATA_SECTION_NOT_FOUND;
    }


    json_object_object_foreachC(json_modbus_actions, iter)
    {
        //find the modbus action parameters which names starts with 'ActionId'
        if (memcmp(MODBUS_MASTER_ACTION_ID_KEY, iter.key, (sizeof(MODBUS_MASTER_ACTION_ID_KEY) / sizeof(char))-1) == 0)
        {
            //get the modbus action parameters identifier for this action id
            //which is identical for all modbus action parameters of this modbus action
            //The modbus action parameters identifier is the char sequence between
            //the first and the second underscore in the json name (tag)
            regex_t regex;
            regcomp(&regex, "_([^_]+)_", REG_EXTENDED);
            regmatch_t matches[1];
            if (regexec(&regex, iter.key, sizeof(matches) / sizeof(regmatch_t), matches, 0) == 0)
            {
                action_parameters_identifier = calloc((matches[0].rm_eo - matches[0].rm_so) + 1, sizeof(char));
                memcpy((void*)action_parameters_identifier, (void*)&(iter.key[matches[0].rm_so]), (matches[0].rm_eo - matches[0].rm_so));
            }

            //allocate memory for next modbus action
            nextAction = calloc(1, sizeof(struct TMBActionEntry));
            if (nextAction == NULL)
            {
                syslog(LOG_ERR, "parsing modbus action list failed. Memory allocation failed.\n");
                if (action_parameters_identifier != NULL)
                {
                    free((void*)action_parameters_identifier);
                }
                return GENERAL_EXCEPTION;
            }

            //set actionId
            val_str_buffer = get_modbus_action_matching_name_string_value(
                    json_modbus_actions,
                MODBUS_MASTER_ACTION_ID_KEY,
                action_parameters_identifier);
            if (val_str_buffer == NULL)
            {
                free((void*)nextAction);
                nextAction = NULL;
                continue;
            }
            errno = 0;
            uint32_t actionID = strtoul(val_str_buffer, NULL, 10);
            if (errno != 0)
            {
                return ACTION_ID_WRONG_FORMAT;
            }
            assert((actionID > 0) && (actionID < INT32_MAX));  //check min 1, max INT32_MAX
            nextAction->modbusAction.i16uActionID = actionID;

            //set slave address
            val_str_buffer = get_modbus_action_matching_name_string_value(
                    json_modbus_actions,
                MODBUS_MASTER_SLAVE_ADDRESS_KEY,
                action_parameters_identifier);
            if (val_str_buffer == NULL)
            {
                free((void*)nextAction);
                nextAction = NULL;
                continue;
            }
            errno = 0;
            uint32_t slave_address = strtoul(val_str_buffer, NULL, 10);
            if (errno != 0)
            {
                return ACTION_ADDRESS_WRONG_FORMAT;
            }
            //assert(slave_address < 248);     //see modbus station address specifications, '0' for broadcast
            nextAction->modbusAction.i8uSlaveAddress = slave_address;

            //set modbus function code
            val_str_buffer = get_modbus_action_matching_name_string_value(
                    json_modbus_actions,
                MODBUS_MASTER_FUNCTON_CODE_KEY,
                action_parameters_identifier);
            if (val_str_buffer == NULL)
            {
                free((void*)nextAction);
                nextAction = NULL;
                continue;
            }
            errno = 0;
            uint32_t modbus_function_code = strtoul(val_str_buffer, NULL, 10);
            if (errno != 0)
            {
                return ACTION_FUNCTION_CODE_WRONG_FORMAT;
            }
            assert((modbus_function_code >= eREAD_COILS) && (modbus_function_code < eWRITE_AND_READ_REGISTERS));
            nextAction->modbusAction.eFunctionCode = (EModbusFunction)modbus_function_code;

            //set modbus register address
            val_str_buffer = get_modbus_action_matching_name_string_value(
                    json_modbus_actions,
                MODBUS_MASTER_REGISTER_ADDRESS_KEY,
                action_parameters_identifier);
            if (val_str_buffer == NULL)
            {
                free((void*)nextAction);
                nextAction = NULL;
                continue;
            }
            errno = 0;
            uint32_t register_address = strtoul(val_str_buffer, NULL, 10);
            if (errno != 0)
            {
                return ACTION_REGISTER_ADDRESS_WRONG_FORMAT;
            }
            assert((register_address > 0) && (register_address < 0x10000));  //check min/max register address
            nextAction->modbusAction.i32uStartRegister = register_address;

            //set modbus register quantity
            val_str_buffer = get_modbus_action_matching_name_string_value(
                    json_modbus_actions,
                MODBUS_MASTER_QUANTITY_OF_REGISTERS_KEY,
                action_parameters_identifier);
            if (val_str_buffer == NULL)
            {
                free((void*)nextAction);
                nextAction = NULL;
                continue;
            }
            errno = 0;
            uint32_t quantity_of_registers = strtoul(val_str_buffer, NULL, 10);
            if (errno != 0)
            {
                return ACTION_REGISTER_QUANTITY_WRONG_FORMAT;
            }

            if (quantity_of_registers > MAX_REGISTER_SIZE_PER_ACTION)
            {
                syslog(LOG_ERR,
                    "Error PiCtory, quantity of registers configured for action ID %d exceeds MAX REGISTER SIZE PER ACTION %d \n",
                    nextAction->modbusAction.i16uActionID,
                    MAX_REGISTER_SIZE_PER_ACTION);
            }

            assert((quantity_of_registers > 0) && (quantity_of_registers <= MAX_REGISTER_SIZE_PER_ACTION));    //check min/max register quantity
            nextAction->modbusAction.i16uRegisterCount = quantity_of_registers;

            //set modbus command interval
            val_str_buffer = get_modbus_action_matching_name_string_value(
                    json_modbus_actions,
                MODBUS_MASTER_ACTION_INTERVAL_KEY,
                action_parameters_identifier);
            if (val_str_buffer == NULL)
            {
                free((void*)nextAction);
                nextAction = NULL;
                continue;
            }
            errno = 0;
            uint32_t action_interval = strtoul(val_str_buffer, NULL, 10);
            if (errno != 0)
            {
                return ACTION_INTERVALL_WRONG_FORMAT;
            }
            assert((action_interval > 0) && (action_interval <= (1000 * 60 * 30)));  //check min 1 ms, max 0.5h = 1800000ms
            nextAction->modbusAction.i32uInterval_us = action_interval * 1000; //msec to usec

            uint32_t process_image_byte_offset = 0;
            uint32_t process_image_bit_offset = 0;

            //get modbus device value parameter (name of variable in pictory)
            val_str_buffer = get_modbus_action_matching_name_string_value(
                    json_modbus_actions,
                MODBUS_MASTER_PROCESS_IMAGE_VARIABLE_NAME_KEY,
                action_parameters_identifier);
            if (val_str_buffer == NULL)
            {
                free((void*)nextAction);
                nextAction = NULL;
                continue;
            }
            //search for variable name in inp and out list of device
            int32_t success = get_variable_parameters(json_pi_device_p, val_str_buffer, &process_image_byte_offset, &process_image_bit_offset);
            if (success != 0)
            {
                print_err(success);
                free((void*)nextAction);
                nextAction = NULL;
                continue;
            }
            nextAction->modbusAction.i32uStartByteProcessData = process_image_byte_offset;
            nextAction->modbusAction.i8uStartBitProcessData = process_image_bit_offset;
            if (nextAction->modbusAction.eFunctionCode == eREAD_COILS
                    || nextAction->modbusAction.eFunctionCode == eREAD_DISCRETE_INPUTS
                    || nextAction->modbusAction.eFunctionCode == eWRITE_SINGLE_COIL
                    || nextAction->modbusAction.eFunctionCode == eWRITE_MULTIPLE_COILS)
            {
                int offset = nextAction->modbusAction.i8uStartBitProcessData / 8;
                nextAction->modbusAction.i8uStartBitProcessData %= 8;
                nextAction->modbusAction.i32uStartByteProcessData += offset;
            }

            //get modbus action status offset
            val_str_buffer = get_modbus_action_matching_name_string_value(
                    json_modbus_actions,
                MODBUS_MASTER_ACTION_STATUS_BYTE,
                action_parameters_identifier);
            if (val_str_buffer == NULL)
            {
                free((void*)nextAction);
                nextAction = NULL;
                continue;
            }
            //search for variable name in inp and out list of device
            success = get_variable_parameters(json_pi_device_p, val_str_buffer, &process_image_byte_offset, &process_image_bit_offset);
            if (success != 0)
            {
                print_err(success);
                free((void*)nextAction);
                nextAction = NULL;
                continue;
            }
            nextAction->modbusAction.i32uStatusByteProcessImageOffset = process_image_byte_offset;

            //get modbus action status reset offset
            val_str_buffer = get_modbus_action_matching_name_string_value(
                    json_modbus_actions,
                MODBUS_MASTER_ACTION_STATUS_RESET,
                action_parameters_identifier);
            if (val_str_buffer == NULL)
            {
                free((void*)nextAction);
                nextAction = NULL;
                continue;
            }
            //search for variable name in inp and out list of device
            success = get_variable_parameters(json_pi_device_p, val_str_buffer, &process_image_byte_offset, &process_image_bit_offset);
            if (success != 0)
            {
                print_err(success);
                free((void*)nextAction);
                nextAction = NULL;
                continue;
            }
            nextAction->modbusAction.i32uResetStatusProcessImageByteOffset = process_image_byte_offset;
            nextAction->modbusAction.i8uResetStatusProcessImageBitOffset = (uint8_t)process_image_bit_offset;

            // initialize status byte and reset bit to 0
            SPIValue reset_data_l;
            reset_data_l.i16uAddress = (uint16_t)(nextAction->modbusAction.i32uResetStatusProcessImageByteOffset);
            reset_data_l.i8uBit      = nextAction->modbusAction.i8uResetStatusProcessImageBitOffset;
            reset_data_l.i8uValue    = 0;
            piControlSetBitValue(&reset_data_l);

            uint8_t reset_data = 0;
            piControlWrite(nextAction->modbusAction.i32uStatusByteProcessImageOffset, (uint32_t)1, &(reset_data));

            val_str_buffer = NULL;
            if (action_parameters_identifier != NULL)
            {
                free((void*)action_parameters_identifier);
                action_parameters_identifier = NULL;
            }
        }

        //add action to list
        if (nextAction != NULL)
        {
            SLIST_INSERT_HEAD(tModbusActionListHead_p, nextAction, entries);
            nextAction = NULL;
            i32ActionCount++;
        }
    }

    return i32ActionCount;
}


/*****************************************************************************/
/** @ brief search for specified json name (tag) and returns the corresponding
 *	        value if the value is of type string
 *
 *	@param[in] json_modbus_actions_p
 *
 *	@param[in] action_parameter_prefix_p
 *
 *	@param[in] size_of_action_parameter_prefix_p
 *
 *	@param[in] action_identifier_p
 *
 *	@param[in] size_of_action_identifier_p
 *
 *	@return pointer to char array if successful, otherwise NULL
 */
/*****************************************************************************/

const char* get_modbus_action_matching_name_string_value(
        struct json_object *json_modbus_actions_p,
    const char* action_parameter_prefix_p,
    const char* action_identifier_p)
{
    const char *action_parameter_prefix_and_identifier = NULL;
    struct json_object_iter iter;

    if (action_parameter_prefix_p != NULL && action_identifier_p != NULL)
    {
        action_parameter_prefix_and_identifier =
            calloc(strlen(action_parameter_prefix_p) + strlen(action_identifier_p) + 1, sizeof(char));
    }
    if (action_parameter_prefix_and_identifier == NULL)
    {
        syslog(LOG_ERR, "parsing modbus configuration failed. Memory allocation failed.\n");
        return NULL;
    }
    memcpy((void*)action_parameter_prefix_and_identifier,
        (void*)action_parameter_prefix_p,
        (strlen(action_parameter_prefix_p)));
    memcpy((void*)&(action_parameter_prefix_and_identifier[strlen(action_parameter_prefix_and_identifier)]),
        (void*)action_identifier_p,
        strlen(action_identifier_p));

    json_object_object_foreachC(json_modbus_actions_p, iter)
    {
        if (memcmp(action_parameter_prefix_and_identifier, iter.key, strlen(action_parameter_prefix_and_identifier)) == 0)
        {
            if (json_object_is_type(iter.val, json_type_string) < 0)
            {
                print_err(ACTION_ADDRESS_WRONG_FORMAT);
                free((void*)action_parameter_prefix_and_identifier);
                return NULL;
            }
            free((void*)action_parameter_prefix_and_identifier);
            return json_object_get_string(iter.val);
        }
    }
    free((void*)action_parameter_prefix_and_identifier);
    return NULL;
}


/*****************************************************************************/
/** @ brief search for specified json name (tag) in a config.rsc variable section (inp, out)
 *          and returns the corresponding values for absolute byte and bit offset as int
 *
 *	@param[in] json_pi_device_p
 *
 *	@param[out] byte_offset_p
 *
 *	@param[out] bit_offset_p
 *
 *
 *	@return 0 if successful, otherwise a negative value
 */
/*****************************************************************************/
int32_t get_variable_parameters(json_object *json_pi_device_p,
    const char* json_parameter_name_p,
    uint32_t *byte_offset_p,
    uint32_t *bit_offset_p)
{
    int32_t device_pi_process_image_offset = -1;
    json_object *json_pi_inp_out = NULL;

    //get device process image absolute offset
    device_pi_process_image_offset = get_process_image_device_offset(json_pi_device_p);
    if (device_pi_process_image_offset < 0)
    {
        return device_pi_process_image_offset;
    }

    //search in json block "inp"
    if (!(json_object_object_get_ex(json_pi_device_p, "inp", &json_pi_inp_out)))
    {
        return INPUT_PARAMETER_SECTION_NOT_FOUND;
    }

    int32_t rc1 = get_variable_relative_offsets(json_pi_inp_out,
        json_parameter_name_p,
        byte_offset_p,
        bit_offset_p);

    //search in json block "out"
    if (!(json_object_object_get_ex(json_pi_device_p, "out", &json_pi_inp_out)))
    {
        return OUTPUT_PARAMETER_SECTION_NOT_FOUND;
    }

    int32_t rc2 = get_variable_relative_offsets(json_pi_inp_out,
        json_parameter_name_p,
        byte_offset_p,
        bit_offset_p);

    if ((rc1 < 0) && (rc2 < 0))
    {
        return GENERAL_EXCEPTION;
    }
    else
    {
        *byte_offset_p = *byte_offset_p + device_pi_process_image_offset;
    }
    return 0;
}


int32_t get_variable_relative_offsets(json_object *json_in_out_section_p,
    const char* json_parameter_name_p,
    uint32_t *byte_offset_p,
    uint32_t *bit_offset_p)
{
    json_object_object_foreach(json_in_out_section_p, key, val)
    {
        (void)key;

        if (json_object_is_type(val, json_type_array) < 0)
        {
            print_err(OFFSET_POSITION_PARAMETER_WRONG_TYPE);
            continue;
        }
        struct array_list *input_data_array = json_object_get_array(val);
        if (input_data_array == NULL)
        {
            continue;
        }
        json_object *json_variable_name = (json_object*)array_list_get_idx(input_data_array, VARIABLE_NAME_ARRAY_POSITION);
        if (json_object_is_type(json_variable_name, json_type_string) < 0)
        {
            print_err(OFFSET_POSITION_PARAMETER_WRONG_TYPE);
            continue;
        }
        const char* variable_name = json_object_get_string(json_variable_name);

        //check if the variable names matches
        if ((strlen(variable_name) == strlen(json_parameter_name_p)) &&
                (memcmp(json_parameter_name_p, variable_name, strlen(json_parameter_name_p)) == 0))
        {
            //get byte offset
            json_object *json_byte_offset = (json_object*)array_list_get_idx(input_data_array, RELATIVE_PROCESS_IMAGE_VARIABLE_BYTE_OFFSET_ARRAY_POSITION);
            if (json_object_is_type(json_byte_offset, json_type_string) < 0)
            {
                print_err(OFFSET_POSITION_PARAMETER_WRONG_TYPE);
                continue;
            }
            const char* sz8Byte_offset = json_object_get_string(json_byte_offset);
            errno = 0;
            uint32_t u32Byte_offset = strtoumax(sz8Byte_offset, NULL, 10);
            if (errno != 0)
            {
                //error
                syslog(LOG_ERR, "parsing config file process variable byte offset failed: %s", strerror(errno));
                return OFFSET_POSITION_PARAMETER_WRONG_TYPE;
            }
            *byte_offset_p = u32Byte_offset;

            //get bit offset
            json_object *json_bit_offset = (json_object*)array_list_get_idx(input_data_array, RELATIVE_PROCESS_IMAGE_VARIABLE_BIT_OFFSET_ARRAY_POSITION);
            if (json_object_is_type(json_bit_offset, json_type_string) < 0)
            {
                print_err(OFFSET_POSITION_PARAMETER_WRONG_TYPE);
                continue;
            }
            const char* sz8bit_offset = json_object_get_string(json_bit_offset);
            errno = 0;
            uint32_t u32bit_offset = strtoumax(sz8bit_offset, NULL, 10);
            if (errno != 0)
            {
                //error
                syslog(LOG_ERR, "parsing config file process variable bit offset failed: %s", strerror(errno));
                return OFFSET_POSITION_PARAMETER_WRONG_TYPE;
            }
            *bit_offset_p = u32bit_offset;
            return 0;
        }
    }
    return GENERAL_EXCEPTION;
}


/*****************************************************************************/
/** @ brief get device process image absolute offset
 *
 *	@param[in] json_pi_device_p
 *
 *	@return the offset if successful, otherwise a negative value
 */
/*****************************************************************************/
int32_t get_process_image_device_offset(json_object *json_pi_device_p)
{
    json_object *json_device_pi_process_image_offset = NULL;
    int32_t device_pi_process_image_offset = -1;
    if (!(json_object_object_get_ex(json_pi_device_p, "offset", &json_device_pi_process_image_offset)))
    {
        return OFFSET_POSITION_PARAMETER_NOT_FOUND;
    }
    if (json_object_is_type(json_device_pi_process_image_offset, json_type_int) < 0)
    {
        return OFFSET_POSITION_PARAMETER_WRONG_TYPE;
    }
    errno = 0;
    device_pi_process_image_offset = json_object_get_int(json_device_pi_process_image_offset);
    if (errno != 0)
    {
        syslog(LOG_ERR, "parsing config file failed: %s", strerror(errno));
        return OFFSET_POSITION_PARAMETER_WRONG_TYPE;
    }

    return device_pi_process_image_offset;
}
