/*!
 *
 * Project: piModbusMaster
 * (C)    : KUNBUS GmbH, Heerweg 15C, 73370 Denkendorf, Germany
 *
 */

#include "project.h"

#define _POSIX_C_SOURCE 200112L //clock_nanosleep and struct timespec
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include "modbusconfig.h"
#include <unistd.h>
#include "Scheduler.h"
#include "ComAndDataProcessor.h"
#include "ModbusMasterThread.h"
#include <syslog.h>

#ifndef _MSC_VER
#include <sys/time.h>
#else	
#include <time.h>
#endif

//#define MODBUS_DEBUG

static int32_t setprio(int prio, int sched)
{
    struct sched_param param;
    // Set realtime priority for this thread
    param.sched_priority = prio;
    if (sched_setscheduler(0, sched, &param) < 0)
    {
        perror("sched_setscheduler");
        return -1;
    }
    return 0;
}

const int32_t MAX_CONSECUTIVE_DELAYED_ACTIONS = 5;

void cleanupTcpMasterThread(void *ptr)
{
    modbus_t *pModbusContext = (modbus_t *)ptr;
    
    //syslog(LOG_ERR, "cleanupTcpMasterThread %p\n", ptr);
    
    if (pModbusContext)
    {
        modbus_close(pModbusContext);
        modbus_free(pModbusContext);
    }
}

