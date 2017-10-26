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
* @defgroup SENSOR_DEVICE_ENVIRONMENT Sensor Device Environment
* @{
*
* @brief This file provides the Implementation of SensorDeviceEnvironment.
*
* @details Environmental Sensor Device processing source file
*
* @file
**/

/* own header files */
#include "Lwm2mObject_Barometer.h"
#include "Lwm2mObject_Temperature.h"
#include "Lwm2mObject_Humidity.h"
#include "SensorDeviceEnvironment.h"

/* additional interface header files */
#include "XdkSensorHandle.h"

#define PRESSURE_TO_HPA_FLOAT(I)  ((float)(I) / 100.0F)		/* from integer Pa */
#define TEMPERATURE_TO_C_FLOAT(I) ((float)(I) / 1000.0F)	/* from integer mDeg */
#define HUMIDITY_TO_PERC_FLOAT(I) ((float)(I))				/* from integer %rH */

/* current impl.: one structure for sensor -> one enable! */
static SensorDeviceProcessDataFloat_T TemperatureSensorDeviceData = { { { 0 } }, 1, 0, CURRENT, false };
static SensorDeviceProcessDataFloat_T HumiditySensorDeviceData = { { { 0 } }, 1, 0, CURRENT, false };
static SensorDeviceProcessDataFloat_T PressureSensorDeviceData = { { { 0 } }, 1, 0, CURRENT, false };
/*lint -esym(956, *EnableSensor) only to be used by sensor timer task */
static bool EnableSensor = false;

void SensorDeviceEnvironment_Activate(bool enableTemperature, bool enableHumidity, bool enablePressure)
{
    bool Enable = enableTemperature || enableHumidity || enablePressure;
    if (EnableSensor != Enable)
    {
        EnableSensor = Enable;
        if (Enable)
        {
            Retcode_T environReturnValue = Environmental_init(xdkEnvironmental_BME280_Handle);
            if (RETCODE_OK != environReturnValue)
            {
                enableTemperature = false;
                enableHumidity = false;
                enablePressure = false;
            }
        }
        else
        {
            if (RETCODE_OK != Environmental_deInit(xdkEnvironmental_BME280_Handle))
            {
                printf("Failed to disable the accelerometer\r\n");
            }
        }
    }

    if (TemperatureSensorDeviceData.enabled != enableTemperature)
    {
        TemperatureSensorDeviceData.enabled = enableTemperature;
        SensorDevice_ResetProcessDataFloat(&TemperatureSensorDeviceData);
        if (enableTemperature)
        {
            ObjectTemperature_Enable(-40.0F, 85.0F); // °C	ToDo define correct BME280 values!
        }
        else
        {
            ObjectTemperature_Disable();
        }
    }

    if (HumiditySensorDeviceData.enabled != enableHumidity)
    {
        HumiditySensorDeviceData.enabled = enableHumidity;
        SensorDevice_ResetProcessDataFloat(&HumiditySensorDeviceData);
        if (enableHumidity)
        {
            ObjectHumidity_Enable(0.0F, 100.0F); // %rH	ToDo define correct BME280 values!
        }
        else
        {
            ObjectHumidity_Disable();
        }
    }

    if (PressureSensorDeviceData.enabled != enablePressure)
    {
        PressureSensorDeviceData.enabled = enablePressure;
        SensorDevice_ResetProcessDataFloat(&PressureSensorDeviceData);
        if (enablePressure)
        {
            ObjectBarometer_Enable(300.0F, 1100.0F); // hPa
        }
        else
        {
            ObjectBarometer_Disable();
        }
    }
}

void SensorDeviceEnvironment_Update(enum ProcessingMode mode, bool notify)
{
    Retcode_T ReturnValue = RETCODE_OK;
    SensorDeviceSampleDataFloat_T Sample;
    Environmental_Data_T EnvDataHolder = { INT32_C(0), UINT32_C(0), UINT32_C(0) };

    if (!EnableSensor)
        return;

    /* Read temperature, pressure, humidity current values */
    ReturnValue = Environmental_readData(xdkEnvironmental_BME280_Handle, &EnvDataHolder);

    if ( RETCODE_OK == ReturnValue)
    {
        if (TemperatureSensorDeviceData.enabled)
        {
            Sample.values[0] = TEMPERATURE_TO_C_FLOAT(EnvDataHolder.temperature);
            SensorDevice_ProcessDataFloat(mode, &TemperatureSensorDeviceData, &Sample);
        }
        if (HumiditySensorDeviceData.enabled)
        {
            Sample.values[0] = HUMIDITY_TO_PERC_FLOAT(EnvDataHolder.humidity);
            SensorDevice_ProcessDataFloat(mode, &HumiditySensorDeviceData, &Sample);
        }
        if (PressureSensorDeviceData.enabled)
        {
            Sample.values[0] = PRESSURE_TO_HPA_FLOAT(EnvDataHolder.pressure);
            SensorDevice_ProcessDataFloat(mode, &PressureSensorDeviceData, &Sample);
        }
    }
    else
    {
        printf("Environmental Read actual Data Failed\n\r");
    }
    if (notify)
    {
        if (SensorDevice_GetDataFloat(&TemperatureSensorDeviceData, &Sample))
            ObjectTemperature_SetValue((float) Sample.values[0]);
        if (SensorDevice_GetDataFloat(&HumiditySensorDeviceData, &Sample))
            ObjectHumidity_SetValue((float) Sample.values[0]);
        if (SensorDevice_GetDataFloat(&PressureSensorDeviceData, &Sample))
            ObjectBarometer_SetValue((float) Sample.values[0]);
    }
}

/**@} */
