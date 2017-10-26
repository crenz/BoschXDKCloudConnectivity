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
#include "XDKAppInfo.h"

#undef BCDS_MODULE_ID  /* Module ID define before including Basics package*/
#define BCDS_MODULE_ID XDK_APP_MODULE_ID_LWM2M_OBJ_SENSOR_DEVICE

#include "SensorDeviceAccelerometer.h"
#include "SensorDeviceEnvironment.h"
#include "SensorDeviceGyroscope.h"
#include "SensorDeviceMagnetometer.h"
#include "SensorDeviceIlluminance.h"
#include "Lwm2mObject_SensorDevice.h"
#include "Lwm2mObjects.h"
#include "Lwm2mUtil.h"

/* additional interface header files */
#include "BCDS_Basics.h"
#include "BCDS_CmdProcessor.h"

#include "XdkSensorHandle.h"
#include <Serval_Exceptions.h>
#include <Serval_Clock.h>
#include <Serval_Lwm2m.h>

/* time increased from 2000 to 15000 for cloud use-case */
#define DEFAULT_SENSORS_TRANSPORT_INTERVAL UINT32_C(15000)
#define DEFAULT_SENSORS_MODE AVG

#define SENSORS_READ_INTERVAL UINT32_C(100)

#define MAX_MODE_LENGTH UINT8_C(8)
#define MAC_LENGTH      UINT8_C(18)

#define SENSORDEVICE_RESOURCES_INDEX(res) LWM2M_RESOURCES_INDEX(SensorDeviceResources, res)

static retcode_t Interval(Lwm2mSerializer_T *serializer_ptr, Lwm2mParser_T *parser_ptr);
static retcode_t PreprocessingMode(Lwm2mSerializer_T *serializer_ptr, Lwm2mParser_T *parser_ptr);

SensorDeviceResource_T SensorDeviceResources =
        {
                { 0, LWM2M_DYNAMIC(Interval) },
                { 1, LWM2M_BOOL(true) },
                { 2, LWM2M_BOOL(true) },
                { 3, LWM2M_BOOL(true) },
                { 4, LWM2M_BOOL(true) },
                { 5, LWM2M_BOOL(true) },
                { 6, LWM2M_BOOL(true) },
                { 7, LWM2M_BOOL(true) },
                { 8, LWM2M_DYNAMIC(PreprocessingMode) },
                { 9, LWM2M_STRING_RO("") },
                };

/*lint -e(956) only accessed by timer */
static uint64_t LastTransportTime = 0; /** time of last sending information */
/*lint -e(956) only accessed by timer */
static int32_t LastTransportInterval = DEFAULT_SENSORS_TRANSPORT_INTERVAL; /** last time for sending sensor values in ms */
/*lint -e(956) only accessed by timer */
static ProcessingMode_T LastMode = DEFAULT_SENSORS_MODE;

/*lint -e(956) mutex used */
static int32_t TransportInterval = DEFAULT_SENSORS_TRANSPORT_INTERVAL; /** time for sending sensor values in ms */
/*lint -e(956) mutex used */
static ProcessingMode_T Mode = DEFAULT_SENSORS_MODE;

/*lint -e(956) */
static xTimerHandle TimerHandle = NULL;/**< Timer handle for periodically reading sensor values */

static LWM2M_MUTEX_INSTANCE(Mutex) = LWM2M_MUTEX_INIT_VALUE;
static volatile bool Started = false;

/*Handler to update the timer objects*/
void SensorTimerHandler(void * param1, uint32_t data)
{
    BCDS_UNUSED(param1);
    BCDS_UNUSED(data);
    retcode_t retVal = RC_OK;
    uint64_t Millis;
    bool Notify = false;
    bool Start = false;
    int32_t CurrentTransportInterval = LastTransportInterval;
    ProcessingMode_T CurrentMode = LastMode;

    retVal = Clock_getTimeMillis(&Millis);
    if (RC_OK != retVal)
    {
        printf("Fatal Error in Getting time inside the SensorTimerHandler() Handler \r\n ");
    }
    else
    {
        if (LWM2M_MUTEX_LOCK_TIMEOUT(Mutex, 50))
        {
            CurrentTransportInterval = TransportInterval;
            CurrentMode = Mode;
            LWM2M_MUTEX_UNLOCK(Mutex);
        }

        if (LastMode != CurrentMode)
        {
            LastMode = CurrentMode;
            Start = true;
        }
        if (LastTransportInterval != CurrentTransportInterval)
        {
            LastTransportInterval = CurrentTransportInterval;
            Start = true;
        }
        if (Start)
        {
            LastTransportTime = Millis;
        }
        else
        {
            Notify = (LastTransportTime + LastTransportInterval) < Millis;
            if (Notify)
                LastTransportTime = Millis;
        }

        SensorDeviceAccelerometer_Activate(SensorDeviceResources.accelerometerEnabled.data.b);
        if (SensorDeviceResources.accelerometerEnabled.data.b)
        {
            SensorDeviceAccelerometer_Update(CurrentMode, Notify);
        }
        SensorDeviceGyroscope_Activate(SensorDeviceResources.gyroscopeEnabled.data.b);
        if (SensorDeviceResources.gyroscopeEnabled.data.b)
        {
            SensorDeviceGyroscope_Update(CurrentMode, Notify);
        }
        SensorDeviceMagnetometer_Activate(SensorDeviceResources.magnetometerEnabled.data.b);
        if (SensorDeviceResources.magnetometerEnabled.data.b)
        {
            SensorDeviceMagnetometer_Update(CurrentMode, Notify);
        }
        SensorDeviceIlluminance_Activate(SensorDeviceResources.illuminanceSensorEnabled.data.b);
        if (SensorDeviceResources.illuminanceSensorEnabled.data.b)
        {
            SensorDeviceIlluminance_Update(CurrentMode, Notify);
        }
        SensorDeviceEnvironment_Activate(SensorDeviceResources.temperatureSensorEnabled.data.b,
                SensorDeviceResources.humiditySensorEnabled.data.b,
                SensorDeviceResources.barometerEnabled.data.b);
        if (SensorDeviceResources.barometerEnabled.data.b
                || SensorDeviceResources.temperatureSensorEnabled.data.b
                || SensorDeviceResources.humiditySensorEnabled.data.b)
        {
            SensorDeviceEnvironment_Update(CurrentMode, Notify);
        }
    }

}