void *startTcpMasterThread(void *arg)
{
    //init modbus device
    TModbusMasterConfiguration *psModbusConfiguration_l = (TModbusMasterConfiguration *)arg;
    
    //set realtime priority of the thread
    if(setprio(20, SCHED_RR) < 0)
    {
        syslog(LOG_ERR, "Set realtime priority for modbus tcp thread failed\n");
        writeErrorMessage(psModbusConfiguration_l->tModbusDeviceConfig.i32uDeviceStatusByteProcessImageOffset, (uint8_t)(eInternalError));
        return NULL;
    }
    
    
    uint8_t buffer[MAX_REGISTER_SIZE_PER_ACTION] = { 0 };  //max register size for each pictory action
    TTcpConfig *ptTcpConfig_l = &psModbusConfiguration_l->tModbusDeviceConfig.uProt.tTcpConfig;
    modbus_t *pModbusContext = NULL;
    char st8TcpPort[12];
    sprintf(st8TcpPort, "%d", psModbusConfiguration_l->tModbusDeviceConfig.uProt.tTcpConfig.i32uPort);
    
    pModbusContext = modbus_new_tcp_pi(
        psModbusConfiguration_l->tModbusDeviceConfig.uProt.tTcpConfig.szTcpIpAddress,
        st8TcpPort);
    
    if (pModbusContext == NULL)
    {
        syslog(LOG_ERR, "Unable to allocate modbus tcp context\n");
        writeErrorMessage(psModbusConfiguration_l->tModbusDeviceConfig.i32uDeviceStatusByteProcessImageOffset, (uint8_t)(eNoDevice));
        pthread_exit(0);
    }

    pthread_cleanup_push(cleanupTcpMasterThread, pModbusContext);
    //syslog(LOG_ERR, "pthread_cleanup_push %p\n", pModbusContext);
    
#ifdef STRETCH
    if(modbus_set_error_recovery(pModbusContext, MODBUS_ERROR_RECOVERY_PROTOCOL) < 0)
#else
    if(modbus_set_error_recovery(pModbusContext, MODBUS_ERROR_RECOVERY_LINK | MODBUS_ERROR_RECOVERY_PROTOCOL) < 0)
#endif
    {
        syslog(LOG_ERR, "Set Modbus error recovery mode failed: %s\n", modbus_strerror(errno));
    }
    
#ifdef MODBUS_DEBUG
    modbus_set_debug(pModbusContext, 1);
#endif
    while (1)
    {
        if (modbus_connect(pModbusContext) < 0)
        {
            struct timespec tv_sleep;
            
            syslog(LOG_ERR, "Modbus connection failed: ip=%s errno=%s\n", ptTcpConfig_l->szTcpIpAddress, modbus_strerror(errno));
            writeErrorMessage(psModbusConfiguration_l->tModbusDeviceConfig.i32uDeviceStatusByteProcessImageOffset, (uint8_t)(eNoResponseFromDevice));
            
            // wait for 5 seconds and try again
            tv_sleep.tv_sec = 5;
            tv_sleep.tv_nsec = 0;
            clock_nanosleep(CLOCK_MONOTONIC, 0, &tv_sleep, NULL);
        }
        else
        {
            //init scheduler 
            struct suEventListHead eventListHead = TAILQ_HEAD_INITIALIZER(eventListHead);
            if (initScheduler(psModbusConfiguration_l->mbActionListHead, &eventListHead) < 0)
            {
                syslog(LOG_ERR, "Scheduler initialization failed\n");
                writeErrorMessage(psModbusConfiguration_l->tModbusDeviceConfig.i32uDeviceStatusByteProcessImageOffset, (uint8_t)(eInternalError));
                pthread_exit(0);
            }
    
            //set modbus timeout values according to minimal modbus action interval
            struct timespec tv_min_interval = { 0, 0 };
            get_minimal_modbus_action_interval(&tv_min_interval, &eventListHead);
            struct timeval modbus_timeout = { 0, 0 };

            //divide the minimal interval by 2 to get an appropriate timeout value
            if (tv_min_interval.tv_sec % 2 != 0)
            {
                tv_min_interval.tv_nsec = tv_min_interval.tv_nsec + s32_nanoseconds_per_second;
            }	
            modbus_timeout.tv_sec = (tv_min_interval.tv_sec / 2);
            modbus_timeout.tv_usec = ((tv_min_interval.tv_nsec) / 1000 / 2);
            if ((modbus_timeout.tv_sec <= 0) && (modbus_timeout.tv_usec <= 0))
            {
                modbus_timeout.tv_usec = 1;
                syslog(LOG_ERR, "modbus timeout set to 1 microsecond");
            }
	        
#if LIBMODBUS_VERSION_CHECK(3,1,2)
	        modbus_set_response_timeout(pModbusContext, modbus_timeout.tv_sec, modbus_timeout.tv_usec);
	        modbus_set_byte_timeout(pModbusContext, modbus_timeout.tv_sec, modbus_timeout.tv_usec);
#else	
	        modbus_set_response_timeout(pModbusContext, &modbus_timeout);
	        modbus_set_byte_timeout(pModbusContext, &modbus_timeout);
#endif

            tModbusEvent nextEvent;	//next modbus action from scheduler
            struct timespec tv_current = { 0, 0 };
            struct timespec tv_earliest_next_trigger_time = { 0, 0 };
            struct timespec tv_minimal_event_offset = { 0, 0 };
            get_minimal_modbus_event_offset(&tv_minimal_event_offset, &eventListHead);

            int32_t ret_val_modbus_action = 0;
            int32_t err_cnt = 0;
    
            //for debug: calculate delay
            //int32_t delayedActions = 0;
            //struct timespec tv_tmp = { 0, 0 };	
            syslog(LOG_INFO, "Modbus connection established to ip=%s port=%d\n", ptTcpConfig_l->szTcpIpAddress, ptTcpConfig_l->i32uPort);
            writeErrorMessage(psModbusConfiguration_l->tModbusDeviceConfig.i32uDeviceStatusByteProcessImageOffset, (uint8_t)(eNoError));
        
            while (1)
            {
                getNextEvent(&nextEvent, &eventListHead);

                //check if reset status is set and reset status if neccessarry
                ret_val_modbus_action = reset_modbus_action_status(
                    nextEvent.ptModbusAction->i32uResetStatusProcessImageByteOffset,
                    nextEvent.ptModbusAction->i8uResetStatusProcessImageBitOffset,
                    nextEvent.ptModbusAction->i32uStatusByteProcessImageOffset);
        
                ret_val_modbus_action = reset_modbus_master_status(
                    psModbusConfiguration_l->tModbusDeviceConfig.i32uDeviceStatusResetByteProcessImageByteOffset,
                    psModbusConfiguration_l->tModbusDeviceConfig.i32uDeviceStatusByteProcessImageOffset);

#if 0
                //calculate delay to check if next modbus command is overdue and print a message
                clock_gettime(CLOCK_MONOTONIC, &tv_current);
                if (timespec_diff(&tv_tmp, &(nextEvent.triggerTime), &tv_current) < 0)
                {
                    if (((-1)*(((int32_t)(tv_tmp.tv_sec) * 1000 * 1000) + (int32_t)(tv_tmp.tv_nsec / 1000))) > ((int32_t)(tv_min_interval.tv_sec * 1000 * 1000) + (int32_t)(tv_min_interval.tv_nsec / 1000)))
                    {
                        delayedActions++;
#if 1
                        syslog(LOG_ERR,
                            "modbus action backlog. Delay is %d milliseconds\n consecutive delayed actions: %d\n",
                            (-1)*(((int32_t)(tv_tmp.tv_sec) * 1000) + (int32_t)(tv_tmp.tv_nsec / 1000 / 1000)),
                            delayedActions);
#endif
                        if (delayedActions > MAX_CONSECUTIVE_DELAYED_ACTIONS)
                        {
                            writeErrorMessage(psModbusConfiguration_l->tModbusDeviceConfig.i32uDeviceStatusByteProcessImageOffset, (uint8_t)(eModbusActionBacklog));
                        }			
                    }
                }
                else
                {
                    delayedActions = 0;
                }
#endif
        
                //sleep until absolute system time specified by nextEvent.triggerTime is reached
                clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &(nextEvent.triggerTime), NULL);
        
                //additional sleep if tv_earliest_next_trigger_time is not yet overdue
                clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &tv_earliest_next_trigger_time, NULL);
        
                //set the modbus slave address for the next command
                if (modbus_set_slave(pModbusContext, nextEvent.ptModbusAction->i8uSlaveAddress) < 0)
                {
                    syslog(LOG_ERR, "Set Modbus slave address for next command failed: %s\n", modbus_strerror(errno));
                }
        
                ret_val_modbus_action = processModbusAction(pModbusContext, &nextEvent, buffer);

                //store earliest next trigger time for next event
                clock_gettime(CLOCK_MONOTONIC, &tv_current);
                timespec_add(&tv_earliest_next_trigger_time, &tv_current, &tv_minimal_event_offset);
        
                if (ret_val_modbus_action < 0)
                {
                    syslog(LOG_ERR,
                        "Modbus TCP action IP: %s, Port: %d function: 0x%02X, address: %d failed %d/%d\n",
                        ptTcpConfig_l->szTcpIpAddress,
                        ptTcpConfig_l->i32uPort,
                        (int32_t)nextEvent.ptModbusAction->eFunctionCode,
                        (int32_t)nextEvent.ptModbusAction->i32uStartRegister,
                        ret_val_modbus_action,
                        errno);
                    err_cnt++;
                    if (err_cnt > 100)
                    {
                        syslog(LOG_ERR,
                            "Modbus TCP IP: %s, Port %d: too many errors -> restart\n",
                            ptTcpConfig_l->szTcpIpAddress,
                            ptTcpConfig_l->i32uPort);
                       
                        modbus_close(pModbusContext);
                        break;
                    }
#if 0
                    //a modbus timeout could lead to delay times, which exceeds the idle time to the next modbus action.
                    //This results in subsequent timeouts therefore the action list(scheduler) reset is performed		
                    if (initScheduler(psModbusConfiguration_l->mbActionListHead, &eventListHead) < 0)
                    {
                        syslog(LOG_ERR, "reset scheduler after modbus timeout failed\n");
                        pthread_exit(0);
                    }
                    else
                    {
                        syslog(LOG_ERR, "reset scheduler\n");
                    }
#endif
                }
                else
                {
#if 0
                    syslog(LOG_ERR,
                        "processModbusAction returned %d.   %d:%d\n",
                        ret_val_modbus_action,
                        (int)tv_earliest_next_trigger_time.tv_sec,
                        (int)(tv_earliest_next_trigger_time.tv_nsec / 1000000));
#endif
                    err_cnt = 0;                    
                }
            }
        }
    }
    pthread_cleanup_pop(1);
    return NULL;    
}




