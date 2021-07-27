/*!
 *
 * Project: piModbusSlave
 * (C)    : KUNBUS GmbH, Heerweg 15C, 73370 Denkendorf, Germany
 *
 */

#include "project.h"

#define _POSIX_C_SOURCE 200112L //clock_nanosleep and struct timespec
#include <time.h>
#include "ComAndDataProcessor.h"
#include "modbusconfig.h"
#include <stdio.h>
#include <errno.h>
#include <piTest/piControlIf.h>
#include <sys/param.h>
#include <assert.h>
#include <syslog.h>
#include <pthread.h>

#ifndef MODBUS_MAX_PDU_LENGTH 
#define MODBUS_MAX_PDU_LENGTH 253
#endif

pthread_mutex_t mutex_master_instance = PTHREAD_MUTEX_INITIALIZER;

/************************************************************************/
/** @ brief processing the given modbus action and updates the process image
 *  
 *  
 *  @param[in] pModbusContext the pointer to the libmodus device
 *  @param[in] nextEvent the modbus action which has to be processed
 *  @return return value of modbus function if 
 *
 *	data from/to modbus is stored in a buffer. Reading/Writing from/to the
 *	process image is done individual for every modbus action
 *	
 */
/************************************************************************/
int32_t processModbusAction(modbus_t *pModbusContext, tModbusEvent* mb_event, uint8_t *buffer)
{
    int32_t len = 0;
    int32_t successful = 0;
    // use a mutex since modbus_ctx is not thread safe
    pthread_mutex_lock(&mutex_master_instance);

    //process modbus action
    switch ((mb_event->ptModbusAction->eFunctionCode))
    {
    case eREAD_COILS:
        {
            assert(mb_event->ptModbusAction->i16uRegisterCount <= MODBUS_MAX_READ_BITS);
            len = modbus_read_bits(pModbusContext,
                mb_event->ptModbusAction->i32uStartRegister - MODBUS_ADDRESS_OFFSET,
                mb_event->ptModbusAction->i16uRegisterCount,
                (uint8_t*)buffer);
            if (len < mb_event->ptModbusAction->i16uRegisterCount)
            {
                break;
            }
            for(int32_t i = 0; i < (mb_event->ptModbusAction->i16uRegisterCount); i++)
            {
                SPIValue coil_data_l;
                coil_data_l.i16uAddress = (uint16_t)(mb_event->ptModbusAction->i32uStartByteProcessData + (i / 8));
                coil_data_l.i8uBit      = ((mb_event->ptModbusAction->i8uStartBitProcessData)+i) % 8;
                coil_data_l.i8uValue    = buffer[i];
                successful = piControlSetBitValue(&coil_data_l);
                if (successful < 0)
                {
                    syslog(LOG_ERR, "write to process image failed: %d\n", successful);
                    break;
                }
            }		
        }   break;
            
    case eREAD_DISCRETE_INPUTS:
        {
            assert(mb_event->ptModbusAction->i16uRegisterCount <= MODBUS_MAX_READ_BITS);
            len = modbus_read_input_bits(pModbusContext,
                mb_event->ptModbusAction->i32uStartRegister - MODBUS_ADDRESS_OFFSET,
                mb_event->ptModbusAction->i16uRegisterCount,
                (uint8_t*)buffer);
            if (len < mb_event->ptModbusAction->i16uRegisterCount)
            {
                break;
            }
            for (int32_t i = 0; i < (mb_event->ptModbusAction->i16uRegisterCount); i++)
            {
                SPIValue coil_data_l;
                coil_data_l.i16uAddress = (uint16_t)(mb_event->ptModbusAction->i32uStartByteProcessData + (i / 8));
                coil_data_l.i8uBit      = ((mb_event->ptModbusAction->i8uStartBitProcessData) + i) % 8;
                coil_data_l.i8uValue    = buffer[i];
                successful = piControlSetBitValue(&coil_data_l);
                if (successful < 0)
                {
                    syslog(LOG_ERR, "write to process image failed: %d\n", successful);
                    break;
                }
            }		
        }   break;
        
    case eREAD_HOLDING_REGISTERS:
        {
            assert(mb_event->ptModbusAction->i16uRegisterCount <= MODBUS_MAX_READ_REGISTERS);
            len = modbus_read_registers(pModbusContext,
                mb_event->ptModbusAction->i32uStartRegister - MODBUS_ADDRESS_OFFSET,
                mb_event->ptModbusAction->i16uRegisterCount,
                (uint16_t*)buffer);
            if (len < mb_event->ptModbusAction->i16uRegisterCount)
            {
                break;
            }
            successful = piControlWrite(mb_event->ptModbusAction->i32uStartByteProcessData,
                ((uint32_t)(mb_event->ptModbusAction->i16uRegisterCount << 1)),
                buffer);
            if (successful < 0)
            {
                syslog(LOG_ERR, "write to process image failed: %d\n", successful);
                break;
            }
        }   break;
                
    case eREAD_INPUT_REGISTERS:
        {
            assert(mb_event->ptModbusAction->i16uRegisterCount <= MODBUS_MAX_READ_REGISTERS);
            len = modbus_read_input_registers(pModbusContext,
                mb_event->ptModbusAction->i32uStartRegister - MODBUS_ADDRESS_OFFSET,
                mb_event->ptModbusAction->i16uRegisterCount,
                (uint16_t*)buffer);
            if (len < mb_event->ptModbusAction->i16uRegisterCount)
            {
                break;
            }
            successful = piControlWrite(mb_event->ptModbusAction->i32uStartByteProcessData,
                ((uint32_t)(mb_event->ptModbusAction->i16uRegisterCount << 1)),
                buffer);
            if (successful < 0)
            {
                syslog(LOG_ERR, "write to process image failed: %d\n", successful);
                break;
            }
        }   break;
        
    case eWRITE_SINGLE_REGISTER:
        {
            assert(mb_event->ptModbusAction->i16uRegisterCount == 1);
            successful = piControlRead(mb_event->ptModbusAction->i32uStartByteProcessData, mb_event->ptModbusAction->i16uRegisterCount << 1, (uint8_t*)buffer);	
            if (successful <= 0)
            {
                syslog(LOG_ERR, "read from process image failed: %d\n", successful);
                break;
            }
            if (successful >= 0)
            {
                int val = buffer[0] + (buffer[1] << 8); // little endian
                
                len = modbus_write_register(pModbusContext,
                    mb_event->ptModbusAction->i32uStartRegister - MODBUS_ADDRESS_OFFSET,
                    val);
            }
        }   break;
            
    case eREAD_EXCEPTION_STATUS:
        {
            //not implemented in libmodbus
            syslog(LOG_ERR, "Unknown Modbus function: %d\n", (int8_t)(mb_event->ptModbusAction->eFunctionCode));
        }   break;
            
    case eWRITE_SINGLE_COIL:
        {
            assert(mb_event->ptModbusAction->i16uRegisterCount == 1);
            for (int32_t i = 0; i < (mb_event->ptModbusAction->i16uRegisterCount); i++)
            {
                SPIValue coil_data_l;
                coil_data_l.i16uAddress = (uint16_t)(mb_event->ptModbusAction->i32uStartByteProcessData + (i / 8));
                coil_data_l.i8uBit      = ((mb_event->ptModbusAction->i8uStartBitProcessData) + i) % 8;
                coil_data_l.i8uValue    = 0;
                successful = piControlGetBitValue(&coil_data_l);
                if (successful < 0)
                {
                    syslog(LOG_ERR, "read to process image failed: %d\n", successful);
                    break;
                }
                buffer[i] = coil_data_l.i8uValue;
            }
            if (successful >= 0)
            {
                len = modbus_write_bit(pModbusContext,
                    mb_event->ptModbusAction->i32uStartRegister - MODBUS_ADDRESS_OFFSET,
                    buffer[0]);
            }

        }   break;
            
    case eWRITE_MULTIPLE_COILS:
        {
            assert(mb_event->ptModbusAction->i16uRegisterCount <= MODBUS_MAX_WRITE_BITS);
            for (int32_t i = 0; i < (mb_event->ptModbusAction->i16uRegisterCount); i++)
            {
                SPIValue coil_data_l;
                coil_data_l.i16uAddress = (uint16_t)(mb_event->ptModbusAction->i32uStartByteProcessData + (i / 8));
                coil_data_l.i8uBit      = ((mb_event->ptModbusAction->i8uStartBitProcessData) + i) % 8;
                coil_data_l.i8uValue    = 0;
                successful = piControlGetBitValue(&coil_data_l);
                if (successful < 0)
                {
                    syslog(LOG_ERR, "read from process image failed: %d\n", successful);
                    break;
                }
                buffer[i] = coil_data_l.i8uValue;
            }
            if (successful >= 0)
            {
                len = modbus_write_bits(pModbusContext,
                    mb_event->ptModbusAction->i32uStartRegister - MODBUS_ADDRESS_OFFSET,
                    mb_event->ptModbusAction->i16uRegisterCount,
                    (uint8_t*)buffer);
            }

        }   break;
            
    case eWRITE_MULTIPLE_REGISTERS:
        {
            assert(mb_event->ptModbusAction->i16uRegisterCount <= MODBUS_MAX_WRITE_REGISTERS);
            successful = piControlRead(mb_event->ptModbusAction->i32uStartByteProcessData, mb_event->ptModbusAction->i16uRegisterCount << 1, (uint8_t*)buffer);	
            if (successful <= 0)
            {
                syslog(LOG_ERR, "read from process image failed: %d\n", successful);
                break;
            }
            if (successful >= 0)
            {
                len = modbus_write_registers(pModbusContext,
                    mb_event->ptModbusAction->i32uStartRegister - MODBUS_ADDRESS_OFFSET,
                    mb_event->ptModbusAction->i16uRegisterCount,
                    (uint16_t*)buffer);
            }
        }   break;
            
    case eREPORT_SLAVE_ID:
        {
            assert(mb_event->ptModbusAction->i16uRegisterCount <= MODBUS_MAX_PDU_LENGTH);
	        
#if LIBMODBUS_VERSION_CHECK(3,1,2)
	        len = modbus_report_slave_id(pModbusContext, MAX_MODBUS_READ_COILS_COUNT, (uint8_t*)buffer);
#else	
	        len = modbus_report_slave_id(pModbusContext, (uint8_t*)buffer);
#endif

            if (len < ((mb_event->ptModbusAction->i16uRegisterCount) ))
            {
                break;
            }
            successful = piControlWrite(mb_event->ptModbusAction->i32uStartByteProcessData,
                ((uint32_t)(mb_event->ptModbusAction->i16uRegisterCount)),
                buffer);
            if (successful < 0)
            {
                syslog(LOG_ERR, "write to process image failed: %d\n", successful);
                break;
            }
        }   break;
            
    default:
        {
            syslog(LOG_ERR, "Unknown Modbus function: %d\n", (int8_t)(mb_event->ptModbusAction->eFunctionCode));				
        }   break;
    }

    if (successful < 0)
    {
        struct timespec tv_sleep;
            
        // wait for 100 milliseconds and try again
        tv_sleep.tv_sec = 0;
        tv_sleep.tv_nsec = 100*1000*1000;
        clock_nanosleep(CLOCK_MONOTONIC, 0, &tv_sleep, NULL);
        
    }
    
    if (len < 0)
    {
        uint32_t err = errno;
        // reduce error messages syslog(LOG_ERR, "Modbus function error: %d %d %s\n", err, err - MODBUS_ENOBASE, modbus_strerror(err));
        
        // Errnos from the modbus lib are bigger than MODBUS_ENOBASE but we have only 1 byte in the process image.
        // Subtract MODBUS_ENOBASE to get a number between 1 and 15. These number could also occurr as regular errno,
        // but these number should never occur in this application.
        if (err > MODBUS_ENOBASE)
            err -= MODBUS_ENOBASE;
        
        // write error code to process image
        writeErrorMessage(mb_event->ptModbusAction->i32uStatusByteProcessImageOffset , (uint8_t)(err));
    }
    
    
    pthread_mutex_unlock(&mutex_master_instance);
    return len;
}


