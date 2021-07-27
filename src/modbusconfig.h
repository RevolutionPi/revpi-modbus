#pragma once

#include <sys/types.h>
#include <stdint.h>
#include <modbus/modbus.h>
#include <limits.h>
#include <arpa/inet.h>
#include <sys/queue.h>


#define MODBUS_RTU_MIN_DATABITS 5
#define MODBUS_RTU_MAX_DATABITS 8
#define MODBUS_RTU_MIN_STOPBITS 1
#define MODBUS_RTU_MAX_STOPBITS 2



//max registersize/bitsize for modbus functions
#define MAX_MODBUS_READ_COILS_COUNT             2000
#define MAX_MODBUS_READ_DISCRETE_INPUTS_COUNT   2000
#define MAX_MODBUS_READ_HOLDING_REGISTERS_COUNT 125
#define MAX_MODBUS_READ_INPUT_REGISTERS_COUNT   125
#define MAX_MODBUS_WRITE_COILS_COUNT            1968
#define MAX_MODBUS_WRITE_REGISTERS_COUNT        123

//max register size for each modbus device action configured in pictory
#define MAX_REGISTER_SIZE_PER_ACTION            256

typedef enum
{
    eProtRTU,
    eProtTCP,
} EModbusProtocol;

typedef struct
{
    char szTcpIpAddress[INET_ADDRSTRLEN];     // String in numbers-and-dots notation ("a.b.c.d")
    int32_t i32uPort;
    int32_t maxModbusConnections;
} TTcpConfig;

typedef enum
{
    eREAD_COILS               = 0x01,
    eREAD_DISCRETE_INPUTS     = 0x02,
    eREAD_HOLDING_REGISTERS   = 0x03,
    eREAD_INPUT_REGISTERS     = 0x04,
    eWRITE_SINGLE_COIL        = 0x05,
    eWRITE_SINGLE_REGISTER    = 0x06,
    eREAD_EXCEPTION_STATUS    = 0x07,	//serial line only, not supported by libmodbus v3.0.6
    eWRITE_MULTIPLE_COILS     = 0x0F,
    eWRITE_MULTIPLE_REGISTERS = 0x10,
    eREPORT_SLAVE_ID          = 0x11,
    eWRITE_MASK_REGISTER      = 0x16,
    eWRITE_AND_READ_REGISTERS = 0x17,
} EModbusFunction;

typedef enum
{
    eNoError                = 0x00,     // no error
    eNoDevice               = 0x10,
    eNoResponseFromDevice   = 0x11,
    eModbusActionBacklog    = 0x12,
    eInternalError          = 0xf0,
} EModbusErrors;

typedef struct  
{
    uint16_t i16uVendorId;
    uint16_t i16uModelId;
    char astrSerialNumber[256];
    uint32_t i32uBaud;
    char cParity;					// Could be 'N', 'E' or 'O'
    uint8_t i8uDatabits;			// Could be 5, 6, 7 or 8
    uint8_t i8uStopbits;			// Could be 1 or 2
    uint8_t u8DeviceModbusAddress;	//Modbus device addresse between 1 and 255, 0 is reserved for broadcast
    char sz8DeviceFilePath[PATH_MAX];	//path to the specified device file e.g. "/dev/ttyUSB0"
} TRtuConfig;

typedef struct
{
    uint16_t i16uActionID;
    uint8_t i8uSlaveAddress;			//modbus slave address from 0(broadcast) to 
    EModbusFunction eFunctionCode;
    uint32_t i32uStartRegister;			//modbus adresses from 0x1 to 0x10000
    uint16_t i16uRegisterCount;			//data length in bits, (bytes) or words, depends on modbus function 
    uint32_t i32uInterval_us;			//interval in micro seconds
    uint32_t i32uStartByteProcessData;
    uint8_t i8uStartBitProcessData;
    uint32_t i32uStatusByteProcessImageOffset;		//The pi process image offset for the commands status byte
    uint32_t i32uResetStatusProcessImageByteOffset;	//The pi process image byte offset for the status reset
    uint8_t	i8uResetStatusProcessImageBitOffset;	//The pi process image bit offset for the status reset
        
} TModbusAction;

struct TMBActionEntry
{
    TModbusAction modbusAction;
    SLIST_ENTRY(TMBActionEntry) entries;
};


typedef struct
{
    uint16_t u16DiscreteInputs;		// # of discrete inputs
    uint16_t u16Coils;				// # of coils
    uint16_t u16InputRegisters;		// # of input registers
    uint16_t u16HoldingRegisters;	// # of holding registers
}TModbusSlaveDataSizeConfig;

typedef struct
{
    uint32_t u32DiscreteInputsOffset;		    // process image offset in bytes
    uint32_t u32DiscreteInputsLength;		    // data length in bits
    uint32_t u32CoilsInputOffset;			    // process image offset in bytes
    uint32_t u32CoilsLength;				    // data length in bits
    uint32_t u32InputRegistersOffset;		    // process image offset in bytes
    uint32_t u32InputRegistersLength;		    // data length in bytes
    uint32_t u32HoldingRegistersInputOffset;	// process image offset in bytes
    uint32_t u32HoldingRegistersLength;			// data length in bytes
}TProcessImageConfiguration;

typedef struct
{
    EModbusProtocol eProtocol;       // TCP or RTU
    union 
    {
        TTcpConfig tTcpConfig;
        TRtuConfig tRtuConfig;
    } uProt;
    int32_t i32uDeviceStatusByteProcessImageOffset;             //the device status byte offset in the pi process image
    int32_t i32uDeviceStatusResetByteProcessImageByteOffset;    //status reset byte offset
} TModbusDeviceConfiguration;



typedef struct
{
    TModbusDeviceConfiguration tModbusDeviceConfig;
    int32_t i32ActionCount;
    SLIST_HEAD(TMBActionListHead, TMBActionEntry) mbActionListHead; // Array of demanded actions, last element must be NULL
} TModbusMasterConfiguration;

typedef struct
{
    TModbusDeviceConfiguration tModbusDeviceConfig;
    TModbusSlaveDataSizeConfig tModbusDataConfig;
    TProcessImageConfiguration tProcessImageConfig;
} TModbusSlaveConfiguration;

struct TMBSlaveConfigEntry
{
    TModbusSlaveConfiguration mbSlaveConfig;
    SLIST_ENTRY(TMBSlaveConfigEntry) entries;
};

struct TMBMasterConfigEntry
{
    TModbusMasterConfiguration mbMasterConfig;
    SLIST_ENTRY(TMBMasterConfigEntry) entries;
};

SLIST_HEAD(TMBSlaveConfHead, TMBSlaveConfigEntry) mbSlaveConfHead;
SLIST_HEAD(TMBMasterConfHead, TMBMasterConfigEntry) mbMasterConfHead;