void cleanupRtuMasterThread(void *ptr)
{
    modbus_t *pModbusContext = (modbus_t *)ptr;
    
    //syslog(LOG_ERR, "cleanupRtuMasterThread %p\n", ptr);
    
    if(pModbusContext)
    {
        modbus_close(pModbusContext);
        modbus_free(pModbusContext);
    }
}


void *startRtuMasterThread(void *arg)
{
    //init modbus device
    TModbusMasterConfiguration *psModbusConfiguration_l = (TModbusMasterConfiguration *)arg;
    int logRtuPath = 0;

    /* Wait for serial device getting ready(Readable, Writable) */
    while(access(psModbusConfiguration_l->tModbusDeviceConfig.uProt.tRtuConfig.sz8DeviceFilePath,
                                                            R_OK | W_OK)) {
        if (!logRtuPath) {
            syslog(LOG_INFO, "RTU Master waiting for serial device:%s\n",
                psModbusConfiguration_l->tModbusDeviceConfig.uProt.tRtuConfig.sz8DeviceFilePath);
            logRtuPath = 1;
        }
        /* Repeat checking in 1 Sec */
        sleep(1);
    }
    syslog(LOG_INFO, "RTU Master got serial device:%s\n",
        psModbusConfiguration_l->tModbusDeviceConfig.uProt.tRtuConfig.sz8DeviceFilePath);

    //set realtime priority of the thread
    if(setprio(20, SCHED_RR) < 0)
    {
        syslog(LOG_ERR, "Set realtime priority for modbus rtu thread failed\n");
        writeErrorMessage(psModbusConfiguration_l->tModbusDeviceConfig.i32uDeviceStatusByteProcessImageOffset, (uint8_t)(eInternalError));
        return NULL;
    }
    
    
    uint8_t buffer[MAX_REGISTER_SIZE_PER_ACTION] = { 0 };  //max register size for each pictory action
    TRtuConfig *ptRtuConfig_l = &psModbusConfiguration_l->tModbusDeviceConfig.uProt.tRtuConfig;
    modbus_t *pModbusContext = NULL; 
    char st8TcpPort[12];
    sprintf(st8TcpPort, "%d", psModbusConfiguration_l->tModbusDeviceConfig.uProt.tTcpConfig.i32uPort);
    
    pModbusContext = modbus_new_rtu(
        ptRtuConfig_l->sz8DeviceFilePath,
        ptRtuConfig_l->i32uBaud,
        ptRtuConfig_l->cParity,
        ptRtuConfig_l->i8uDatabits,
        ptRtuConfig_l->i8uStopbits);
    
    if (pModbusContext == NULL)
    {
        syslog(LOG_ERR, "Unable to allocate modbus rtu context\n");
        writeErrorMessage(psModbusConfiguration_l->tModbusDeviceConfig.i32uDeviceStatusByteProcessImageOffset, (uint8_t)(eNoDevice));
        return NULL;
    }
    if (modbus_set_error_recovery(pModbusContext, MODBUS_ERROR_RECOVERY_LINK | MODBUS_ERROR_RECOVERY_PROTOCOL) < 0)
    {
        syslog(LOG_ERR, "Set Modbus error recovery mode failed: %s\n", modbus_strerror(errno));
    }
        
#ifdef MODBUS_DEBUG
    modbus_set_debug(pModbusContext, 1);
#endif
    if (modbus_connect(pModbusContext) < 0)
    {
        syslog(LOG_ERR, "Modbus connection failed: %s\n", modbus_strerror(errno));
        modbus_free(pModbusContext);
        writeErrorMessage(psModbusConfiguration_l->tModbusDeviceConfig.i32uDeviceStatusByteProcessImageOffset, (uint8_t)(eNoResponseFromDevice));
        return NULL;
    }

#if 0    
    // the call make no sense because the used ioctl-call is not implmented in most serial drivers.
    if (modbus_rtu_set_serial_mode(pModbusContext, MODBUS_RTU_RS485) < 0)
    {
        syslog(LOG_ERR, "Set Modbus serial mode failed: %s\n", modbus_strerror(errno));
    }
#endif
    pthread_cleanup_push(cleanupRtuMasterThread, pModbusContext);
    //syslog(LOG_ERR, "pthread_cleanup_push %p\n", pModbusContext);
    

    //init scheduler
    struct suEventListHead eventListHead = TAILQ_HEAD_INITIALIZER(eventListHead);
    if(initScheduler(psModbusConfiguration_l->mbActionListHead, &eventListHead) < 0)
    {
        syslog(LOG_ERR, "Scheduler initialization failed\n");
        modbus_close(pModbusContext);
        modbus_free(pModbusContext);
        writeErrorMessage(psModbusConfiguration_l->tModbusDeviceConfig.i32uDeviceStatusByteProcessImageOffset, (uint8_t)(eInternalError));
        pthread_exit(0);
    }
    
    
    //set modbus timeout values according to minimal modbus action interval
    struct timespec tv_min_interval = { 0, 0 };
    get_minimal_modbus_action_interval(&tv_min_interval, &eventListHead);
    struct timeval modbus_timeout = { 0, 0 };
    //divide the minimal interval by 2 to get an appropriate timeout value
    if (tv_min_interval.tv_sec % 2 != 0)
    {
        tv_min_interval.tv_nsec = tv_min_interval.tv_nsec + s32_nanoseconds_per_second;
    }	
    modbus_timeout.tv_sec = (tv_min_interval.tv_sec / 2);
    modbus_timeout.tv_usec = ((tv_min_interval.tv_nsec) / 1000 / 2);
    if((modbus_timeout.tv_sec <= 0) && (modbus_timeout.tv_usec <= 0))
    {
        modbus_timeout.tv_usec = 1;
        syslog(LOG_ERR, "modbus timeout set to 1 microsecond");
    }
#if LIBMODBUS_VERSION_CHECK(3,1,2)
	modbus_set_response_timeout(pModbusContext, modbus_timeout.tv_sec, modbus_timeout.tv_usec);
	modbus_set_byte_timeout(pModbusContext, modbus_timeout.tv_sec, modbus_timeout.tv_usec);	
#else	
	modbus_set_response_timeout(pModbusContext, &modbus_timeout);
	modbus_set_byte_timeout(pModbusContext, &modbus_timeout);
#endif	

    syslog(LOG_ERR,
        "modbus rtu action timeout: %d s %d us\n",
        (int)modbus_timeout.tv_sec,
        (int)modbus_timeout.tv_usec);
    
    tModbusEvent nextEvent;	//next modbus action from scheduler
    struct timespec tv_current = { 0, 0 };
    struct timespec tv_earliest_next_trigger_time = { 0, 0 };
    struct timespec tv_minimal_event_offset = { 0, 0 };
    //get_minimal_modbus_event_offset(&tv_minimal_event_offset, &eventListHead);
    get_minimal_modbus_action_interval(&tv_minimal_event_offset, &eventListHead);
    // devide the minimal interval by the number of actions and by 4
    // then the time between the transmissions is one quarter of the whole transfer
    uint64_t minoffset = tv_minimal_event_offset.tv_sec;
    minoffset *= s32_nanoseconds_per_second;
    minoffset += tv_minimal_event_offset.tv_nsec;           // convert to nsec
    minoffset /= psModbusConfiguration_l->i32ActionCount;   // divide by number of actions
    minoffset /= 4;                                         // divide by 4
    tv_minimal_event_offset.tv_sec = minoffset / s32_nanoseconds_per_second;
    tv_minimal_event_offset.tv_nsec = minoffset % s32_nanoseconds_per_second;
    
    syslog(LOG_ERR,
        "modbus rtu minimal time between telegrams: %d s %d us\n",
        (int)tv_minimal_event_offset.tv_sec,
        (int)(tv_minimal_event_offset.tv_nsec / 1000));

#if 0
    // set to 1 sec for Berg power meters
    tv_minimal_event_offset.tv_sec = 1;
    tv_minimal_event_offset.tv_nsec = 0 * 1000 * 1000;
    
    syslog(LOG_ERR,
        "modbus rtu minimal time between telegrams forced to: %d s %d us\n",
        (int)tv_minimal_event_offset.tv_sec,
        (int)(tv_minimal_event_offset.tv_nsec / 1000));
#endif
    
    int32_t ret_val_modbus_action = 0;
    writeErrorMessage(psModbusConfiguration_l->tModbusDeviceConfig.i32uDeviceStatusByteProcessImageOffset, (uint8_t)(eNoError));
    
    //for debug: calculate delay
    //int32_t delayedActions = 0;
    //struct timespec tv_tmp = { 0, 0 };	
    
    while (1)
    {
        getNextEvent(&nextEvent, &eventListHead);

#if 0
        //check, if reset status is set for this action and reset status if neccessarry
        ret_val_modbus_action = reset_modbus_action_status(
            nextEvent.ptModbusAction->i32uResetStatusProcessImageByteOffset,
            nextEvent.ptModbusAction->i8uResetStatusProcessImageBitOffset,
            nextEvent.ptModbusAction->i32uStatusByteProcessImageOffset);
#else
        {
            //check, if reset status is set for ANY action and reset status if neccessarry
            struct schedulerEvent* pEvent = NULL;
            TAILQ_FOREACH(pEvent, &eventListHead, events)
            {
                TModbusAction *pModbusAction = pEvent->ptModbusAction;
                reset_modbus_action_status(pModbusAction->i32uResetStatusProcessImageByteOffset,
                    pModbusAction->i8uResetStatusProcessImageBitOffset,
                    pModbusAction->i32uStatusByteProcessImageOffset);
            }
        }
#endif        
        
        ret_val_modbus_action = reset_modbus_master_status(
            psModbusConfiguration_l->tModbusDeviceConfig.i32uDeviceStatusResetByteProcessImageByteOffset,
            psModbusConfiguration_l->tModbusDeviceConfig.i32uDeviceStatusByteProcessImageOffset);

#if 0
        //calculate delay to check if next modbus command is overdue
        clock_gettime(CLOCK_MONOTONIC, &tv_current);
        if (timespec_diff(&tv_tmp, &(nextEvent.triggerTime), &tv_current) < 0)
        {
            if ( ((-1)*(((int32_t)(tv_tmp.tv_sec) *1000*1000) + (int32_t)(tv_tmp.tv_nsec / 1000))) > ((int32_t)(tv_min_interval.tv_sec *1000*1000) +(int32_t)(tv_min_interval.tv_nsec/1000)) )
            {
                delayedActions++;
#if 1
                syslog(LOG_ERR, "modbus action backlog. Delay is %d milliseconds\n consecutive delayed actions: %d\n",
                    (-1)*(((int32_t)(tv_tmp.tv_sec) * 1000) + (int32_t)(tv_tmp.tv_nsec / 1000 / 1000)), delayedActions);
#endif
                if (delayedActions > MAX_CONSECUTIVE_DELAYED_ACTIONS)
                {
                    writeErrorMessage(psModbusConfiguration_l->tModbusDeviceConfig.i32uDeviceStatusByteProcessImageOffset, (uint8_t)(eModbusActionBacklog));
                }			
            }
            
        }
        else
        {
            delayedActions = 0;
        }
#endif
        
        //sleep until absolute system time specified by nextEvent.triggerTime is reached
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &(nextEvent.triggerTime), NULL);
    
        //additional sleep if tv_earliest_next_trigger_time is not yet overdue
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &tv_earliest_next_trigger_time, NULL);
        
        //set the modbus slave address for the next command
        if(modbus_set_slave(pModbusContext, nextEvent.ptModbusAction->i8uSlaveAddress) < 0)
        {
            syslog(LOG_ERR, "Set Modbus slave address for next command failed: %s\n", modbus_strerror(errno));
        }
        
        ret_val_modbus_action = processModbusAction(pModbusContext, &nextEvent, buffer);
        
        //store earliest next trigger time for next event
        clock_gettime(CLOCK_MONOTONIC, &tv_current);
        timespec_add(&tv_earliest_next_trigger_time, &tv_current, &tv_minimal_event_offset);	
        
        if (ret_val_modbus_action < 0)
        {
            syslog(LOG_ERR,
                "modbus rtu action device: %s, slave address: %d function: 0x%02X, address: %d failed %d/%d/%d\n",
                ptRtuConfig_l->sz8DeviceFilePath,
                (int32_t)nextEvent.ptModbusAction->i8uSlaveAddress,
                (int32_t)nextEvent.ptModbusAction->eFunctionCode,
                (int32_t)nextEvent.ptModbusAction->i32uStartRegister,
                ret_val_modbus_action,
                errno,
                errno-MODBUS_ENOBASE);
            
#if 0
            //a modbus timeout could lead to delay times, which exceeds the idle time to the next modbus action.
            //This results in subsequent timeouts therefore the action list(scheduler) reset is performed		
            if(initScheduler(psModbusConfiguration_l->mbActionListHead, &eventListHead) < 0)
            {
                syslog(LOG_ERR, "reset scheduler after modbus timeout failed\n");
                modbus_close(pModbusContext);
                modbus_free(pModbusContext);
                return NULL;
            }
            else
            {
                syslog(LOG_ERR, "reset scheduler\n");
            }
#endif
        }
#if 0 //def MODBUS_DEBUG
        else
        {
            syslog(LOG_ERR,
                "modbus rtu action device: %s, slave address: %d function: 0x%02X, address: %d succeeded\n",
                ptRtuConfig_l->sz8DeviceFilePath,
                (int32_t)nextEvent.ptModbusAction->i8uSlaveAddress,
                (int32_t)nextEvent.ptModbusAction->eFunctionCode,
                (int32_t)nextEvent.ptModbusAction->i32uStartRegister);
        }
#endif
    }
    pthread_cleanup_pop(1);
    return NULL;
}

