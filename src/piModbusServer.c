/*
 * SPDX-FileCopyrightText: 2026 KUNBUS GmbH
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/*!
 *
 * Project: piModbusServer
 * (C)    : KUNBUS GmbH, Magirusstraße 7, 73760 Ostfildern, Germany
 *
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/queue.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <syslog.h>

#include "piModbusServer.h"
#include "ModbusServerThread.h"
#include <piControlIf.h>
#include "piConfigParser/piConfigParser.h"


int32_t main(int32_t argc, char *argv[])
{
    int i, modbusDevicesCount;
    pthread_t *pThreads = NULL;
    
    (void)argc;
    (void)argv;
    
    //open syslog
    openlog("piModbusServer", LOG_PID, LOG_DAEMON);
    setlogmask(LOG_UPTO(LOG_NOTICE));       // LOG_INFO und LOG_DEBUG Meldungen werden nicht ausgegeben.
    syslog(LOG_NOTICE, "piModbusServer started\n");

    while (1) {
        //create list for all modbus server configurations stored in pictory config file
        SLIST_INIT(&mbServerConfHead);
        //parse config data
        get_server_device_config_list(&mbServerConfHead);
        if (SLIST_EMPTY(&mbServerConfHead))
        {
            //syslog
            syslog(LOG_NOTICE, "No modbus server configuration found in config file");
            modbusDevicesCount = 0;
        }
        else
        {
            struct TMBServerConfigEntry *mbServerConfigListEntry;

            //count number of matching devices to allocate memory for pthreads
            modbusDevicesCount = 0;
            SLIST_FOREACH(mbServerConfigListEntry, &mbServerConfHead, entries)
            {
                modbusDevicesCount++;
            }
            pThreads = malloc(sizeof(pthread_t)*modbusDevicesCount);
            if (!pThreads)
                return ENOMEM;
    
            //start a thread for every matching configuration found in pictory config file
            i = 0;
            SLIST_FOREACH(mbServerConfigListEntry, &mbServerConfHead, entries)
            {
                if (mbServerConfigListEntry->mbServerConfig.tModbusDeviceConfig.eProtocol == eProtRTU)
                {
                    if (0 != pthread_create(&pThreads[i], NULL, &startRtuServerThread, (void*)&(mbServerConfigListEntry->mbServerConfig)))
                    {
                        syslog(LOG_ERR,
                            "Cannot create modbus server thread for device %s with modbus address %d\n",
                            mbServerConfigListEntry->mbServerConfig.tModbusDeviceConfig.uProt.tRtuConfig.sz8DeviceFilePath,
                            mbServerConfigListEntry->mbServerConfig.tModbusDeviceConfig.uProt.tRtuConfig.u8DeviceModbusAddress);
                        pThreads[i] = 0;
                    }
                }
                else if (mbServerConfigListEntry->mbServerConfig.tModbusDeviceConfig.eProtocol == eProtTCP)
                {
                    if (0 != pthread_create(&pThreads[i], NULL, &startTcpServerThread, (void*)&(mbServerConfigListEntry->mbServerConfig)))
                    {
                        syslog(LOG_ERR, 
                            "Cannot create modbus server thread for IP %s and Port %d\n",
                            mbServerConfigListEntry->mbServerConfig.tModbusDeviceConfig.uProt.tTcpConfig.szTcpIpAddress,
                            mbServerConfigListEntry->mbServerConfig.tModbusDeviceConfig.uProt.tTcpConfig.i32uPort);
                        pThreads[i] = 0;
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
                pthread_cancel(pThreads[i]); // NOLINT is initialized if modbusDevicesCount > 0
            }
            free(pThreads);

            while (SLIST_FIRST(&mbServerConfHead))
            {
                struct TMBServerConfigEntry *entry = SLIST_FIRST(&mbServerConfHead);
                SLIST_REMOVE_HEAD(&mbServerConfHead, entries);
                free(entry);
            }

        }
        free_config_buffer();
    }
    return 0;
}
