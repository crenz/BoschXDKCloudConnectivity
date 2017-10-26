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
 * @file
 */

/* header definition ******************************************************** */
#ifndef SOURCE_SENSORDEVICE_H_
#define SOURCE_SENSORDEVICE_H_

#include <stdbool.h>

/**
 * @brief Definition of maxiumum values processed per sensor.
 */
#define SENSORDEVICE_MAX_VALUES 3

/**
 * @brief Processing modes for sensor values.
 */
enum ProcessingMode
{
    CURRENT, /**< use current value */
    AVG, /**< use avarage value */
    MIN, /**< use minimum value */
    MAX /**< use maximum value */
};

typedef enum ProcessingMode ProcessingMode_T;
/**
 * @brief Sensor values for processing.
 *
 * Used to pass values from sensors to processing functions.
 */
typedef struct SensorDeviceSampleDataFloat
{
    double values[SENSORDEVICE_MAX_VALUES]; /**< array with seonsor values */
} SensorDeviceSampleDataFloat_T;

/*lint -esym(956, *SensorDeviceData) only to be used by sensor timer task */
/**
 * @brief Processed sensor values.
 */
typedef struct SensorDeviceProcessDataFloat
{
    SensorDeviceSampleDataFloat_T data; /**< processed sensor values */
    int value_counter; /**< number of used values. Must not be larger then SENSORDEVICE_MAX_VALUES */
    int sample_counter; /**< number of samples */
    ProcessingMode_T mode; /**< processing mode */
    bool enabled; /**< indicator for sensor activation */
} SensorDeviceProcessDataFloat_T;

/**
 * @brief reset processed sensor values.
 *
 * Reset minimum, maximum and avarage value by reseting the sample_counter to 0.
 *
 * @param data processed sensor values
 */
extern void SensorDevice_ResetProcessDataFloat(SensorDeviceProcessDataFloat_T* data);

/**
 * @brief process current sensor values.
 *
 * Process current sensor values according process mode and store the result in the processed sensor values.
 * Adjust processed value for minimum, maximum or current mode, add values for avarage mode.
 *
 * @param mode process mode
 * @param data processed sensor values
 * @param sample current sensor values
 */
extern void SensorDevice_ProcessDataFloat(ProcessingMode_T mode, SensorDeviceProcessDataFloat_T* data, SensorDeviceSampleDataFloat_T* sample);

/**
 * @brief get processed sensor values.
 *
 * Get the processed sensor values and reset the processing.
 *
 * @param data processed sensor values
 * @param sample read values
 * @return true, if values are valid, false, otherwise
 */
extern bool SensorDevice_GetDataFloat(SensorDeviceProcessDataFloat_T* data, SensorDeviceSampleDataFloat_T* sample);

#endif /* SOURCE_SENSORDEVICE_H_ */

/** ************************************************************************* */
