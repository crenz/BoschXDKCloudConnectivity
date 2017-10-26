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
#include "Lwm2mUtil.h"
#include "Lwm2mInterface.h"

/* additional interface header files */

#define GYROSCOPE_RESOURCES_INDEX(res) LWM2M_RESOURCES_INDEX(GyroscopeResources, res)

#define FLUSH_RESOURCES	Lwm2mReporting_multipleResourcesChanged(&GyroscopeUriPath, 6, \
			GYROSCOPE_RESOURCES_INDEX(minRangeValue),\
			GYROSCOPE_RESOURCES_INDEX(maxRangeValue),\
			GYROSCOPE_RESOURCES_INDEX(units),\
			GYROSCOPE_RESOURCES_INDEX(xValue),\
			GYROSCOPE_RESOURCES_INDEX(yValue),\
			GYROSCOPE_RESOURCES_INDEX(zValue))

static void ObjectGyroscope_InternalEnable(float minRangeValue, float maxRangeValue);
static void ObjectGyroscope_InternalDisable(void);
static void ObjectGyroscope_InternalSetValues(float xValue, float yValue, float zValue);

/*lint -e(956) const ?*/
static Lwm2m_URI_Path_T GyroscopeUriPath = { OBJECTS_IX_GYROSCOPE_0, OBJECTS_IX_GYROSCOPE_0, -1 };

static Lwm2m_Pair_Resource_Update_T AsyncCall_Enabler = { .set_pair = ObjectGyroscope_InternalEnable, .mutex = LWM2M_MUTEX_INIT_VALUE };
static Lwm2m_Call_T AsyncCall_Disabler = { .call = ObjectGyroscope_InternalDisable };
static Lwm2m_Tripple_Resource_Update_T AsyncCall_Updater = { .set_tripple = ObjectGyroscope_InternalSetValues, .mutex = LWM2M_MUTEX_INIT_VALUE };
static volatile bool Started = false;

/* TODO need a way to define inactive resources. */
GyroscopeResource_T GyroscopeResources =
        {
                { 5603, LWM2M_FLOAT(0.0F) | LWM2M_READ_ONLY }, // minRangeValue|R|Single|O|Float |   |
                { 5604, LWM2M_FLOAT(0.0F) | LWM2M_READ_ONLY }, // minRangeValue|R|Single|O|Float |   |
                { 5701, LWM2M_STRING_RO("") }, // units        |R|Single|O|String|   |
                { 5702, LWM2M_FLOAT(0.0F) | LWM2M_READ_ONLY }, // xValue 		|R|Single|M|Float |   |
                { 5703, LWM2M_FLOAT(0.0F) | LWM2M_READ_ONLY }, // yValue       |R|Single|O|Float |   | Optional really?
                { 5704, LWM2M_FLOAT(0.0F) | LWM2M_READ_ONLY }, // zValue       |R|Single|O|Float |   | Optional really?
        };

static void ObjectGyroscope_InternalEnable(float minRangeValue, float maxRangeValue)
{
    Started = true;
    GyroscopeResources.minRangeValue.data.f = minRangeValue;
    GyroscopeResources.maxRangeValue.data.f = maxRangeValue;
    GyroscopeResources.units.data.s = "Deg/s";
    GyroscopeResources.xValue.data.f = minRangeValue;
    GyroscopeResources.yValue.data.f = minRangeValue;
    GyroscopeResources.zValue.data.f = minRangeValue;
    /*lint -e(534) purposefully suppressing the warning:Ignoring return value of function,since in CONF mode this may fail many times */
    FLUSH_RESOURCES;
}

static void ObjectGyroscope_InternalDisable()
{
    Started = true;
    GyroscopeResources.minRangeValue.data.f = 0.0F;
    GyroscopeResources.maxRangeValue.data.f = 0.0F;
    GyroscopeResources.units.data.s = "";
    GyroscopeResources.xValue.data.f = 0.0F;
    GyroscopeResources.yValue.data.f = 0.0F;
    GyroscopeResources.zValue.data.f = 0.0F;
    /*lint -e(534) purposefully suppressing the warning:Ignoring return value of function,since in CONF mode this may fail many times */
    FLUSH_RESOURCES;
}

static void ObjectGyroscope_InternalSetValues(float xValue, float yValue, float zValue)
{
    INIT_LWM2M_DYNAMIC_CHANGES(changes);

    LWM2M_DYNAMIC_CHANGES_SET_FLOAT_VALUE(changes, GyroscopeResources, xValue, xValue);
    LWM2M_DYNAMIC_CHANGES_SET_FLOAT_VALUE(changes, GyroscopeResources, yValue, yValue);
    LWM2M_DYNAMIC_CHANGES_SET_FLOAT_VALUE(changes, GyroscopeResources, zValue, zValue);
    if (Started)
        LWM2M_DYNAMIC_CHANGES_REPORT(changes, GyroscopeUriPath);
}

/* global functions ********************************************************* */
void ObjectGyroscope_Init(void)
{
    Started = false;
    LWM2M_MUTEX_CREATE(AsyncCall_Updater.mutex);
    AsyncCall_Enabler.mutex = AsyncCall_Updater.mutex;
}

void ObjectGyroscope_Enable(float minRangeValue, float maxRangeValue)
{
    Lwm2mUtil_UpdatePairResources(minRangeValue, maxRangeValue, &AsyncCall_Enabler);
}

void ObjectGyroscope_Disable()
{
    Lwm2mUtil_Schedule(&AsyncCall_Disabler);
}

void ObjectGyroscope_SetValues(float xValue, float yValue, float zValue)
{
    Lwm2mUtil_UpdateTrippleResources(xValue, yValue, zValue, &AsyncCall_Updater);
}