/************************************************************************/
/** @ brief writes error message to processImage
 *  
 *  @param[in] the pi process image offset for the status
 *	@param[in] the Modbus or Device error code
 *  @return value > 0 if error was successful written
 *  
 */
/************************************************************************/
int32_t writeErrorMessage(uint32_t status_byte_pi_offset_p, uint8_t modbus_error_code_p)
{
    int32_t successful = -1;
    successful = piControlWrite(status_byte_pi_offset_p, 1,	(uint8_t*)&(modbus_error_code_p));
    return successful;
}

/************************************************************************/
/** @ brief check status reset bit and reset status if neccessarry
 *  
 *  @param[in] the pi process image reset byte offset
 *  @param[in] the pi process image reset bit offset
 *	@param[in] the pi process image status offset
 *  @return value >= 0 if successful, otherwise a negative value
 *  
 */
/************************************************************************/
int32_t reset_modbus_action_status(uint32_t status_reset_byte_offset_p, uint8_t status_reset_bit_offset_p, uint32_t status_byte_pi_offset_p)
{
    int32_t successful = -1;
    SPIValue reset_data_l;
    reset_data_l.i16uAddress = (uint16_t)(status_reset_byte_offset_p);
    reset_data_l.i8uBit      = status_reset_bit_offset_p;
    reset_data_l.i8uValue    = 0;
    uint8_t reset_data = 0;
    successful = piControlGetBitValue(&reset_data_l);
    if (successful < 0)
    {
        syslog(LOG_ERR, "read from process image failed: %d\n", successful);
        return successful;
    }
    if (reset_data_l.i8uValue)
    {
        successful = piControlWrite(status_byte_pi_offset_p, (uint32_t)1, &(reset_data));
        if (successful < 0)
        {
            syslog(LOG_ERR, "write to process image failed: %d\n", successful);
            return successful;
        }
        reset_data_l.i8uValue = 0;
        successful = piControlSetBitValue(&reset_data_l);
        if (successful < 0)
        {
            syslog(LOG_ERR, "read from process image failed: %d\n", successful);
            return successful;
        }
    }
    return successful;
}

/************************************************************************/
/** @ brief check status reset bit and reset status if neccessarry
 *  
 *
 *  @param[in] the pi process image reset byte offset
 *	@param[in] the pi process image status offset
 *  @return value >= 0 if successful, otherwise a negative value
 *  
 */
/************************************************************************/
int32_t reset_modbus_master_status(uint32_t status_reset_byte_offset_p, uint32_t status_byte_offset_p)
{
    return reset_modbus_action_status(status_reset_byte_offset_p, 0, status_byte_offset_p);
}
