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

/* header definition ******************************************************** */
#ifndef SOURCE_LWM2MOBJECT_ACCELEROMETER_H_
#define SOURCE_LWM2MOBJECT_ACCELEROMETER_H_

/* additional interface header files */
#include <Serval_Lwm2m.h>

/**
 * @brief LWM2M object id for accelerometer.
 */
#define LWM2M_OBJECTID_IPSO_ACCELEROMETER 3313

/**
 * @brief LWM2M resource data for accelerometer.
 */
struct AccelerometerResource_S
{
    Lwm2mResource_T minRange; /**< minimum range of possible values */
    Lwm2mResource_T maxRange; /**< maximum range of possible values */
    Lwm2mResource_T units; /**< textual units, "g" according UCUM */
    Lwm2mResource_T xAxis; /**< acceleration x-axis */
    Lwm2mResource_T yAxis; /**< acceleration y-axis */
    Lwm2mResource_T zAxis; /**< acceleration z-axis */
};

typedef struct AccelerometerResource_S AccelerometerResource_T;

/*lint -esym(956, AccelerometerResources) only accessed by serval scheduler */
/**
 * @brief LWM2M resource data for instance 0.
 */
extern AccelerometerResource_T AccelerometerResources;
/**
 * @brief Initialize LWM2M object instance. Must be called before any other function call.
 */
extern void ObjectAccelerometer_Init(void);

/**
 * @brief Enable LWM2M object instance to start providing sensor data.
 * Function is thread safe, it schedules a job for serval.
 * Execution may therefore be deferred.
 *
 * @param[in] minRangeValue minimum range of value in [g].
 * @param[in] maxRangeValue maximum range of value in [g].
 */
extern void ObjectAccelerometer_Enable(float minRangeValue, float maxRangeValue);

/**
 * @brief Disable LWM2M object instance to stop providing sensor data.
 * Sets unit to "" and all other values to 0.
 * Function is thread safe, it schedules a job for serval.
 * Execution may therefore be deferred.
 */
extern void ObjectAccelerometer_Disable(void);

/**
 * @brief Set values for x-, y-, and z-axis.
 *
 * Set values for axis, units in [g]. Will triggers the sending of
 * a notification, if a value was changed.
 * Function is thread safe, it schedules a job for serval.
 * Execution may therefore be deferred.
 *
 * @param[in] xAxis acceleration of x-axis in [g]
 * @param[in] yAxis acceleration of y-axis in [g]
 * @param[in] zAxis acceleration of z-axis in [g]
 */
extern void ObjectAccelerometer_SetValues(float xAxis, float yAxis, float zAxis);

#endif /* SOURCE_LWM2MOBJECT_ACCELEROMETER_H_ */

/** ************************************************************************* */
