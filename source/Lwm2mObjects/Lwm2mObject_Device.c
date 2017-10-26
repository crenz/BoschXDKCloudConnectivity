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
 * @brief
 *
 * @file
 **/
/* own header files */
#include "Lwm2mObjects.h"
#include "Lwm2mInterface.h"
#include "SntpTime.h"

/* additional interface header files */
#include <Serval_Exceptions.h>
#include <Serval_Log.h>
#include <em_system.h>
#include <BCDS_FWContainer.h>
#include <BCDS_FotaStorageAgent.h>
#include <XdkVersion.h>
#include <FotaDownload.h>
#include "Lwm2mObject_SensorDevice.h"

#define LOG_MODULE "DVO" /**< serval logging prefix */

/* global variables ********************************************************* */

/* constant definitions ***************************************************** */
#define LWM2MOBJECTS_UDP_LENGTH                 UINT32_C(1)
#define LWM2MOBJECTS_UDP_QUEUED_LENGTH          UINT32_C(2)
#define LWM2MOBJECTS_SMS_LENGTH                 UINT32_C(1)
#define LWM2MOBJECTS_SMS_QUEUED_LENGTH          UINT32_C(2)
#define LWM2MOBJECTS_UDP_AND_SMS_LENGTH         UINT32_C(2)
#define LWM2MOBJECTS_UDP_QUEUED_AND_SMS_LENGTH  UINT32_C(3)

#define DEVICE_RESOURCES_INDEX(res) LWM2M_RESOURCES_INDEX(DeviceResources, res)

#define POWER_SOURCES_COUNT		    			2

/* local variables ********************************************************* */

/*lint -esym(956,tz) */
static char tz[20] = "Europe/Berlin";
/*lint -e(956) */
static int error_codes_count = 1;
/*lint -esym(956,error_codes) */
static int32_t error_codes[5] = { 0, 0, 0, 0, 0 };
static volatile bool started = false;


/* local functions ********************************************************** */
/* @brief
 * This function is used to update the errorCode Resource value into the LWM2M Server*
 */
static retcode_t util_serialize_array(Lwm2mSerializer_T *serializer_ptr, int count, int32_t values[])
{
    int index;

    if (serializer_ptr == NULL)
        return (RC_LWM2M_METHOD_NOT_ALLOWED);
    retcode_t rc = Lwm2mSerializer_startSerializingResourceArray(serializer_ptr);
    if (rc != RC_OK)
        return rc;

    for (index = 0; index < count; ++index)
    {
        rc = Lwm2mSerializer_setResourceInstanceId(serializer_ptr, index);
        if (rc != RC_OK)
            return rc;

        rc = Lwm2mSerializer_serializeInt(serializer_ptr, values[index]);
        if (rc != RC_OK)
            return rc;
    }

    return Lwm2mSerializer_endSerializingResourceArray(serializer_ptr);
}

/* @brief
 * This function is used to set the device serial number resource with the XDK CoreID in the LWM2M Server*
 */
static retcode_t serialNumber_RO(Lwm2mSerializer_T *serializer_ptr, Lwm2mParser_T *parser_ptr)
{
    if (parser_ptr != NULL)
    {
        return (RC_LWM2M_METHOD_NOT_ALLOWED);
    }
    char coreID[17]; // max. 16 hex digits (uint64_t)
    sprintf(coreID, "%llX", SYSTEM_GetUnique());
#ifdef USE_LOG_FOR_DEVICE
    printf("serialNumber (coreID): %s\r\n", coreID);
#endif
    StringDescr_T strDescr_sn;
    StringDescr_set(&strDescr_sn, coreID, strlen(coreID));
    return (Lwm2mSerializer_serializeString(serializer_ptr, &strDescr_sn));
}

/*
 * @brief
 *
 * This function is used to set the firmware version number present as part of the firmware binary from the Server
 *
 */
static retcode_t firmwareVersionNumber_RO(Lwm2mSerializer_T *serializer_ptr, Lwm2mParser_T *parser_ptr)
{
    if (parser_ptr != NULL)
    {
        return (RC_LWM2M_METHOD_NOT_ALLOWED);
    }
    Retcode_T retval = RETCODE_OK;

    uint32_t xdkVersion;
    uint32_t fwVersionValue = UINT32_C(0x00000000);
    uint8_t* startOfAppVersionPtr = NULL;
    uint8_t index = 0;
    uint8_t versionNo[19] = { 0 };
    xdkVersion = XdkVersion_GetVersion();
#if XDK_FOTA_ENABLED_BOOTLOADER == 1
    retval = FWContainer_ReadFWVersion(FotaStorageAgentHandle, FOTA_PARTITION_PRIMARY, (uint8_t *) &fwVersionValue);
#endif
    if (RETCODE_OK == retval)
    {
        startOfAppVersionPtr = convert32IntegerToVersionString(fwVersionValue, xdkVersion, versionNo);
    }
    index = startOfAppVersionPtr - versionNo;
    versionNo[index-1] = '-';
    #if XDK_FOTA_ENABLED_BOOTLOADER == 0
    /*Replace with default string in case the FOTA version is not there*/
    versionNo[index] = 'x';
    versionNo[index+1] = 'x';
    versionNo[index+2] = '.';
    versionNo[index+3] = 'x';
    versionNo[index+4] = 'x';
    versionNo[index+5] = '.';
    versionNo[index+6] = 'x';
    versionNo[index+7] = 'x';
    versionNo[index+8] = '\0';
#endif
    StringDescr_T strDescr_sn;
    StringDescr_set(&strDescr_sn, (char*) versionNo, strlen((char*) versionNo));
    return (Lwm2mSerializer_serializeString(serializer_ptr, &strDescr_sn));
}

