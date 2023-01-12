/* SPDX-License-Identifier: GPL-2.0-or-later */

/*!
 *
 * Project: piModbusSlave
 * (C)    : KUNBUS GmbH, Heerweg 15C, 73370 Denkendorf, Germany
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

#include "piModbusSlave.h"
#include "ModbusSlaveThread.h"
#include <piTest/piControlIf.h>
#include "piConfigParser/piConfigParser.h"


int32_t main(int32_t argc, char *argv[])
{
    int i, modbusDevicesCount;
    pthread_t *pThreads = NULL;
    
    (void)argc;
    (void)argv;
    
    //open syslog
    openlog("piModbusSlave", LOG_PID, LOG_DAEMON);
    setlogmask(LOG_UPTO(LOG_NOTICE));       // LOG_INFO und LOG_DEBUG Meldungen werden nicht ausgegeben.
    syslog(LOG_NOTICE, "piModbusSlave started\n");

    while (1) {
        //create list for all modbus slave configurations stored in pictory config file
        SLIST_INIT(&mbSlaveConfHead);
        //parse config data
        get_slave_device_config_list(&mbSlaveConfHead);
        if (SLIST_EMPTY(&mbSlaveConfHead))
        {
            //syslog
            syslog(LOG_NOTICE, "No modbus slave configuration found in config file");
            modbusDevicesCount = 0;
        }
        else
        {
            struct TMBSlaveConfigEntry *mbSlaveConfigListEntry;

            //count number of matching devices to allocate memory for pthreads
            modbusDevicesCount = 0;
            SLIST_FOREACH(mbSlaveConfigListEntry, &mbSlaveConfHead, entries)
            {
                modbusDevicesCount++;
            }
            pThreads = malloc(sizeof(pthread_t)*modbusDevicesCount);
            if (!pThreads)
                return ENOMEM;
    
            //start a thread for every matching configuration found in pictory config file
            i = 0;
            SLIST_FOREACH(mbSlaveConfigListEntry, &mbSlaveConfHead, entries)
            {
                if (mbSlaveConfigListEntry->mbSlaveConfig.tModbusDeviceConfig.eProtocol == eProtRTU)
                {
                    if (0 != pthread_create(&pThreads[i], NULL, &startRtuSlaveThread, (void*)&(mbSlaveConfigListEntry->mbSlaveConfig)))
                    {
                        syslog(LOG_ERR,
                            "Cannot create modbus slave thread for device %s with modbus address %d\n",
                            mbSlaveConfigListEntry->mbSlaveConfig.tModbusDeviceConfig.uProt.tRtuConfig.sz8DeviceFilePath,
                            mbSlaveConfigListEntry->mbSlaveConfig.tModbusDeviceConfig.uProt.tRtuConfig.u8DeviceModbusAddress);
                        pThreads[i] = 0;
                    }
                }
                else if (mbSlaveConfigListEntry->mbSlaveConfig.tModbusDeviceConfig.eProtocol == eProtTCP)
                {
                    if (0 != pthread_create(&pThreads[i], NULL, &startTcpSlaveThread, (void*)&(mbSlaveConfigListEntry->mbSlaveConfig)))
                    {
                        syslog(LOG_ERR, 
                            "Cannot create modbus slave thread for IP %s and Port %d\n",
                            mbSlaveConfigListEntry->mbSlaveConfig.tModbusDeviceConfig.uProt.tTcpConfig.szTcpIpAddress,
                            mbSlaveConfigListEntry->mbSlaveConfig.tModbusDeviceConfig.uProt.tTcpConfig.i32uPort);
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

            while (SLIST_FIRST(&mbSlaveConfHead))
            {
                struct TMBSlaveConfigEntry *entry = SLIST_FIRST(&mbSlaveConfHead);
                SLIST_REMOVE_HEAD(&mbSlaveConfHead, entries);
                free(entry);
            }

        }
        free_config_buffer();
    }
    return 0;
}
