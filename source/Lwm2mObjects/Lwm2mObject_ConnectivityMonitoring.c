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
/** s
 * @brief
 *
 * @file
 **/
/* own header files */
#include "Lwm2mObjects.h"
#include "Lwm2mInterface.h"
#include "Lwm2mUtil.h"

/* additional interface header files */
#include <Serval_Exceptions.h>

/* global variables ********************************************************* */

/* constant definitions ***************************************************** */
#define CONN_MON_RESOURCES_INDEX(res) LWM2M_RESOURCES_INDEX(ConnMonResources, res)

/* impl. specific see: Lwm2mObject_Device.h: DeviceResource_S */
#define NETWORK_BEARER_COUNT	1
#define IP_ADDRESSES_COUNT		1

/* local variables ********************************************************* */
/*lint -esym(956, NetworkBearer) const ?*/
static int32_t NetworkBearer[NETWORK_BEARER_COUNT] = { 21 };

/*lint -esym(956, IpAddresses) mutex used*/
static char IpAddresses[IP_ADDRESSES_COUNT][16] = { "" }; /* xxx.xxx.xxx.xxx 4*3+3+1=16 max. */
static LWM2M_MUTEX_INSTANCE(Mutex) = LWM2M_MUTEX_INIT_VALUE;
static volatile bool Started = false;

/* local functions ********************************************************** */
/* @brief
 * This function is used to return the available network Bearer array
 */
static retcode_t GetAvailableNetworkBearer(Lwm2mSerializer_T *serializer_ptr, Lwm2mParser_T *parser_ptr)
{
    if (parser_ptr != NULL)
    {
        return (RC_LWM2M_METHOD_NOT_ALLOWED);
    }
    else
    {
//        printf("availableNetworkBearer\r\n");
        retcode_t
        rc = Lwm2mSerializer_startSerializingResourceArray(serializer_ptr);
        if (rc != RC_OK)
            return rc;
        rc = Lwm2mSerializer_setResourceInstanceId(serializer_ptr, 0); //index=0
        if (rc != RC_OK)
            return rc;
        rc = Lwm2mSerializer_serializeInt(serializer_ptr, NetworkBearer[0]); //index=0
        if (rc != RC_OK)
            return rc;
        return Lwm2mSerializer_endSerializingResourceArray(serializer_ptr);
    }
}
/* @brief
 * This function is used to update the return the current IP address.
 */
static retcode_t GetIpAddresses(Lwm2mSerializer_T *serializer_ptr, Lwm2mParser_T *parser_ptr)
{
    if (parser_ptr != NULL)
    {
        return (RC_LWM2M_METHOD_NOT_ALLOWED);
    }
    else
    {
//        printf("ipAddresses\r\n");
        retcode_t
        rc = Lwm2mSerializer_startSerializingResourceArray(serializer_ptr);
        if (rc != RC_OK)
            return rc;
        rc = Lwm2mSerializer_setResourceInstanceId(serializer_ptr, 0); //index=0
        if (rc != RC_OK)
            return rc;
        StringDescr_T strDes;
        if (LWM2M_MUTEX_LOCK(Mutex))
        {
            StringDescr_set(&strDes, IpAddresses[0], strlen(IpAddresses[0])); //index=0
            rc = Lwm2mSerializer_serializeString(serializer_ptr, &strDes);
            LWM2M_MUTEX_UNLOCK(Mutex);
        }
        if (rc != RC_OK)
            return rc;
        return Lwm2mSerializer_endSerializingResourceArray(serializer_ptr);
    }
}
/* global functions */

void ObjectConnectivityMonitoring_Init(void)
{
    Started = false;
    NetworkBearer[0] = 21;
    IpAddresses[0][0] = 0;
    IpAddresses[0][sizeof(IpAddresses[0]) - 1] = 0;
    ConnMonResources.radioSignalStrength.data.i = 0;
    LWM2M_MUTEX_CREATE(Mutex);
}

void ObjectConnectivityMonitoring_Enable(void)
{
    Started = true;
}

void ObjectConnectivityMonitoring_SetRadioSignalStrength(int rss)
{ // 0..64 in dBm in GSM case!
    if (ConnMonResources.radioSignalStrength.data.i != rss)
    {
        ConnMonResources.radioSignalStrength.data.i = rss;
        if (Started)
        {
            Lwm2m_URI_Path_T rscUriPath = { OBJECTS_IX_CONN_MON_0, OBJECTS_IX_CONN_MON_0,
                    CONN_MON_RESOURCES_INDEX(radioSignalStrength) };
            /*lint -e(534) purposefully suppressing the warning:Ignoring return value of function,since in CONF mode this may fail many times */
            Lwm2mReporting_resourceChanged(&rscUriPath);
        }
    }
}
void ObjectConnectivityMonitoring_SetIpAddress(const char* ipAddr)
{
    if (strcmp(IpAddresses[0], ipAddr) != 0)
    {
        if (LWM2M_MUTEX_LOCK(Mutex))
        {
            strncpy(IpAddresses[0], ipAddr, (sizeof(IpAddresses[0]) - 1));
            IpAddresses[0][sizeof(IpAddresses[0]) - 1] = 0;
            LWM2M_MUTEX_UNLOCK(Mutex);
            if (Started)
            {
                Lwm2m_URI_Path_T rscUriPath = { OBJECTS_IX_CONN_MON_0, OBJECTS_IX_CONN_MON_0,
                        CONN_MON_RESOURCES_INDEX(ipAddresses) };
                /*lint -e(534) purposefully suppressing the warning:Ignoring return value of function,since in CONF mode this may fail many times */
                Lwm2mReporting_resourceChanged(&rscUriPath);
            }
        }
    }
}

ConnMonResource_T ConnMonResources =
        {
                { 0, LWM2M_INTEGER(21) | LWM2M_READ_ONLY }, // minRangeValue|R|Single|M|Int   |   |
                { 1, LWM2M_DYNAMIC_ARRAY(GetAvailableNetworkBearer) | LWM2M_READ_ONLY  }, // avaNetwBearer|R|Multi |M|Int   |   |
                { 2, LWM2M_INTEGER(0) | LWM2M_READ_ONLY }, // RadioSigStrn.|R|Single|M|Int   |   | dBm |
                { 4, LWM2M_DYNAMIC_ARRAY(GetIpAddresses)| LWM2M_READ_ONLY  }, // ipAddresses 	|R|Multi |M|String|   |
                };
