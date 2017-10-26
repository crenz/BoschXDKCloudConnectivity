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

#ifndef SOURCE_LWM2MOBJECT_GYROSCOPE_H_
#define SOURCE_LWM2MOBJECT_GYROSCOPE_H_

#include <Serval_Lwm2m.h>

/**
 * @brief LWM2M object id for Gyroscope.
 * @see https://github.com/IPSO-Alliance/pub/blob/master/reg/xml/3334.xml
 */
#define LWM2M_OBJECTID_GYROSCOPE 3334

/**
 * @brief LWM2M resource data for barometer.
 */
struct GyroscopeResource_S
{
    Lwm2mResource_T minRangeValue; /**< minimum range of possible values */
    Lwm2mResource_T maxRangeValue; /**< maximum range of possible values */
    Lwm2mResource_T units; /**< textual units, "Deg/s" according UCUM */
    Lwm2mResource_T xValue; /**< value for x-axis */
    Lwm2mResource_T yValue; /**< value for y-axis */
    Lwm2mResource_T zValue; /**< value for z-axis */
};

typedef struct GyroscopeResource_S GyroscopeResource_T;

/*lint -esym(956, GyroscopeResources) only accessed by serval scheduler */
/**
 * @brief LWM2M resource data for instance 0.
 */
extern GyroscopeResource_T GyroscopeResources;

/**
 * @brief Initialize LWM2M object instance. Must be called before any other function call.
 */
extern void ObjectGyroscope_Init(void);

/**
 * @brief Enable LWM2M object instance to start providing sensor data.
 * Function is thread safe, it schedules a job for serval.
 * Execution may therefore be deferred.
 *
 * @param[in] minRange minimum range of value in [Deg/s].
 * @param[in] maxRange maximum range of value in [Deg/s].
 */
extern void ObjectGyroscope_Enable(float minRange, float maxRange);

/**
 * @brief Disable LWM2M object instance to stop providing sensor data.
 * Sets unit to "" and all other values to 0.
 * Function is thread safe, it schedules a job for serval.
 * Execution may therefore be deferred.
 */
extern void ObjectGyroscope_Disable(void);

/**
 * @brief Set values for x-, y-, and z-axis.
 *
 * Set values for axis, units in "Deg/s". Will triggers the sending of
 * a notification, if a values was changed.
 * Function is thread safe, it schedules a job for serval.
 * Execution may therefore be deferred.
 *
 * @param[in] xValue value of x-axis in [Deg/s]
 * @param[in] yValue value of y-axis in [Deg/s]
 * @param[in] zValue value of z-axis in [Deg/s]
 */
extern void ObjectGyroscope_SetValues(float xValue, float yValue, float zValue); // in Deg/s

#endif /* SOURCE_LWM2MOBJECT_GYROSCOPE_H_ */
