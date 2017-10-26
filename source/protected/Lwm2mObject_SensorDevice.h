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
#ifndef SOURCE_LWM2MOBJECT_SENSORDEVICE_H_
#define SOURCE_LWM2MOBJECT_SENSORDEVICE_H_

/* additional interface header files */

#include <Serval_Lwm2m.h>
/**
 * @brief LWM2M object id for sensor device.
 * BOSCH propr. in range of 10241 – 32768
 */
#define LWM2M_OBJECTID_SENSORDEVICE 15000

/**
 * @brief LWM2M resource data for sensor device.
 */
struct SensorDeviceResource_S
{
    Lwm2mResource_T transportInterval; /**< transport interval for sensor values in [s]*/
    Lwm2mResource_T accelerometerEnabled; /**< accelerometer enable/disable */
    Lwm2mResource_T gyroscopeEnabled; /**< gyroscope enable/disable */
    Lwm2mResource_T humiditySensorEnabled; /**< humidity enable/disable */
    Lwm2mResource_T illuminanceSensorEnabled; /**< illuminance enable/disable */
    Lwm2mResource_T magnetometerEnabled; /**< magnetometer enable/disable */
    Lwm2mResource_T barometerEnabled; /**< barometer/disable */
    Lwm2mResource_T temperatureSensorEnabled; /**< temperature enable/disable */
    Lwm2mResource_T preprocessingMode; /**< preprocessing mode, "current", "min", "max" or "avg" */
    Lwm2mResource_T macAddress; /**< WLAN MAC address of device */
};
typedef struct SensorDeviceResource_S SensorDeviceResource_T;

/*lint -esym(956, SensorDeviceResources) only accessed by serval scheduler */
/**
 * @brief LWM2M resource data for instance 0.
 */
extern SensorDeviceResource_T SensorDeviceResources;

/**
 * @brief Initialize LWM2M object instance. Must be called before any other function call.
 *
 * @param[in] mac textual WLAN MAC address of device. The parameter is used as reference
 *                          therefore the MAC address name must stay valid after the call.
 */
extern void ObjectSensorDevice_Init(const char* mac);

/**
 * @brief Enable LWM2M object instance to start providing sensor device controls.
 * Starts sensors timer to frequently read and process values.
 */
extern void ObjectSensorDevice_Enable(void);

/**
 * @brief Disable LWM2M object instance to stop providing sensor device controls.
 * Stops the sensors timer to frequently read and process values.
 */
Retcode_T ObjectSensorDevice_Disable(void);

#endif /* SOURCE_LWM2MOBJECT_SENSORDEVICE_H_ */

/** ************************************************************************* */
