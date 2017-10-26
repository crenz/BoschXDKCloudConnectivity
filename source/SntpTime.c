/*
* Licensee agrees that the example code provided to Licensee has been developed and released by Bosch solely as an example to be used as a potential reference for Licensee�s application development. 
* Fitness and suitability of the example code for any use within Licensee�s applications need to be verified by Licensee on its own authority by taking appropriate state of the art actions and measures (e.g. by means of quality assurance measures).
* Licensee shall be responsible for conducting the development of its applications as well as integration of parts of the example code into such applications, taking into account the state of the art of technology and any statutory regulations and provisions applicable for such applications. Compliance with the functional system requirements and testing there of (including validation of information/data security aspects and functional safety) and release shall be solely incumbent upon Licensee. 
* For the avoidance of doubt, Licensee shall be responsible and fully liable for the applications and any distribution of such applications into the market.
* 
* 
* Redistribution and use in source and binary forms, with or without 
* modification, are permitted provided that the following conditions are 
* met:
* 
*     (1) Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer. 
* 
*     (2) Redistributions in binary form must reproduce the above copyright
*     notice, this list of conditions and the following disclaimer in
*     the documentation and/or other materials provided with the
*     distribution.  
*     
*     (3)The name of the author may not be used to
*     endorse or promote products derived from this software without
*     specific prior written permission.
* 
*  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR 
*  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
*  DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
*  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
*  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
*  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
*  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
*  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
*  IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
*  POSSIBILITY OF SUCH DAMAGE.
*
* Contributors:
* Bosch Software Innovations GmbH
*/
/*----------------------------------------------------------------------------*/
/**
* @ingroup BOSCH_XDK_CLOUD_CONNECTIVITY
*
* @defgroup SNTP_TIME SNTP Time
* @{
*
* @brief This file provides the Implementation of SntpTime.
*
* @details SNTPTime processing source file
*
* @file
**/
 
//lint -esym(956,*) /* Suppressing Non const, non volatile static or external variable */

/* own header files */
#include "SntpTime.h"

/* additional interface header files */
#include <Serval_XUdp.h>
#include <Serval_Log.h>
#include <Serval_Clock.h>
#include <PAL_initialize_ih.h>

#define LOG_MODULE "NTP"
#define NTP_PACKET_SIZE					UINT8_C(48)						/* NTP time is in the first 48 bytes of message */
#define NTP_DNS_TIMEOUT_IN_S            UINT16_C(5)
#define NTP_DNS_RETRY_INTERVAL_IN_MS    UINT16_C(100)

static uint32_t SysUpTime2UtcOffset = UINT32_C(0);

static void SendCallback(Msg_T *msg_ptr, retcode_t status);
static void ReceiveCallback(Msg_T *msg_ptr, retcode_t status);

/* The description is in the header file. */
void SetUtcTime(uint32_t utcTime)
{
    retcode_t rc = RC_CLOCK_ERROR_FATAL;
    uint32_t sysUpTime;
    rc = Clock_getTime(&sysUpTime);
    if(rc!= RC_OK)
    {
        printf("Failed to get the Clock Time \r\n");
    }
    SysUpTime2UtcOffset = utcTime - sysUpTime;
}

/* The description is in the header file. */
uint32_t GetUtcTime()
{
    retcode_t rc = RC_CLOCK_ERROR_FATAL;
    uint32_t sysUpTime;
    rc = Clock_getTime(&sysUpTime);
    if(rc!= RC_OK)
    {
        printf("Failed to get the Clock Time \r\n");
    }
    return sysUpTime + SysUpTime2UtcOffset;
}

