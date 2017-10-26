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
* @defgroup DNS_UTIL DNS Util
* @{
*
* @brief This file provides the Implementation of DnsUtil.
*
* @details Domain Name Servicing source file
*
* @file
**/
 
/* own header files */
/*lint -emacro(641, RC_RESOLVE) */

#include "DnsUtil.h"

/* additional interface header files */
#include <PAL_initialize_ih.h>
#include <Serval_Network.h>
#include <Serval_Ip.h>
#include <Serval_Clock.h>

#define MUTEX_TIMEOUT  (100/(portTICK_PERIOD_MS)) /**< timeout for mutex functions */

#define DNS_TASK_STACK_SIZE        UINT16_C(512)         /**< TASK stack size for DNS Task */
#define DNS_TASK_PRIORITY          UINT8_C(2)            /**< Task Priority for DNS Task*/

#define DNS_RETRY_INTERVAL_IN_MS    UINT16_C(100) /**< interval in milliseconds for lookup retries */

/*lint -esym(956, Dns*Handle) */
static xSemaphoreHandle DnsMutexHandle; /**< mutex to protect DNS context data for asynchronous lookup */
static xSemaphoreHandle DnsSemaHandle;  /**< semaphore to trigger asynchronous lookup */
static xTaskHandle DnsTaskHandle; /**< task to execute asynchronous lookup */

/*lint -esym(956, DnsUrl, DnsResolvedURL, DnsResolvedCallback, DnsContext, DnsTimeout) used with mutex */
static char DnsUrl[128];            /**< DNS context data, URL to be resolved */
static char DnsResolvedURL[128];    /**< DNS context data, resolved URL */
static DnsCallback_T DnsResolvedCallback; /**< DNS context data, resolved callback */
static uint32_t DnsTimeout; /**< DNS context data, retry timeout in seconds */
static void * DnsCallbackContext; /**< DNS context data, callback context */

/**
 * Task for asynchronous DNS lookup.
 * If parameter is provided, it must point to a "int32_t" which contains the number of loops.
 * If not provided (NULL) unlimited loops are executed.
 * @param pvParameters, optional pointer to number of loops.
 */
static void DnsTask(void *pvParameters)
{
    uint64_t StartMillis = 0;
    uint64_t FinishMillis = 0;
    int32_t counter = 0;
    retcode_t rc = RC_DNS_OK;

    if (NULL != pvParameters)
    {
        counter = *(int32_t*) pvParameters;
    }
    for (;;)
    {
        if (xSemaphoreTake(DnsSemaHandle, portMAX_DELAY) == pdTRUE)
        {
            if (xSemaphoreTake(DnsMutexHandle, portMAX_DELAY) == pdTRUE)
            {
                printf("DNS lookup %s\n", DnsUrl);
                retcode_t rcTime = Clock_getTimeMillis(&StartMillis);
                rc = Dns_ExtendedResolveHostname(DnsUrl, DnsResolvedURL, sizeof(DnsResolvedURL), DnsTimeout, NULL, NULL);
                if (RC_OK == rcTime)
                {
                    rcTime = Clock_getTimeMillis(&FinishMillis);
                }
                if (RC_OK == rcTime)
                {
                    if (RC_OK == rc)
                    {
                        printf("DNS resolved %s, time %llu ms\n", DnsResolvedURL, (unsigned long long) (FinishMillis - StartMillis));
                    }
                    else
                    {
                        printf("DNS failed " RC_RESOLVE_FORMAT_STR ", time %llu ms\n", RC_RESOLVE(rc), (unsigned long long) (FinishMillis - StartMillis));
                    }
                }
                else
                {
                    if (RC_OK == rc)
                    {
                        printf("DNS resolved %s\n", DnsResolvedURL);
                    }
                    else
                    {
                        printf("DNS failed " RC_RESOLVE_FORMAT_STR "\n", RC_RESOLVE(rc));
                    }
                }
                assert(NULL != DnsResolvedCallback);
                DnsResolvedCallback(rc, DnsResolvedURL, DnsCallbackContext);
                /* cleanup */
                DnsResolvedCallback = NULL;
                DnsTimeout = 0;
                if (pdTRUE != xSemaphoreGive(DnsMutexHandle))
                {
                    assert(false);
                }
            }
        }
        if (NULL != pvParameters)
        {
            --counter;
            if (0 >= counter)
                break;
        }
    }
}

retcode_t Dns_Init(void)
{
    memset(DnsUrl, 0, sizeof(DnsUrl));
    memset(DnsResolvedURL, 0, sizeof(DnsResolvedURL));
    DnsResolvedCallback = NULL;
    DnsCallbackContext = NULL;

    DnsSemaHandle = xSemaphoreCreateBinary();
    if (NULL == DnsSemaHandle)
    {
        /* Failed to create semaphore */
        assert(false);
    }
    DnsMutexHandle = xSemaphoreCreateMutex();
    if (NULL == DnsMutexHandle)
    {
        /* Failed to create mutex */
        assert(false);
    }
    if (xTaskCreate(DnsTask, (const char * const) "dns", DNS_TASK_STACK_SIZE, NULL, DNS_TASK_PRIORITY, &DnsTaskHandle) != pdPASS)
    {
        assert(false);
    }

    return RC_DNS_OK;
}

retcode_t Dns_StartResolveHostname(const char * const URL, uint32_t timeout, const DnsCallback_T callback, void * context)
{
    retcode_t rc = RC_DNS_BUSY;
    assert(NULL != URL);
    assert(NULL != callback);
    if (NULL == URL || NULL == callback)
    {
        /* check for none debug build and lint warnings */
        return RC_PLATFORM_ERROR;
    }
    if (pdTRUE == xSemaphoreTake(DnsMutexHandle, MUTEX_TIMEOUT))
    {
        if (NULL == DnsResolvedCallback)
        {
            if (sizeof(DnsUrl) <= strlen(URL))
            {
                rc = RC_PLATFORM_ERROR;
                goto free_mutex;
            }
            strcpy(DnsUrl, URL);
            memset(DnsResolvedURL, 0, sizeof(DnsResolvedURL));
            DnsResolvedCallback = callback;
            DnsCallbackContext = context;
            DnsTimeout = timeout;
            if (pdTRUE == xSemaphoreGive(DnsSemaHandle))
            {
                rc = RC_DNS_PENDING;
            }
            else
            {
                assert(false);
                DnsResolvedCallback = NULL;
                DnsCallbackContext = NULL;
                DnsTimeout = 0;
                rc = RC_PLATFORM_ERROR;
            }
        }
        free_mutex:
        if (pdTRUE != xSemaphoreGive(DnsMutexHandle))
        {
            assert(false);
        }
    }
    return rc;
}

retcode_t Dns_ResolveHostname(const char * const URL, char * const resolvedURL, const size_t sizeResolvedURL, uint32_t timeout)
{
    return Dns_ExtendedResolveHostname(URL, resolvedURL, sizeResolvedURL, timeout, NULL, NULL);
}

retcode_t Dns_ExtendedResolveHostname(const char * const URL, char * const resolvedURL, const size_t sizeResolvedURL, uint32_t timeout, const DnsCallback_T callback, void * context)
{
    retcode_t rc = RC_DNS_OK;
    Ip_Address_T Address;
    size_t DestSize = sizeResolvedURL;
    size_t HostSize = 0;
    char* Dest = resolvedURL;
    const char* Pos = NULL;
    const char* End = NULL;

// check for scheme
    Pos = strstr(URL, "://");
    if (NULL == Pos)
    {
        // no scheme
        Pos = URL;
        Dest[0] = 0;
    }
    else
    {
        // copy scheme
        int Head;
        Pos += 3; // skip "://"
        Head = Pos - URL;
        memmove(Dest, URL, Head);
        Dest[Head] = 0;
        Dest += Head;
        DestSize -= Head;
    }
// end of host part
    End = strpbrk(Pos, ":/");
    if (NULL == End)
    {
        HostSize = strlen(Pos);
    }
    else
    {
        HostSize = End - Pos;
    }
    if (HostSize >= DestSize)
    {
        // host part too large for left buffer
        rc = RC_PLATFORM_ERROR;
        goto error_exit;
    }
    memmove(Dest, Pos, HostSize);
    Dest[HostSize] = 0;

// check for already resolved address
    rc = Ip_convertStringToAddr(Dest, &Address);
    if (RC_OK == rc)
    {
        // already resolved address
        if (strlen(Pos) >= DestSize)
        {
            // host part too large for left buffer
            rc = RC_PLATFORM_ERROR;
            goto error_exit;
        }
        strcpy(Dest, Pos);
        return RC_DNS_OK;
    }
    if (PAL_IP_ADDRESS_SIZE > DestSize)
    {
        // left buffer too small for resolved ip address
        rc = RC_PLATFORM_ERROR;
        goto error_exit;
    }
// resolve host
    if (NULL == callback)
    {
        uint32_t now;

        if (0 < timeout && (RC_OK == (rc = Clock_getTime(&now))))
        {
            /* time available, use timeout */
            const uint32_t dnsEndTime = now + timeout;
            while (dnsEndTime > now)
            {
                rc = PAL_getIpaddress((uint8_t*)Dest, &Address);
                if (RC_OK == rc)
                    break;
                vTaskDelay(DNS_RETRY_INTERVAL_IN_MS / portTICK_RATE_MS);
                if (RC_OK != Clock_getTime(&now))
                    break;
            }
        }
        else
        {
            /* if no timeout is used or Clock_getTime returns an error */
            rc = PAL_getIpaddress((uint8_t*)Dest, &Address);
        }
        if (RC_OK != rc)
        {
            goto error_exit;
        }

        Dest[0] = 0;
        HostSize = Ip_convertAddrToString(&Address, Dest);
        if (NULL != End)
        {
            Dest += HostSize;
            DestSize -= HostSize;
            if (strlen(End) >= DestSize)
            {
                // tail too large for left buffer
                rc = RC_PLATFORM_ERROR;
                goto error_exit;
            }
            strcpy(Dest, End);
        }
    }
    else
    {
        rc = Dns_StartResolveHostname(URL, timeout, callback, context);
        goto error_exit;
    }
    return RC_DNS_OK;
    error_exit:
    memset(resolvedURL, 0, sizeResolvedURL);
    return rc;
}
/**@} */