/* @brief
 * This function is used to update the deviceReboot Resource value into the LWM2M Server*
 */
static retcode_t deviceRebootFunc(Lwm2mSerializer_T *serializer_ptr, Lwm2mParser_T *parser_ptr)
{
    (void) parser_ptr;
    (void) serializer_ptr;
    Retcode_T retVal = RETCODE_OK;
    retVal = ObjectSensorDevice_Disable();
    if(RETCODE_OK != retVal )
    {
    	printf("Unable to stop the sensor sampling timer \r\n");
    }
    printf("deviceReboot\r\n");
    Lwm2mInterfaceReboot();
    return (RC_OK);
}
/* @brief
 * This function is used to update the factoryReset Resource value into the LWM2M Server*
 */
static retcode_t factoryResetFunc(Lwm2mSerializer_T *serializer_ptr, Lwm2mParser_T *parser_ptr)
{
    (void) parser_ptr;
    (void) serializer_ptr;

    printf("factoryReset\r\n");
    return (RC_OK);
}


/* @brief
 * This function is used to update the errorCode Resource value into the LWM2M Server*
 */
static retcode_t errorCodeFunc(Lwm2mSerializer_T *serializer_ptr, Lwm2mParser_T *parser_ptr)
{
    (void) parser_ptr;
#ifdef USE_LOG_FOR_DEVICE
    printf("errorCode\r\n");
#endif
    return util_serialize_array(serializer_ptr, error_codes_count, error_codes);
}
/* @brief
 * This function is used to update the resetErrorCode Resource value into the LWM2M Server*
 */
static retcode_t resetErrorCodeFunc(Lwm2mSerializer_T *serializer_ptr, Lwm2mParser_T *parser_ptr)
{
    (void) parser_ptr;
    (void) serializer_ptr;

#ifdef USE_LOG_FOR_DEVICE
    printf("resetErrorCode\r\n");
#endif
    error_codes_count = 1;
    error_codes[0] = 0;

    if (started)
    {
        Lwm2m_URI_Path_T errorCodeUriPath = { OBJECTS_IX_DEVICE_0, OBJECTS_IX_DEVICE_0, DEVICE_RESOURCES_INDEX(errorCode) };
        Lwm2mReporting_resourceChanged(&errorCodeUriPath);
    }

    return (RC_OK);
}

/* @brief
 * This function is used to update the currentTime Resource value into the LWM2M Server*
 */
static retcode_t getCurrentTime(Lwm2mSerializer_T *serializer_ptr, Lwm2mParser_T *parser_ptr)
{
    retcode_t rc = RC_OK;

    if (parser_ptr != NULL)
    {
        /* Update current time */
        int32_t newTime;
        rc = Lwm2mParser_getTime(parser_ptr, &newTime);
        if (rc != RC_OK)
        {
            return (rc);
        }

        SetUtcTime(newTime);

        /* URI values from array by index:              /<ObjectId>		    /<ObjectId-Inst>	 /<ResourceId> */
        if (started)
        {
            Lwm2m_URI_Path_T currentTimeUriPath = { OBJECTS_IX_DEVICE_0, OBJECTS_IX_DEVICE_0, DEVICE_RESOURCES_INDEX(currentTime) };
            Lwm2mReporting_resourceChanged(&currentTimeUriPath);
        }
        printf("currentTime adjusted\r\n");
        return (RC_OK);
    }

#ifdef USE_LOG_FOR_DEVICE
    printf("currentTime\r\n");
#endif
    return (Lwm2mSerializer_serializeTime(serializer_ptr, GetUtcTime()));
}

/* @brief
 * This function is used to update the tzone Resource value into the LWM2M Server*
 */
