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
* @defgroup SENSOR_DEVICE_GYROSCOPE Sensor Device Gyroscope
* @{
*
* @brief This file provides the Implementation of SensorDeviceGyroscope.
*
* @details Gyroscope Sensor Device processing source file
*
* @file
**/
 
/* own header files */
#include "Lwm2mObject_Gyroscope.h"
#include "SensorDeviceGyroscope.h"

/* additional interface header files */
#include "XdkSensorHandle.h"
#include <Serval_Lwm2m.h>

#define GYROSCOPE_TO_FLOAT(I) ((I) / 1000.0F)

static SensorDeviceProcessDataFloat_T SensorDeviceData = { { { 0 } }, 3, 0, CURRENT, false };

void SensorDeviceGyroscope_InitRand(void)
{
    bool Deinit = false;
    unsigned int Init = 0;
    Retcode_T ReturnValue = RETCODE_OK;
    Gyroscope_XyzData_T GetMdegData = { INT32_C(0), INT32_C(0), INT32_C(0) };

    if (true != xdkGyroscope_BMG160_Handle->sensorInfo.initializationStatus)
    {
        ReturnValue = Gyroscope_init(xdkGyroscope_BMG160_Handle);
        if (RETCODE_OK != ReturnValue)
            return;
        Deinit = true;
        ReturnValue = Gyroscope_readXyzDegreeValue(xdkGyroscope_BMG160_Handle, &GetMdegData);
    }

    if (RETCODE_OK == ReturnValue)
    {
        while (0 == Init)
        {
            ReturnValue = Gyroscope_readXyzDegreeValue(xdkGyroscope_BMG160_Handle, &GetMdegData);
            if (RETCODE_OK == ReturnValue)
            {
                Init = (unsigned int) (GetMdegData.xAxisData + GetMdegData.yAxisData + GetMdegData.zAxisData);
            }
        }
        printf("srand(%u)\r\n", Init);
        srand(Init);
    }
    if (Deinit)
    {
        ReturnValue = Gyroscope_deInit(xdkGyroscope_BMG160_Handle);
        if (RETCODE_OK != ReturnValue)
        {
            printf("Disable gyroscope failed!\n");
        }
    }
}

void SensorDeviceGyroscope_Activate(bool enable)
{
    if (SensorDeviceData.enabled == enable)
        return;
    SensorDeviceData.enabled = enable;
    SensorDevice_ResetProcessDataFloat(&SensorDeviceData);
    if (enable)
    {
        float ValueRange = 0.0F;
        Gyroscope_Range_T SensorRange;
        Retcode_T ReturnValue = Gyroscope_init(xdkGyroscope_BMG160_Handle);
        if (RETCODE_OK != ReturnValue)
            return;

        ReturnValue = Gyroscope_getRange(xdkGyroscope_BMG160_Handle, &SensorRange);
        if (RETCODE_OK != ReturnValue)
            return;

        switch (SensorRange)
        {
        case GYROSCOPE_BMG160_RANGE_125s: /**< set to 0.0625 deg/s in 125 deg/s range */
            ValueRange = 125.;
            break;
        case GYROSCOPE_BMG160_RANGE_250s: /**< set to 0.125 deg/s in 250 deg/s range */
            ValueRange = 250.;
            break;
        case GYROSCOPE_BMG160_RANGE_500s: /**< set to 0.25 deg/s in 500 deg/s range */
            ValueRange = 500.;
            break;
        case GYROSCOPE_BMG160_RANGE_1000s: /**< set to 0.5 deg/s in 1000 deg/s range */
            ValueRange = 1000.;
            break;
        case GYROSCOPE_BMG160_RANGE_2000s: /**< set to 1 deg/s in 2000 deg/s range */
            ValueRange = 2000.;
            break;
        default:
            break;
        }
        ObjectGyroscope_Enable(-ValueRange, ValueRange);
    }
    else
    {
        if (RETCODE_OK != Gyroscope_deInit(xdkGyroscope_BMG160_Handle))
        {
            printf("Disable gyroscope failed!\n");
        }
        ObjectGyroscope_Disable();
    }
}

void SensorDeviceGyroscope_Update(enum ProcessingMode mode, bool notify)
{
    if (!SensorDeviceData.enabled)
        return;

    SensorDeviceSampleDataFloat_T Sample;
    Gyroscope_XyzData_T GetMdegData = { INT32_C(0), INT32_C(0), INT32_C(0) };

    Retcode_T AdvancedApiRetValue = Gyroscope_readXyzDegreeValue(xdkGyroscope_BMG160_Handle, &GetMdegData);

    if (RETCODE_OK == AdvancedApiRetValue)
    {
        Sample.values[0] = GYROSCOPE_TO_FLOAT(GetMdegData.xAxisData);
        Sample.values[1] = GYROSCOPE_TO_FLOAT(GetMdegData.yAxisData);
        Sample.values[2] = GYROSCOPE_TO_FLOAT(GetMdegData.zAxisData);
        SensorDevice_ProcessDataFloat(mode, &SensorDeviceData, &Sample);
    }
    else
    {
    	printf("Error in Gyro Read \r\n");
    }
    if (notify)
    {
        if (SensorDevice_GetDataFloat(&SensorDeviceData, &Sample))
            ObjectGyroscope_SetValues((float) Sample.values[0], (float) Sample.values[1], (float) Sample.values[2]);
    }
}

/**@} */
