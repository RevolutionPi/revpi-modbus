/*
 * SPDX-FileCopyrightText: 2023-2026 KUNBUS GmbH
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/*!
 *
 * Project: piModbusServer
 * (C)    : KUNBUS GmbH, Magirusstraße 7, 73760 Ostfildern, Germany
 *
 */

#define _POSIX_C_SOURCE 200112L //clock_nanosleep and struct timespec
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <pthread.h>
#include <syslog.h>

#include "ModbusServerThread.h"

#include "piProcessImageAccess.h"

struct TMBServerConfHead mbServerConfHead;

//#define MODBUS_DEBUG

/************************************************************************/
/** @ brief start routine for a modbus tcp server thread
 *  
 *	@param[in] arg configuration parameter for the modbus tcp server of type TModbusServerConfiguration
 *
 *	@return returns NULL if initialisation failed
 *
 *	the (lib)modbus initialisation and execution is performed
 *
 */
/************************************************************************/
struct hndlTcpServerThread
{
    modbus_mapping_t* mbMapping;
    modbus_t *mb_server;
    fd_set refset;
};

void cleanupTcpServerThread(void *ptr)
{
    int fd;
    struct hndlTcpServerThread *h = (struct hndlTcpServerThread *)ptr;
    
    syslog(LOG_NOTICE, "cleanupTcpServerThread\n");
    
    if (h->mbMapping)
        modbus_mapping_free(h->mbMapping);
    
    if (h->mb_server)
    {
        modbus_close(h->mb_server);
        modbus_free(h->mb_server);
    }

    for (fd = 0; fd < __FD_SETSIZE; fd++)
    {
        if (FD_ISSET(fd, &h->refset)) // NOLINT
        {
            close(fd);
            syslog(LOG_ERR, "close socket %d\n", fd);
        }
    }
}

void *startTcpServerThread(void *arg)
{
    TModbusServerConfiguration *psModbusConfiguration_l = (TModbusServerConfiguration*)arg;
    struct hndlTcpServerThread hdl;
    int ret;
    int server_socket = -1;
    fd_set rdset;
    /* maximal file descriptor number */
    int fdmax;
    
    struct timespec tv_sleep;
    tv_sleep.tv_sec = 5;        // wait for 5 seconds in case of an error
    tv_sleep.tv_nsec = 0;

    /* Clear the reference set of socket */
    FD_ZERO(&hdl.refset);
    hdl.mbMapping = NULL;
    hdl.mb_server = NULL;

    pthread_cleanup_push(cleanupTcpServerThread, &hdl);
    
    hdl.mbMapping = modbus_mapping_new(
        psModbusConfiguration_l->tModbusDataConfig.u16Coils,
        psModbusConfiguration_l->tModbusDataConfig.u16DiscreteInputs,
        psModbusConfiguration_l->tModbusDataConfig.u16HoldingRegisters,
        psModbusConfiguration_l->tModbusDataConfig.u16InputRegisters);
    
    if (!hdl.mbMapping) {
        syslog(LOG_ERR, "Failed to allocate the mapping: %s\n", modbus_strerror(errno));
        pthread_exit(0);
    }
    
    char st8TcpPort[10];
    snprintf(st8TcpPort, sizeof(st8TcpPort), "%d", psModbusConfiguration_l->tModbusDeviceConfig.uProt.tTcpConfig.i32uPort);
    hdl.mb_server = modbus_new_tcp_pi(psModbusConfiguration_l->tModbusDeviceConfig.uProt.tTcpConfig.szTcpIpAddress, st8TcpPort);
    if (!hdl.mb_server) {
        syslog(LOG_ERR, "Failed to create the modbus context\n");
        pthread_exit(0);
    }
    
#ifdef MODBUS_DEBUG
    modbus_set_debug(mb_server, TRUE);
#endif
    
    //run server
    do
    {
        ret = modbus_tcp_pi_listen(hdl.mb_server, psModbusConfiguration_l->tModbusDeviceConfig.uProt.tTcpConfig.maxModbusConnections);
        if (ret == -1)
        {
            syslog(LOG_ERR, "Failed to create a tcp/ip socket: %s\n", modbus_strerror(errno));

            // wait for 5 seconds and try again
            clock_nanosleep(CLOCK_MONOTONIC, 0, &tv_sleep, NULL);
        }
    } while (ret == -1);
    
    
    server_socket = ret;
    syslog(LOG_NOTICE, "server socket %d\n", server_socket);

                /* Add the server socket */
    FD_SET(server_socket, &hdl.refset);

                /* Keep track of the max file descriptor */
    fdmax = server_socket;
            
    while (1)
    {
        int client_socket = 0;
        rdset = hdl.refset;
        ret = select(fdmax + 1, &rdset, NULL, NULL, NULL);
        if (ret == -1)
        {
            syslog(LOG_ERR, "Could not select: errno=%s\n", modbus_strerror(errno));
            if (server_socket != -1)
            {
                close(server_socket);
            }
            pthread_exit(0);
        }

        /* Run through the existing connections looking for data to be read */
        for (client_socket = 0; client_socket <= fdmax; client_socket++)
        {
            if (!FD_ISSET(client_socket, &rdset))
            {
                continue;
            }
        
            if (client_socket == server_socket)
            {
                syslog(LOG_INFO, "New connection request on socket %d\n", server_socket);
                /* A client is asking a new connection */
                socklen_t addrlen;
                struct sockaddr_in clientaddr;
                int newfd;
            
                /* Handle new connections */
                addrlen = sizeof(clientaddr);
                memset(&clientaddr, 0, sizeof(clientaddr));
                newfd = accept(server_socket, (struct sockaddr *)&clientaddr, &addrlen);
                if (newfd == -1)
                {
                    syslog(LOG_ERR, "Server accept() error");
                }
                else
                {
                    FD_SET(newfd, &hdl.refset);

                    if (newfd > fdmax) {
                        /* Keep track of the maximum */
                        fdmax = newfd;
                    }
                    syslog(LOG_INFO,
                        "New connection from %s:%d on socket %d\n",
                        inet_ntoa(clientaddr.sin_addr),
                        clientaddr.sin_port,
                        newfd);
                }
            }
            else
            {
                modbus_set_socket(hdl.mb_server, client_socket);
                
                syslog(LOG_DEBUG, "Check request on socket %d/%d\n", client_socket, fdmax);
                if (process_modbus_request(hdl.mb_server, hdl.mbMapping, psModbusConfiguration_l) < 0)
                {
                    syslog(LOG_INFO, "Connection closed on socket %d\n", client_socket);
                    close(client_socket);
                    
                    /* Remove from reference set */
                    FD_CLR(client_socket, &hdl.refset);
                    if (client_socket == fdmax)
                    {
                        fdmax--;
                    }
                }
            }
        }
    }
    
    pthread_cleanup_pop(1);
    syslog(LOG_ERR, "Quit the loop: %s\n", modbus_strerror(errno));
    
    return NULL;
}