static void SensorTimer(xTimerHandle xTimer)
{
    BCDS_UNUSED(xTimer);
    SensorTimerHandler(NULL,0);
}

static retcode_t Interval(Lwm2mSerializer_T *serializer_ptr, Lwm2mParser_T *parser_ptr)
{
    if (NULL != parser_ptr)
    {
        /* write value */
        int32_t value;
        retcode_t rc = Lwm2mParser_getInt(parser_ptr, &value);
        if (RC_OK == rc && value != TransportInterval)
        {
            if (LWM2M_MUTEX_LOCK(Mutex))
            {
                TransportInterval = value;
                LWM2M_MUTEX_UNLOCK(Mutex);
                printf("tranport interval changed: %ld\n", (long) value);
                if (Started)
                {
                    Lwm2m_URI_Path_T path = { OBJECTS_IX_SENSORDEVICE_0, OBJECTS_IX_SENSORDEVICE_0, SENSORDEVICE_RESOURCES_INDEX(transportInterval) };
                    /*lint -e(534) purposefully suppressing the warning:Ignoring return value of function,since in CONF mode this may fail many times */
                    Lwm2mReporting_resourceChanged(&path);
                }
            }
        }
        return rc;
    }
    else if ( NULL != serializer_ptr)
    {
        /* read value */
        return (Lwm2mSerializer_serializeInt(serializer_ptr, TransportInterval));
    }
    return (RC_LWM2M_METHOD_NOT_ALLOWED);
}

static retcode_t PreprocessingMode(Lwm2mSerializer_T *serializer_ptr, Lwm2mParser_T *parser_ptr)
{
    if (NULL != parser_ptr)
    {
        /* write value */

        StringDescr_T strDescr;
        retcode_t rc = Lwm2mParser_getString(parser_ptr, &strDescr);
        if (RC_OK == rc)
        {
            enum ProcessingMode newMode = CURRENT;
            if (StringDescr_compare(&strDescr, "current"))
                newMode = CURRENT;
            else if (StringDescr_compare(&strDescr, "max"))
                newMode = MAX;
            else if (StringDescr_compare(&strDescr, "min"))
                newMode = MIN;
            else if (StringDescr_compare(&strDescr, "avg"))
                newMode = AVG;
            else
                return RC_LWM2M_BAD_REQUEST;
            if (Mode != newMode)
            {
                if (LWM2M_MUTEX_LOCK(Mutex))
                {
                    Mode = newMode;
                    LWM2M_MUTEX_UNLOCK(Mutex);
                    if (Started)
                    {
                        Lwm2m_URI_Path_T path = { OBJECTS_IX_SENSORDEVICE_0, OBJECTS_IX_SENSORDEVICE_0, SENSORDEVICE_RESOURCES_INDEX(preprocessingMode) };
                        /*lint -e(534) purposefully suppressing the warning:Ignoring return value of function,since in CONF mode this may fail many times */
                        Lwm2mReporting_resourceChanged(&path);
                    }
                }
            }
        }
        return rc;
    }
    else if ( NULL != serializer_ptr)
    {
        /* read value */
        StringDescr_T strDescr;
        const char * mode = "current";
        switch (Mode)
        {
        case CURRENT:
            mode = "current";
            break;
        case MAX:
            mode = "max";
            break;
        case MIN:
            mode = "min";
            break;
        case AVG:
            mode = "avg";
            break;
        }
        StringDescr_set(&strDescr, mode, strlen(mode));
        return (Lwm2mSerializer_serializeString(serializer_ptr, &strDescr));
    }
    return (RC_LWM2M_METHOD_NOT_ALLOWED);
}

void ObjectSensorDevice_Init(const char* mac)
{
    configASSERT(portTICK_RATE_MS);

    Started = false;
    LWM2M_MUTEX_CREATE(Mutex);
    uint32_t Ticks = SENSORS_READ_INTERVAL / portTICK_RATE_MS;

    /* create timer task for sending data to lwm2m */
    TimerHandle = xTimerCreate((const char * const ) "sensors", Ticks, pdTRUE, NULL, SensorTimer);

    /* timer create fail case */
    if (TimerHandle == NULL)
    {
        assert(false); /* "This software timer was not Created, Due to Insufficient heap memory"); */
    }
    SensorDeviceResources.macAddress.data.s = (char*) mac;
}

void ObjectSensorDevice_Enable(void)
{
    Started = true;
    /*start the timer*/
    if (pdPASS != xTimerStart(TimerHandle, 5000 / portTICK_RATE_MS))
    {
        assert(false);
    }
}

Retcode_T ObjectSensorDevice_Disable(void)
{
    Started = false;
    Retcode_T returnVal = RETCODE_OK;
   /*Stop the sensor sampling timer*/
    if (pdPASS != xTimerStop(TimerHandle, 1000 / portTICK_RATE_MS))
    {
    	returnVal =RETCODE(RETCODE_SEVERITY_ERROR,RETCODE_RTOS_QUEUE_ERROR);
    }
    return (returnVal);
}
