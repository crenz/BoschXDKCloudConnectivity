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

#ifndef SOURCE_PROTECTED_LWM2MOBJECT_DEVICE_H_
#define SOURCE_PROTECTED_LWM2MOBJECT_DEVICE_H_

/* -- LWM2M Object Device ------ */

/**
 * @brief LWM2M object id for device.
 */
#define LWM2M_OBJECTID_DEVICE   	3

/**
 * @brief LWM2M resource data for device.
 */
struct DeviceResource_S
{
    Lwm2mResource_T manufacturer; /* IX=0 */
    Lwm2mResource_T modelNumber; /* IX=1 */
    Lwm2mResource_T serialNumber; /* IX=2 */
    Lwm2mResource_T FirmwareVersion; /* IX=3 */
    Lwm2mResource_T deviceReboot; /* IX=4 */
    Lwm2mResource_T factoryReset; /* IX=5 */
    Lwm2mResource_T errorCode; /* IX=11 */
    Lwm2mResource_T resetErrorCode; /* IX=12 */
    Lwm2mResource_T currentTime; /* IX=13 */
    Lwm2mResource_T UTCOffset; /* IX=14 */
    Lwm2mResource_T Timezone; /* IX=15 */
    Lwm2mResource_T SupportedBindingAndModes; /* IX=16 */
};

typedef struct DeviceResource_S DeviceResource_T;

//lint -esym(956, DeviceResources)
/**
 * @brief LWM2M resource data for instance 0.
 */
extern DeviceResource_T DeviceResources;

/**
 * @brief Initialize LWM2M object instance. Must be called before any other function call.
 */
extern void ObjectDevice_Init(void);

/**
 * @brief Enable LWM2M object instance. Start reporting values.
 * Function is thread safe, it schedules a job for serval.
 * Execution may therefore be deferred.
 */
extern void ObjectDevice_Enable(void);

/**
 * @brief notify that the device time has changed.
 * Will send a notification to observers.
 * Function is thread safe, it schedules a job for serval.
 * Execution may therefore be deferred.
 */
extern void ObjectDevice_NotifyTimeChanged(void);

#endif /* SOURCE_PROTECTED_LWM2MOBJECT_DEVICE_H_ */