/************************************************************************/
/** @ brief start routine for a modbus rtu server thread
 *  
 *	@param[in] arg configuration parameter for the modbus rtu server of type TModbusServerConfiguration
 *
 *	@return returns NULL if initialisation failed
 *
 *	the (lib)modbus initialisation and execution is performed
 *
 */
/************************************************************************/
struct hndlRtuServerThread
{
    modbus_mapping_t* mbMapping;
    modbus_t *mb_server;
};

void cleanupRtuServerThread(void *ptr)
{
    struct hndlRtuServerThread *h = (struct hndlRtuServerThread *)ptr;
    
    syslog(LOG_INFO, "cleanupRtuServerThread\n");
    
    if (h->mbMapping)
        modbus_mapping_free(h->mbMapping);
    
    if (h->mb_server)
    {
        modbus_close(h->mb_server);
        modbus_free(h->mb_server);
    }
}
void *startRtuServerThread(void *arg)
{
    TModbusServerConfiguration *psModbusConfiguration_l = (TModbusServerConfiguration*)arg;
    struct hndlRtuServerThread hdl;

    hdl.mb_server = NULL;
    hdl.mbMapping = NULL;
    
    pthread_cleanup_push(cleanupRtuServerThread, &hdl);
    int logRtuPath = 0; // late declaration prevents Wclobbered error
    
    /* Wait for serial device getting ready(Readable, Writable) */
    while(access(psModbusConfiguration_l->tModbusDeviceConfig.uProt.tRtuConfig.sz8DeviceFilePath,
                                                        R_OK | W_OK)) {
        if (!logRtuPath) {
            syslog(LOG_INFO, "RTU Server waiting for serial device:%s\n",
                psModbusConfiguration_l->tModbusDeviceConfig.uProt.tRtuConfig.sz8DeviceFilePath);
            logRtuPath = 1;
        }
        /* Repeat checking in 1 Sec */
        sleep(1);
    }
    syslog(LOG_INFO, "RTU Server got serial device:%s\n",
        psModbusConfiguration_l->tModbusDeviceConfig.uProt.tRtuConfig.sz8DeviceFilePath);

    hdl.mbMapping = modbus_mapping_new(
        psModbusConfiguration_l->tModbusDataConfig.u16Coils,
        psModbusConfiguration_l->tModbusDataConfig.u16DiscreteInputs,
        psModbusConfiguration_l->tModbusDataConfig.u16HoldingRegisters,
        psModbusConfiguration_l->tModbusDataConfig.u16InputRegisters);
    
    if (!hdl.mbMapping) {
        syslog(LOG_ERR, "Failed to allocate the mapping: %s\n", modbus_strerror(errno));
        pthread_exit(0);
    }

    hdl.mb_server = modbus_new_rtu(
        psModbusConfiguration_l->tModbusDeviceConfig.uProt.tRtuConfig.sz8DeviceFilePath,
        psModbusConfiguration_l->tModbusDeviceConfig.uProt.tRtuConfig.i32uBaud,
        psModbusConfiguration_l->tModbusDeviceConfig.uProt.tRtuConfig.cParity,
        psModbusConfiguration_l->tModbusDeviceConfig.uProt.tRtuConfig.i8uDatabits,
        psModbusConfiguration_l->tModbusDeviceConfig.uProt.tRtuConfig.i8uStopbits);

    if (!hdl.mb_server) {
        syslog(LOG_ERR, "Failed to create the modbus context\n");
        pthread_exit(0);
    }
    
#ifdef MODBUS_DEBUG
    modbus_set_debug(hdl.mb_server, TRUE);
#endif
    
    if (modbus_set_slave(hdl.mb_server, psModbusConfiguration_l->tModbusDeviceConfig.uProt.tRtuConfig.u8DeviceModbusAddress) == -1)
    {
        syslog(LOG_ERR, "Set server address failed: %s\n", modbus_strerror(errno));
        pthread_exit(0);
    }
    
    
    if (modbus_connect(hdl.mb_server) == -1) {
        syslog(LOG_ERR, "Unable to connect: %s\n", modbus_strerror(errno));
        pthread_exit(0);
    }	

    while (1)
    {
        process_modbus_request(hdl.mb_server, hdl.mbMapping, psModbusConfiguration_l);
    }
    
    pthread_cleanup_pop(1); // this makro closes the loop of pthread_cleanup_push
}