static retcode_t getTimeZone(Lwm2mSerializer_T *serializer_ptr, Lwm2mParser_T *parser_ptr)
{
    StringDescr_T strDescr_tz;

#ifdef USE_LOG_FOR_DEVICE
    printf("tzone\r\n");
#endif
    if (parser_ptr == NULL)
    {
        StringDescr_set(&strDescr_tz, (char*) tz, strlen(tz));
        return (Lwm2mSerializer_serializeString(serializer_ptr, &strDescr_tz));
    }
    else
    {
        retcode_t rc;

        rc = Lwm2mParser_getString(parser_ptr, &strDescr_tz);
        if (rc != RC_OK)
        {
            return (rc);
        }
        if ((0 > strDescr_tz.length) || ((uint32_t) strDescr_tz.length >= sizeof(tz)))
        {
            return (RC_LWM2M_INTERNAL_ERROR);
        }
        strncpy(tz, strDescr_tz.start, strDescr_tz.length);
        tz[strDescr_tz.length] = '\0';
        if (started)
        {
            Lwm2m_URI_Path_T timeZoneUriPath = { OBJECTS_IX_DEVICE_0, OBJECTS_IX_DEVICE_0, DEVICE_RESOURCES_INDEX(Timezone) };
            Lwm2mReporting_resourceChanged(&timeZoneUriPath);
        }
        return (RC_OK);
    }
}
/* @brief
 * This function is used to update the getBinding Resource value into the LWM2M Server*
 */
static retcode_t getBindingFunc(Lwm2mSerializer_T *serializer_ptr, Lwm2mParser_T *parser_ptr)
{
    if (parser_ptr != NULL)
    {
        return (RC_LWM2M_METHOD_NOT_ALLOWED);
    }

    char* binding = "";
    uint8_t length = 0;

#ifdef USE_LOG_FOR_DEVICE
    printf("getBinding\r\n");
#endif
    switch (DeviceResourceInfo.binding)
    {
    case UDP:
        binding = "U";
        length = LWM2MOBJECTS_UDP_LENGTH;
        break;

    case UDP_QUEUED:
        binding = "UQ";
        length = LWM2MOBJECTS_UDP_QUEUED_LENGTH;
        break;

    case SMS:
        binding = "S";
        length = LWM2MOBJECTS_SMS_LENGTH;
        break;

    case SMS_QUEUED:
        binding = "SQ";
        length = LWM2MOBJECTS_SMS_QUEUED_LENGTH;
        break;

    case UDP_AND_SMS:
        binding = "US";
        length = LWM2MOBJECTS_UDP_AND_SMS_LENGTH;
        break;

    case UDP_QUEUED_AND_SMS:
        binding = "UQS";
        length = LWM2MOBJECTS_UDP_QUEUED_AND_SMS_LENGTH;
        break;
    }

    StringDescr_T strDescr;

    StringDescr_set(&strDescr, binding, length);

    return (Lwm2mSerializer_serializeString(serializer_ptr, &strDescr));
}
/* TODO need a way to define inactive resources. */
DeviceResource_T DeviceResources =
        { //   R-ID!
        { 0, LWM2M_STRING_RO( "Bosch BCDS" ) },
                { 1, LWM2M_STRING_RO( "XDK110" ) },
                { 2, LWM2M_DYNAMIC( serialNumber_RO ) | LWM2M_READ_ONLY },
                { 3, LWM2M_DYNAMIC( firmwareVersionNumber_RO ) | LWM2M_READ_ONLY },
                { 4, LWM2M_FUNCTION( deviceRebootFunc ) },
                { 5, LWM2M_FUNCTION( factoryResetFunc ) },
                { 11, LWM2M_DYNAMIC_ARRAY( errorCodeFunc ) | LWM2M_READ_ONLY },
                { 12, LWM2M_FUNCTION( resetErrorCodeFunc ) },
                { 13, LWM2M_DYNAMIC( getCurrentTime ) },
                { 14, LWM2M_STRING_RO( "UTC+2" ) },
                { 15, LWM2M_DYNAMIC( getTimeZone ) },
                { 16, LWM2M_DYNAMIC( getBindingFunc ) | LWM2M_READ_ONLY },
        };

/* Here URI_Path is the path points to the "Current Time" Resource */

void ObjectDevice_Init(void)
{
    started = false;
}

void ObjectDevice_Enable(void)
{
    started = true;
}

void ObjectDevice_NotifyTimeChanged(void)
{
    if (started)
    {
        Lwm2m_URI_Path_T currentTimeUriPath = { OBJECTS_IX_DEVICE_0, OBJECTS_IX_DEVICE_0, DEVICE_RESOURCES_INDEX(currentTime) };
        retcode_t rc = Lwm2mReporting_resourceChanged(&currentTimeUriPath);
        if (RC_OK != rc && RC_COAP_SERVER_SESSION_ALREADY_ACTIVE != rc)
        {
            LOG_DEBUG("Could not send time notification " RC_RESOLVE_FORMAT_STR, RC_RESOLVE(rc));
        }
    }
}
