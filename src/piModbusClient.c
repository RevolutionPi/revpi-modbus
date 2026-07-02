/* SPDX-License-Identifier: GPL-2.0-or-later */

/*
 * SPDX-FileCopyrightText: 2023-2026 KUNBUS GmbH
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <syslog.h>
#include <unistd.h>
#include <stdbool.h>

//#include <common_define.h>
#include "project.h"
#include "modbusconfig.h"

#include "ModbusClientThread.h"
#include "piConfigParser/piConfigParser.h"
#include <piControlIf.h>

#define DEBUG_MODE



int main(int argc, char *argv[])
{
    int i, modbusDevicesCount;
    pthread_t *pThreads = NULL;
    struct TMBClientConfigEntry *mbClientConfigListEntry;

    (void)argc;
    (void)argv;
    
    STDERR = stdout;    // show all messages on stdout

    //open syslog
    openlog("piModbusClient", LOG_PID, LOG_DAEMON);
    syslog(LOG_NOTICE, "piModbusClient started\n");
    
    while (1) {
        //create list for all modbus client configurations stored in pictory config file
        SLIST_INIT(&mbClientConfHead);
    
        //parse config data
        get_client_device_config_list(&mbClientConfHead);
    
        if (SLIST_EMPTY(&mbClientConfHead))
        {
            syslog(LOG_ERR, "No modbus client configuration found in config file");
            modbusDevicesCount = 0;
        }
        else
        {
            modbusDevicesCount = 0;
            //count number of matching devices to allocate memory for pthreads
            SLIST_FOREACH(mbClientConfigListEntry, &mbClientConfHead, entries)
            {
                modbusDevicesCount++;
            }
            pThreads = calloc(modbusDevicesCount, sizeof(pthread_t));
        
            //start a thread for every matching configuration found in pictory config file
            i = 0;
            SLIST_FOREACH(mbClientConfigListEntry, &mbClientConfHead, entries)
            {
                pThreads[i] = 0;
                if (mbClientConfigListEntry->mbClientConfig.tModbusDeviceConfig.eProtocol == eProtRTU)
                {
                    if (0 != pthread_create(&pThreads[i], NULL, &startRtuClientThread, (void*)&(mbClientConfigListEntry->mbClientConfig)))
                    {
                        syslog(LOG_ERR,
                            "Cannot create modbus client thread for device %s\n",
                            mbClientConfigListEntry->mbClientConfig.tModbusDeviceConfig.uProt.tRtuConfig.sz8DeviceFilePath);
                    }
                }
                else if (mbClientConfigListEntry->mbClientConfig.tModbusDeviceConfig.eProtocol == eProtTCP)
                {
                    if (0 != pthread_create(&pThreads[i], NULL, &startTcpClientThread, (void*)&(mbClientConfigListEntry->mbClientConfig)))
                    {
                        syslog(LOG_ERR,
                            "Cannot create modbus client thread for IP %s and Port %d\n",
                            mbClientConfigListEntry->mbClientConfig.tModbusDeviceConfig.uProt.tTcpConfig.szTcpIpAddress,
                            mbClientConfigListEntry->mbClientConfig.tModbusDeviceConfig.uProt.tTcpConfig.i32uPort);
                    }
                }
                else
                {
                    syslog(LOG_ERR, "Unknown modbus protocol");
                }
                i++;
            }
        }
        
        int event;
        do {
            event = piControlWaitForEvent();
        } while (event != KB_EVENT_RESET);
        
        if (modbusDevicesCount > 0)
        {
            for (i = 0; i < modbusDevicesCount; i++) {
                if (pThreads[i])
                    pthread_cancel(pThreads[i]);
                else
                    syslog(LOG_ERR, "thread %d not started", i);
            }
            free(pThreads);

            free_modbus_client_config_data(&mbClientConfHead);
        }
        free_config_buffer();
    } while (1);
    
    closelog();	//close syslog
    
    return 0;
}