/* The description is in the header file. */
void InitSntpTime()
{
    uint32_t now = 0;
    retcode_t rc = RC_OK;
    Msg_T *MsgHandlePtr = NULL;
    Ip_Port_T port = Ip_convertIntToPort(SNTP_DEFAULT_PORT); // also used for local ntp client! */

    rc = XUdp_initialize();
    if (rc != RC_OK)
    {
        LOG_ERROR("Failed to init XUDP; rc=" RC_RESOLVE_FORMAT_STR "\n",
                RC_RESOLVE((int)rc));
        return;
    }

    rc = XUdp_start(port, ReceiveCallback);
    if (rc != RC_OK)
    {
        LOG_ERROR("Failed to start XUDP; rc=" RC_RESOLVE_FORMAT_STR "\n",
                RC_RESOLVE((int)rc));
        return;
    }

    Ip_Address_T sntpIpAddress;
    uint8_t buffer[NTP_PACKET_SIZE];
    unsigned int bufferLen = NTP_PACKET_SIZE;

    /* init request: */
    memset(buffer, 0, NTP_PACKET_SIZE);
    buffer[0] = 0b11100011; /* LI, Version, Mode */ /*lint !e146 */
    buffer[1] = 0; /* Stratum, or type of clock */
    buffer[2] = 6; /* Polling Interval */
    buffer[3] = 0xEC; /* Peer Clock Precision */
    /* 8 bytes of zero for Root Delay & Root Dispersion */
    buffer[12] = 49;
    buffer[13] = 0x4E;
    buffer[14] = 49;
    buffer[15] = 52;

    rc = Clock_getTime(&now);
    if (RC_OK == rc)
    {
        /* time available, use timeout */
        const uint32_t dnsEndTime = now + NTP_DNS_TIMEOUT_IN_S;
        while (dnsEndTime > now)
        {
            rc = PAL_getIpaddress((uint8_t *) SNTP_DEFAULT_ADDR, (Ip_Address_T *) &sntpIpAddress);
            if (RC_OK == rc)
                break;
            vTaskDelay(NTP_DNS_RETRY_INTERVAL_IN_MS / portTICK_RATE_MS);
            if (RC_OK != Clock_getTime(&now))
                break;
        }
    }
    else
    {
        /* no time, no retry */
        rc = PAL_getIpaddress((uint8_t *) SNTP_DEFAULT_ADDR, (Ip_Address_T *) &sntpIpAddress);
    }

    /* resolve IP address of server hostname: 0.de.pool.ntp.org eg.: 176.9.104.147 */
    if (RC_OK != rc)
    {
        printf("NTP Server %s could not be resolved. Use a static IP (176.9.104.147) instead!\n\r", SNTP_DEFAULT_ADDR);
        /* dirctly use a "0.de.pool.ntp.org" pool IP: 176.9.104.147 */
        Ip_convertOctetsToAddr(176, 9, 104, 147, &sntpIpAddress);
    }

    /* now send request: */
    rc = XUdp_push(&sntpIpAddress, SNTP_DEFAULT_PORT, buffer, bufferLen, // to default NTP server port */
            SendCallback, &MsgHandlePtr);
    if (rc != RC_OK)
    {
        LOG_ERROR("Sending failure; rc=" RC_RESOLVE_FORMAT_STR "\n",
                RC_RESOLVE((int)rc));
        return;
    }
    LOG_INFO("Pushed Echo\n");
    return;
}

static void SendCallback(Msg_T *msg_ptr, retcode_t status)
{
    BCDS_UNUSED(msg_ptr);
    LOG_DEBUG("Sending Complete\n");
    printf("NTP request Sending Complete\n\r");

    if (status != RC_OK)
    {
        LOG_ERROR("Sending status not RC_OK; status=" RC_RESOLVE_FORMAT_STR "\n",
                RC_RESOLVE((int)status));
        printf("Sending status not RC_OK; status=" RC_RESOLVE_FORMAT_STR "\n\r",
                RC_RESOLVE((int)status));
    }
}

static void ReceiveCallback(Msg_T *msg_ptr, retcode_t status)
{
    LOG_DEBUG("Received Data\n");

    unsigned int payloadLen;
    uint8_t *payload_ptr;

    if (status != RC_OK)
    {
        LOG_ERROR("Receiving status not RC_OK; status="
                RC_RESOLVE_FORMAT_STR "\n", RC_RESOLVE((int)status));
    }
    XUdp_getXUdpPayload(msg_ptr, &payload_ptr, &payloadLen);

    LOG_INFO("ResponseLen:%d\n", payloadLen);

    if (payloadLen >= NTP_PACKET_SIZE)
    {
        uint64_t secsSince1900;
        /* convert 4 bytes starting at location 40 to a long integer */
        secsSince1900 = (unsigned long) payload_ptr[40] << 24;
        secsSince1900 |= (unsigned long) payload_ptr[41] << 16;
        secsSince1900 |= (unsigned long) payload_ptr[42] << 8;
        secsSince1900 |= (unsigned long) payload_ptr[43];
        /* subtract 70 years 2208988800UL; (Unix: starting from 1970) */
        uint64_t secsSince1970 = secsSince1900 - 2208988800UL; /* UTC else + timeZone*SECS_PER_HOUR; */
        printf("NTP got UTC secSince1970: %llu\n\r", secsSince1970);
        SetUtcTime(secsSince1970);
    }
    else
    {
        printf("NTP response not valid!\n\r");
    }
}

/**@} */
