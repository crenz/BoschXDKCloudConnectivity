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
#include "Lwm2mObject_Accelerometer.h"
#include "Lwm2mObjects.h"
#include "Lwm2mUtil.h"

/* additional interface header files */
#include <Serval_Exceptions.h>
#include <Serval_Lwm2m.h>

#define ACCELEROMETER_RESOURCES_INDEX(res) LWM2M_RESOURCES_INDEX(AccelerometerResources, res)

#define FLUSH_RESOURCES	Lwm2mReporting_multipleResourcesChanged(&AccelerometerUriPath, 6, \
			ACCELEROMETER_RESOURCES_INDEX(units),\
			ACCELEROMETER_RESOURCES_INDEX(minRange),\
			ACCELEROMETER_RESOURCES_INDEX(maxRange),\
			ACCELEROMETER_RESOURCES_INDEX(xAxis),\
			ACCELEROMETER_RESOURCES_INDEX(yAxis),\
			ACCELEROMETER_RESOURCES_INDEX(zAxis))

static void ObjectAccelerometer_InternalEnable(float minRange, float maxRange);
static void ObjectAccelerometer_InternalDisable(void);
static void ObjectAccelerometer_InternalSetValues(float xAxis, float yAxis, float zAxis);

/*lint -e(956) const ?*/
static Lwm2m_URI_Path_T AccelerometerUriPath = { OBJECTS_IX_ACCELEROMETER_0, OBJECTS_IX_ACCELEROMETER_0, -1 };

static Lwm2m_Pair_Resource_Update_T AsyncCall_Enabler = { .set_pair = ObjectAccelerometer_InternalEnable, .mutex = LWM2M_MUTEX_INIT_VALUE };
static Lwm2m_Call_T AsyncCall_Disabler = { .call = ObjectAccelerometer_InternalDisable };
static Lwm2m_Tripple_Resource_Update_T AsyncCall_Updater = { .set_tripple = ObjectAccelerometer_InternalSetValues, .mutex = LWM2M_MUTEX_INIT_VALUE };
static volatile bool Started = false;

AccelerometerResource_T AccelerometerResources =
        {
                { 5603, LWM2M_FLOAT(0.0F) | LWM2M_READ_ONLY },
                { 5604, LWM2M_FLOAT(0.0F) | LWM2M_READ_ONLY },
                { 5701, LWM2M_STRING_RO("") },
                { 5702, LWM2M_FLOAT(0.0F) | LWM2M_READ_ONLY },
                { 5703, LWM2M_FLOAT(0.0F) | LWM2M_READ_ONLY },
                { 5704, LWM2M_FLOAT(0.0F) | LWM2M_READ_ONLY },
        };

static void ObjectAccelerometer_InternalEnable(float minRange, float maxRange)
{
    Started = true;
    AccelerometerResources.units.data.s = "g";
    AccelerometerResources.minRange.data.f = minRange;
    AccelerometerResources.maxRange.data.f = maxRange;
    AccelerometerResources.xAxis.data.f = minRange;
    AccelerometerResources.yAxis.data.f = minRange;
    AccelerometerResources.zAxis.data.f = minRange;
    /*lint -e(534) purposefully suppressing the warning:Ignoring return value of function, since in CONF mode this may fail many times */
    FLUSH_RESOURCES;
}


static void ObjectAccelerometer_InternalDisable(void)
{
    Started = true;
    AccelerometerResources.units.data.s = "";
    AccelerometerResources.minRange.data.f = 0.0F;
    AccelerometerResources.maxRange.data.f = 0.0F;
    AccelerometerResources.xAxis.data.f = 0.0F;
    AccelerometerResources.yAxis.data.f = 0.0F;
    AccelerometerResources.zAxis.data.f = 0.0F;
    /*lint -e(534) purposefully suppressing the warning:Ignoring return value of function,since in CONF mode this may fail many times */
    FLUSH_RESOURCES;
}

static void ObjectAccelerometer_InternalSetValues(float xAxis, float yAxis, float zAxis)
{
    INIT_LWM2M_DYNAMIC_CHANGES(changes);

    LWM2M_DYNAMIC_CHANGES_SET_FLOAT_VALUE(changes, AccelerometerResources, xAxis, xAxis);
    LWM2M_DYNAMIC_CHANGES_SET_FLOAT_VALUE(changes, AccelerometerResources, yAxis, yAxis);
    LWM2M_DYNAMIC_CHANGES_SET_FLOAT_VALUE(changes, AccelerometerResources, zAxis, zAxis);
    if (Started)
        LWM2M_DYNAMIC_CHANGES_REPORT(changes, AccelerometerUriPath);
}

void ObjectAccelerometer_Init(void)
{
    Started = false;
    LWM2M_MUTEX_CREATE(AsyncCall_Updater.mutex);
    AsyncCall_Enabler.mutex = AsyncCall_Updater.mutex;
}

void ObjectAccelerometer_Enable(float minRange, float maxRange)
{
    Lwm2mUtil_UpdatePairResources(minRange, maxRange, &AsyncCall_Enabler);
}

void ObjectAccelerometer_Disable(void)
{
    Lwm2mUtil_Schedule(&AsyncCall_Disabler);
}

void ObjectAccelerometer_SetValues(float xAxis, float yAxis, float zAxis)
{
    Lwm2mUtil_UpdateTrippleResources(xAxis, yAxis, zAxis, &AsyncCall_Updater);
}
