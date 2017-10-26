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
#include <Serval_Clock.h>
#include <Serval_Exceptions.h>
#include <BCDS_Retcode.h>
#include "XdkSensorHandle.h"

/* global variables ********************************************************* */

static retcode_t ResetMinMaxValues(Lwm2mSerializer_T *serializer_ptr, Lwm2mParser_T *parser_ptr);
static void ObjectBarometer_InternalEnable(float minRangeValue, float maxRangeValue);
static void ObjectBarometer_InternalDisable(void);
static void ObjectBarometer_InternalSetValue(float sensorValue_hPa);

BarometerResource_T BarometerResources =
        {
                { 5601, LWM2M_FLOAT(0.0F) | LWM2M_READ_ONLY }, // minMeassuredValue|R|Single|O|Float |   |
                { 5602, LWM2M_FLOAT(0.0F) | LWM2M_READ_ONLY }, // maxMeassuredValue|R|Single|O|Float |   |
                { 5603, LWM2M_FLOAT(0.0F) | LWM2M_READ_ONLY }, // minRangeValue    |R|Single|O|Float |   |
                { 5604, LWM2M_FLOAT(0.0F) | LWM2M_READ_ONLY }, // maxRangeValue    |R|Single|O|Float |   |
                { 5605, LWM2M_FUNCTION(ResetMinMaxValues) }, // resetMinMaxValue |E|Single|O|      |   |
                { 5700, LWM2M_FLOAT(0.0F) | LWM2M_READ_ONLY }, // sensorValue      |R|Single|M|Float |   |
                { 5701, LWM2M_STRING_RO("" ) }, // units            |R|Single|O|String|   |see:http://unitsofmeasure.org/ucum.html
                };

/* local variables ********************************************************** */

/*lint -e(956) const ?*/
static Lwm2m_URI_Path_T BarometerUriPath = { OBJECTS_IX_BAROMETER_0, OBJECTS_IX_BAROMETER_0, -1 };

static Lwm2m_Pair_Resource_Update_T AsyncCall_Enabler = { .set_pair = ObjectBarometer_InternalEnable, .mutex = LWM2M_MUTEX_INIT_VALUE };
static Lwm2m_Call_T AsyncCall_Disabler = { .call = ObjectBarometer_InternalDisable };
static Lwm2m_Single_Resource_Update_T AsyncCall_Updater = { .set_single = ObjectBarometer_InternalSetValue, .mutex = LWM2M_MUTEX_INIT_VALUE };

/*lint -e(956) after initialization only accessed by serval scheduler */
static bool MinMaxInit = false;
static volatile bool Started = false;

/* constant definitions ***************************************************** */
#define BAROMETER_RESOURCE_INDEX(res) LWM2M_RESOURCES_INDEX(BarometerResources, res)

#define FLUSH_RESOURCES	Lwm2mReporting_multipleResourcesChanged(&BarometerUriPath, 6, \
		BAROMETER_RESOURCE_INDEX(minRangeValue),\
		BAROMETER_RESOURCE_INDEX(maxRangeValue),\
		BAROMETER_RESOURCE_INDEX(units),\
		BAROMETER_RESOURCE_INDEX(sensorValue),\
		BAROMETER_RESOURCE_INDEX(minMeasuredValue),\
		BAROMETER_RESOURCE_INDEX(maxMeasuredValue))

/* local functions ********************************************************** */

static retcode_t ResetMinMaxValues(Lwm2mSerializer_T *serializer_ptr, Lwm2mParser_T *parser_ptr)
{
    (void) parser_ptr;
    (void) serializer_ptr;

    INIT_LWM2M_DYNAMIC_CHANGES(changes);

    LWM2M_DYNAMIC_CHANGES_SET_FLOAT_VALUE(changes, BarometerResources, minMeasuredValue, BarometerResources.sensorValue.data.f);
    LWM2M_DYNAMIC_CHANGES_SET_FLOAT_VALUE(changes, BarometerResources, maxMeasuredValue, BarometerResources.sensorValue.data.f);
    LWM2M_DYNAMIC_CHANGES_REPORT(changes, BarometerUriPath);

    printf("Pressure: reset min/max Values to current\r\n");
    return (RC_OK);
}

