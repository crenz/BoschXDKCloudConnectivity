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

/* system header files */
#include "BCDS_Basics.h"

/* additional interface header files */
#include <Serval_Exceptions.h>

#define ILLUMINANCE_RESOURCES_INDEX(res) LWM2M_RESOURCES_INDEX(IlluminanceResources, res)

#define FLUSH_RESOURCES	Lwm2mReporting_multipleResourcesChanged(&IlluminanceUriPath, 6, \
			ILLUMINANCE_RESOURCES_INDEX(minValue),\
			ILLUMINANCE_RESOURCES_INDEX(maxValue),\
			ILLUMINANCE_RESOURCES_INDEX(units),\
			ILLUMINANCE_RESOURCES_INDEX(value),\
			ILLUMINANCE_RESOURCES_INDEX(minRangeValue),\
			ILLUMINANCE_RESOURCES_INDEX(maxRangeValue))

static retcode_t ResetMinMax(Lwm2mSerializer_T *serializer_ptr, Lwm2mParser_T *parser_ptr);
static void ObjectIlluminance_InternalEnable(float minRange, float maxRange);
static void ObjectIlluminance_InternalDisable(void);
static void ObjectIlluminance_InternalSetValue(float currentValue);

IlluminanceResource_T IlluminanceResources =
        {
                { 5601, LWM2M_FLOAT(0.0F) | LWM2M_READ_ONLY }, // minValue			|R|Single|M|Float |	  |
                { 5602, LWM2M_FLOAT(0.0F) | LWM2M_READ_ONLY }, // maxValue			|R|Single|O|Float |   |
                { 5603, LWM2M_FLOAT(0.0F) | LWM2M_READ_ONLY }, // minRangeValue	|R|Single|O|Float |   |
                { 5604, LWM2M_FLOAT(0.0F) | LWM2M_READ_ONLY }, // minRangeValue	|R|Single|O|Float |   |
                { 5605, LWM2M_FUNCTION( ResetMinMax ) }, // reset			|R|Single|O|Float |   |
                { 5700, LWM2M_FLOAT(0.0F) | LWM2M_READ_ONLY }, // Value			|R|Single|O|Float |   |
                { 5701, LWM2M_STRING_RO("") }, // unit 			|R|Single|O|String|   |
                };

/*lint -e(956) */
static bool MinMaxInit = false;

/*lint -e(956) const ?*/
static Lwm2m_URI_Path_T IlluminanceUriPath = { OBJECTS_IX_ILLUMINANCE_0, OBJECTS_IX_ILLUMINANCE_0, -1 };

static Lwm2m_Single_Resource_Update_T AsyncCall_Updater = { .set_single = ObjectIlluminance_InternalSetValue, .mutex = LWM2M_MUTEX_INIT_VALUE };
static Lwm2m_Pair_Resource_Update_T AsyncCall_Enabler = { .set_pair = ObjectIlluminance_InternalEnable, .mutex = LWM2M_MUTEX_INIT_VALUE };
static Lwm2m_Call_T AsyncCall_Disabler = { .call = ObjectIlluminance_InternalDisable };
static volatile bool Started = false;

static retcode_t ResetMinMax(Lwm2mSerializer_T *serializer_ptr, Lwm2mParser_T *parser_ptr)
{
    (void) parser_ptr;
    (void) serializer_ptr;

    INIT_LWM2M_DYNAMIC_CHANGES(changes);

    LWM2M_DYNAMIC_CHANGES_SET_FLOAT_VALUE(changes, IlluminanceResources, minValue, IlluminanceResources.value.data.f);
    LWM2M_DYNAMIC_CHANGES_SET_FLOAT_VALUE(changes, IlluminanceResources, maxValue, IlluminanceResources.value.data.f);
    if (Started)
        LWM2M_DYNAMIC_CHANGES_REPORT(changes, IlluminanceUriPath);

    return (RC_OK);
}

static void ObjectIlluminance_InternalEnable(float minRange, float maxRange)
{
    Started = true;
    IlluminanceResources.minValue.data.f = minRange;
    IlluminanceResources.maxValue.data.f = minRange;
    IlluminanceResources.minRangeValue.data.f = minRange;
    IlluminanceResources.maxRangeValue.data.f = maxRange;
    IlluminanceResources.value.data.f = minRange;
    IlluminanceResources.units.data.s = "mlx";
    MinMaxInit = false;
    /*lint -e(534) purposefully suppressing the warning:Ignoring return value of function,since in CONF mode this may fail many times */
    FLUSH_RESOURCES;
}

static void ObjectIlluminance_InternalDisable(void)
{
    Started = true;
    IlluminanceResources.minValue.data.f = 0.0F;
    IlluminanceResources.maxValue.data.f = 0.0F;
    IlluminanceResources.minRangeValue.data.f = 0.0F;
    IlluminanceResources.maxRangeValue.data.f = 0.0F;
    IlluminanceResources.value.data.f = 0.0F;
    IlluminanceResources.units.data.s = "";
    MinMaxInit = false;
    /*lint -e(534) purposefully suppressing the warning:Ignoring return value of function,since in CONF mode this may fail many times */
    FLUSH_RESOURCES;
}

static void ObjectIlluminance_InternalSetValue(float currentValue)
{
    INIT_LWM2M_DYNAMIC_CHANGES(changes);

    if (MinMaxInit)
    {
        if (LWM2M_DYNAMIC_CHANGES_SET_FLOAT_VALUE(changes, IlluminanceResources, value, currentValue))
        {
            LWM2M_DYNAMIC_CHANGES_SET_FLOAT_MIN_VALUE(changes, IlluminanceResources, minValue, currentValue);
            LWM2M_DYNAMIC_CHANGES_SET_FLOAT_MAX_VALUE(changes, IlluminanceResources, maxValue, currentValue);
            if (Started)
                LWM2M_DYNAMIC_CHANGES_REPORT(changes, IlluminanceUriPath);
        }
    }
    else
    {
        MinMaxInit = true;
        LWM2M_DYNAMIC_CHANGES_SET_FLOAT_VALUE(changes, IlluminanceResources, value, currentValue);
        LWM2M_DYNAMIC_CHANGES_SET_FLOAT_VALUE(changes, IlluminanceResources, minValue, currentValue);
        LWM2M_DYNAMIC_CHANGES_SET_FLOAT_VALUE(changes, IlluminanceResources, maxValue, currentValue);
        if (Started)
            LWM2M_DYNAMIC_CHANGES_REPORT(changes, IlluminanceUriPath);
    }
}

/* global functions ********************************************************* */

void ObjectIlluminance_Init(void)
{
    Started = false;
    LWM2M_MUTEX_CREATE(AsyncCall_Updater.mutex);
    AsyncCall_Enabler.mutex = AsyncCall_Updater.mutex;
}

void ObjectIlluminance_Enable(float minRange, float maxRange)
{
    Lwm2mUtil_UpdatePairResources(minRange, maxRange, &AsyncCall_Enabler);
}

void ObjectIlluminance_Disable(void)
{
    Lwm2mUtil_Schedule(&AsyncCall_Disabler);
}

void ObjectIlluminance_SetValue(float currentValue)
{
    Lwm2mUtil_UpdateSingleResource(currentValue, &AsyncCall_Updater);
}
