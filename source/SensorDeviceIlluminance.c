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
* @defgroup SENSOR_DEVICE_ILLUMINANCE Sensor Device Illuminance
* @{
*
* @brief This file provides the Implementation of SensorDeviceIlluminance.
*
* @details Illuminance Sensor Device processing source file
*
* @file
**/

/* own header files */
#include "Lwm2mObject_Illuminance.h"
#include "SensorDeviceIlluminance.h"

/* system header files */


/* additional interface header files */
#include "XdkSensorHandle.h"
#include <Serval_Lwm2m.h>

static SensorDeviceProcessDataFloat_T SensorDeviceData = { { { 0 } }, 1, 0, CURRENT, false };

void SensorDeviceIlluminance_Activate(bool enable)
{
    if (SensorDeviceData.enabled == enable)
        return;
    SensorDeviceData.enabled = enable;
    SensorDevice_ResetProcessDataFloat(&SensorDeviceData);
    if (enable)
    {
        float MinRange = 0.0F;
        float MaxRange = 188000000.F;
        Retcode_T ReturnValue = LightSensor_init(xdkLightSensor_MAX44009_Handle);
        vTaskDelay((portTickType) 200 / portTICK_RATE_MS);
        if (RETCODE_OK != ReturnValue)
            return;
        ObjectIlluminance_Enable(MinRange, MaxRange);
    }
    else
    {
        if (RETCODE_OK != LightSensor_deInit(xdkLightSensor_MAX44009_Handle))
        {
            printf("Disable light sensor failed!\n");
        }
        ObjectIlluminance_Disable();
    }
}

void SensorDeviceIlluminance_Update(enum ProcessingMode mode, bool notify)
{
    if (!SensorDeviceData.enabled)
        return;

    SensorDeviceSampleDataFloat_T Sample;
    uint32_t GetLightData = INT32_C(0);

    Retcode_T AdvancedApiRetValue = LightSensor_readLuxData(xdkLightSensor_MAX44009_Handle, &GetLightData);
    if (RETCODE_OK == AdvancedApiRetValue)
    {
        Sample.values[0] = (float) GetLightData;
        SensorDevice_ProcessDataFloat(mode, &SensorDeviceData, &Sample);
    }
    else
    {
    	printf("Error in Light sensor Read \r\n");
    }
    if (notify)
    {
        if (SensorDevice_GetDataFloat(&SensorDeviceData, &Sample))
            ObjectIlluminance_SetValue((float) Sample.values[0]);
    }
}

/**@} */