/************************************************************************/
/** @ brief common modbus request processing 
 *  
 *	@param mb_server_p the modbus context
 *	@param mbMapping_p the modbus mapping
 *	@patram psModbusConfiguration_p the modbus device configuration
 *
 *	@return '0' if processing was successful otherwise '-1'
 *
 *	the (lib)modbus initialisation and execution is performed
 *
 */
/************************************************************************/
int32_t process_modbus_request(modbus_t *mb_server_p, modbus_mapping_t* mbMapping_p, TModbusServerConfiguration *psModbusConfiguration_p)
{
    uint8_t req[MODBUS_TCP_MAX_ADU_LENGTH];     // request buffer TCP
    int32_t len = 0;                            //length of the request/response
    
    len = modbus_receive(mb_server_p, req);
    if (len > 0)
    {
        /*	due to seperated input and output areas in the pi process image,
            data from pi process image (e.g. other device) can only be requested by
            the modbus functions READ_INPUT_REGISTERS and READ_DISCRETE_INPUTS
            therefor only for this modbus functions a read from the pi process image is performed
         */
        uint8_t mb_function_code = req[modbus_get_header_length(mb_server_p)];	//the modbus function code for the current request
        if ((mb_function_code == eREAD_INPUT_REGISTERS) || (mb_function_code == eREAD_DISCRETE_INPUTS)) 
        {
            readModbusDataFromProcessImage(mbMapping_p, &(psModbusConfiguration_p->tProcessImageConfig));
        }
        len = modbus_reply(mb_server_p, req, len, mbMapping_p);
        if (len == -1)
        {
            syslog(LOG_ERR, "modbus reply failed\n");
            return -1;
        }			
        if ((mb_function_code == eWRITE_MULTIPLE_COILS) || (mb_function_code == eWRITE_MULTIPLE_REGISTERS) ||
            (mb_function_code == eWRITE_SINGLE_COIL) || (mb_function_code == eWRITE_SINGLE_REGISTER) ||
            (mb_function_code == eWRITE_AND_READ_REGISTERS) || (mb_function_code == eWRITE_MASK_REGISTER))
        {
            writeModbusDataToProcessImage(mbMapping_p, &(psModbusConfiguration_p->tProcessImageConfig));
        }		
    }
    else if (len == -1)
    {
        syslog(LOG_ERR, "modbus receive failed\n");
        return -1;
    }

    return 0;
}