static void ObjectBarometer_InternalEnable(float minRangeValue, float maxRangeValue)
{
    Started = true;
    BarometerResources.units.data.s = "hPa";
    BarometerResources.minRangeValue.data.f = minRangeValue;
    BarometerResources.maxRangeValue.data.f = maxRangeValue;
    BarometerResources.minMeasuredValue.data.f = minRangeValue;
    BarometerResources.maxMeasuredValue.data.f = minRangeValue;
    BarometerResources.sensorValue.data.f = minRangeValue;
    MinMaxInit = false;
    /*lint -e(534) purposefully suppressing the warning:Ignoring return value of function,since in CONF mode this may fail many times */
    FLUSH_RESOURCES;
}

static void ObjectBarometer_InternalDisable(void)
{
    Started = true;
    BarometerResources.units.data.s = "";
    BarometerResources.minRangeValue.data.f = 0.0F;
    BarometerResources.maxRangeValue.data.f = 0.0F;
    BarometerResources.minMeasuredValue.data.f = 0.0F;
    BarometerResources.maxMeasuredValue.data.f = 0.0F;
    BarometerResources.sensorValue.data.f = 0.0F;
    MinMaxInit = false;
    /*lint -e(534) purposefully suppressing the warning:Ignoring return value of function,since in CONF mode this may fail many times */
    FLUSH_RESOURCES;
}

static void ObjectBarometer_InternalSetValue(float sensorValue_hPa)
{
    INIT_LWM2M_DYNAMIC_CHANGES(changes);

    if (MinMaxInit)
    {
        if (LWM2M_DYNAMIC_CHANGES_SET_FLOAT_VALUE(changes, BarometerResources, sensorValue, sensorValue_hPa))
        {
            LWM2M_DYNAMIC_CHANGES_SET_FLOAT_MIN_VALUE(changes, BarometerResources, minMeasuredValue, sensorValue_hPa);
            LWM2M_DYNAMIC_CHANGES_SET_FLOAT_MAX_VALUE(changes, BarometerResources, maxMeasuredValue, sensorValue_hPa);
            if (Started)
                LWM2M_DYNAMIC_CHANGES_REPORT(changes, BarometerUriPath);
        }
    }
    else
    {
        MinMaxInit = true;
        LWM2M_DYNAMIC_CHANGES_SET_FLOAT_VALUE(changes, BarometerResources, sensorValue, sensorValue_hPa);
        LWM2M_DYNAMIC_CHANGES_SET_FLOAT_VALUE(changes, BarometerResources, minMeasuredValue, sensorValue_hPa);
        LWM2M_DYNAMIC_CHANGES_SET_FLOAT_VALUE(changes, BarometerResources, maxMeasuredValue, sensorValue_hPa);
        if (Started)
            LWM2M_DYNAMIC_CHANGES_REPORT(changes, BarometerUriPath);
    }
}

/* global functions ********************************************************* */
void ObjectBarometer_Init(void)
{
    Started = false;
    MinMaxInit = false;
    LWM2M_MUTEX_CREATE(AsyncCall_Updater.mutex);
    AsyncCall_Enabler.mutex = AsyncCall_Updater.mutex;
}

void ObjectBarometer_Enable(float minRangeValue, float maxRangeValue)
{
    Lwm2mUtil_UpdatePairResources(minRangeValue, maxRangeValue, &AsyncCall_Enabler);
}

void ObjectBarometer_Disable(void)
{
    Lwm2mUtil_Schedule(&AsyncCall_Disabler);
}

void ObjectBarometer_SetValue(float sensorValue_hPa)
{
    Lwm2mUtil_UpdateSingleResource(sensorValue_hPa, &AsyncCall_Updater);
}

