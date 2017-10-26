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
 * @defgroup LWM2M_UTIL LWM2M Util
 * @{
 *
 * @brief This file provides the Implementation of LWM2M Util
 *
 * @details The LWM2M util offers the possibility to report multiple resources changes
 * on instance base and to synchronize changes in the resources with the serval
 * scheduler to ensure data consistency.
 *
 * The LWM2M serval API (Serval_lwm2m.h) offers two function for reporting changes:
 *
 * \li retcode_t Lwm2mReporting_resourceChanged(Lwm2m_URI_Path_T *uripath)
 * \li retcode_t Lwm2mReporting_multipleResourcesChanged(Lwm2m_URI_Path_T *objectInstanceUripath, uint8_t count, ...)
 *
 * The first one reports the change of a singel resource. Multiple calls for changes on different resources
 * of the same instance will be reported with multiple notification even on instances.
 *
 * e.g.:\n
 * observe /3313/0 (accelerometer)\n
 * result (current state) { 5603 = -2.0, 5604 = 2.0, 5701 = "g", 5702 = 0.03, 5703 = 0.04, 5704 = 9.8 }
 *
 * change /3313/0/5702 0.01 (x-axis)\n
 * change /3313/0/5703 0.02 (y-axis)\n
 *
 *
 * will result in two instance notification:\n
 * (notification /3313/0) { 5603 = -2.0, 5604 = 2.0, 5701 = "g", 5702 = 0.01, 5703 = 0.04, 5704 = 9.8 }\n
 * (notification /3313/0) { 5603 = -2.0, 5604 = 2.0, 5701 = "g", 5702 = 0.01, 5703 = 0.02, 5704 = 9.8 }
 *
 * So, if the traffic should be reduced, or the changes should be visible only together (atomic)
 * Lwm2mReporting_resourceChanged could not be used.
 *
 * Therefore Lwm2mReporting_multipleResourcesChanged is available. Reporting both changes (x-Axis and y-Axis)
 * will result in one instance notification:\n
 *
 * (notification /3313/0) { 5603 = -2.0, 5604 = 2.0, 5701 = "g", 5702 = 0.01, 5703 = 0.02, 5704 = 9.8 }
 *
 * To simplify the preparation of the call of Lwm2mReporting_multipleResourcesChanged the
 * LWM2M utils offers several MACROS.
 *
 * The \link INIT_LWM2M_DYNAMIC_CHANGES \endlink defines the required data structure
 * \link Lwm2mDynamicChanges_S \endlink to store the changes.\n
 * The \link LWM2M_DYNAMIC_CHANGES_SET_FLOAT_VALUE \endlink,
 * \link LWM2M_DYNAMIC_CHANGES_SET_FLOAT_MIN_VALUE \endlink,
 * \link LWM2M_DYNAMIC_CHANGES_SET_FLOAT_MAX_VALUE \endlink process the new value
 * and store resulting changes in the defined data structure.
 * \link LWM2M_DYNAMIC_CHANGES_REPORT \endlink then reports the stored changes.
 *
 * e.g.
 *
 * \link AccelerometerResource_S \endlink
 *
 * \code{.c}
 * struct AccelerometerResource_S accelerometerResources =
 * {
 *    ...
 * };
 *
 * .... function(float newValue1, float newValue2, float newValue3)
 * {
 *    Lwm2m_URI_Path_T uriPath = {OBJECTS_IX_ACCELEROMETER_0, OBJECTS_IX_ACCELEROMETER_0, -1};
 *    INIT_LWM2M_DYNAMIC_CHANGES(changes);
 *    LWM2M_DYNAMIC_CHANGES_SET_FLOAT_VALUE(changes, accelerometerResources, xAxis, newValue1);
 *    LWM2M_DYNAMIC_CHANGES_SET_FLOAT_VALUE(changes, accelerometerResources, yAxis, newValue2);
 *    LWM2M_DYNAMIC_CHANGES_SET_FLOAT_VALUE(changes, accelerometerResources, zAxis, newValue3);
 *    LWM2M_DYNAMIC_CHANGES_REPORT(changes, uriPath);
 * }
 * \endcode
 *
 * For some resources representing minimum or maximum values, special macros exist.

 * \link BarometerResource_S \endlink
 *
 * \code{.c}
 * struct BarometerResource_S barometerResources =
 * {
 *    ...
 * };
 *
 * ... function(float newValue)
 * {
 *    Lwm2m_URI_Path_T uriPath = {OBJECTS_IX_BAROMETER_0, OBJECTS_IX_BAROMETER_0, -1};
 *    INIT_LWM2M_DYNAMIC_CHANGES(changes);
 *    if (LWM2M_DYNAMIC_CHANGES_SET_FLOAT_VALUE(changes, barometerResources, sensorValue, newValue)) {
 *        LWM2M_DYNAMIC_CHANGES_SET_FLOAT_MIN_VALUE(changes, barometerResources, minMeasuredValue, newValue);
 *        LWM2M_DYNAMIC_CHANGES_SET_FLOAT_MAX_VALUE(changes, barometerResources, maxMeasuredValue, newValue);
 *    }
 *    LWM2M_DYNAMIC_CHANGES_REPORT(changes, uriPath);
 * }
 * \endcode
 *
 * The LWM2M serval uses scheduler jobs to serialize lwm2m object instances
 * (encode the resources to a message).
 * If multiple resource changes should be visible only in a atomic manner,
 * the schedule must be synchronized with the resource access. Therefore the
 * transfer of the values is splitted into two steps:
 *
 * \li the values are copied to temporary variables using a semaphore to protect
 * the multiple changes. Then an updater job is scheduled in the serval scheduler.
 * \li the serval scheduler then calls the updater job, which reads the temporary
 * variables and then transfer them to the resources. Though this transfer is
 * executed in the scope of the scheduler, it appears to be atomic for the
 * serialization.
 *
 * Four functions are used support that synchronization:
 * \link Lwm2mUtil_updateSingleResource \endlink,
 * \link Lwm2mUtil_updatePairResources \endlink,
 * \link Lwm2mUtil_updateTrippleResources \endlink, and
 * \link Lwm2mUtil_schedule \endlink.
 *
 * To use them, you need to define a associated data structure:
 * \link Lwm2m_Single_Resource_Update_T \endlink,
 * \link Lwm2m_Pair_Resource_Update_T \endlink,
 * \link Lwm2m_Tripple_Resource_Update_T \endlink, and
 * \link Lwm2m_Call_T \endlink.
 *
 * Intialize the required fields:
 * \link Lwm2m_Single_Resource_Update_T.set_single \endlink,
 * \link Lwm2m_Pair_Resource_Update_T.set_pair \endlink,
 * \link Lwm2m_Tripple_Resource_Update_T.set_tripple \endlink, and
 * \link Lwm2m_Call_T.call \endlink.
 * and all mutex fields (note: a lwm2m object may share a mutex across
 * multiple synchronization data structures).
 *
 * e.g.
 * \code{.c}
 * static void ObjectAccelerometer_internalEnable(float minRange, float maxRange);
 * static void ObjectAccelerometer_internalDisable(void);
 * static void ObjectAccelerometer_internalSetValues(float xAxis, float yAxis, float zAxis);
 *
 * static Lwm2m_Pair_Resource_Update_T enabler = {.set_pair = ObjectAccelerometer_internalEnable, .mutex = LWM2M_MUTEX_INIT_VALUE };
 * static Lwm2m_Call_T disabler = {.call = ObjectAccelerometer_internalDisable };
 * static Lwm2m_Tripple_Resource_Update_T updater = {.set_tripple = ObjectAccelerometer_internalSetValues, .mutex = LWM2M_MUTEX_INIT_VALUE };
 *
 * ...
 *
 * void ObjectAccelerometer_init(void)
 * {
 *    ...
 * 	  LWM2M_MUTEX_CREATE(updater.mutex);
 * 	  enabler.mutex = updater.mutex;
 * }
 *
 * \endcode
 *
 * Call function the transfer values synchronized to resoruces:
 * e.g.
 * \code{.c}
 * void ObjectAccelerometer_setValues(float xAxis, float yAxis, float zAxis)
 * {
 * 	  Lwm2mUtil_updateTrippleResources(xAxis, yAxis, zAxis, &updater);
 * }
 *
 * \endcode
 *
 * @file
 **/

/* header definition ******************************************************** */
#ifndef SOURCE_LWM2MUTIL_H_
#define SOURCE_LWM2MUTIL_H_

#include <Serval_Lwm2m.h>

#ifndef LWM2M_MAX_DYNAMIC_CHANGES
/**
 * @brief Maximum number of supported resources changes using \link Lwm2mDynamicChanges_S \endlink .
 */
#define LWM2M_MAX_DYNAMIC_CHANGES 4		// ! on change align current dependent source in Lwm2mUtil_ReportChanges() !
#endif

/**
 * Data for dynamic reporting changes.
 */
struct Lwm2mDynamicChanges_S
{
    int changes; /**< number of changes */
    int changeResourceIndexes[LWM2M_MAX_DYNAMIC_CHANGES]; /**< resource ids of changes */
};

/**
 * Data for dynamic reporting changes.
 */
typedef struct Lwm2mDynamicChanges_S Lwm2mDynamicChanges_T;

/**
 * @brief set new float resource value.
 *
 * Check, if the new value changes the resource value.
 * If changed, store the resource index in the dynamic changes data.
 * Intended to be used by \link LWM2M_DYNAMIC_CHANGES_SET_FLOAT_VALUE \endlink .
 *
 * @param[in,out] changes dynamic changes data
 * @param[in] resourceTable resource table the resource belongs to
 * @param[in,out] resource resource of the resource table
 * @param[in] value new value
 *
 * @return true, if resource value was changed, false, otherwise
 */
extern bool Lwm2mUtil_SetFloatValue(Lwm2mDynamicChanges_T* changes, const Lwm2mResource_T* resourceTable, Lwm2mResource_T* resource, float value);

/**
 * @brief set new float resource value, if its less then the current value.
 *
 * Check, if the new value is less then the current resource value.
 * If so, change the resource value and store the resource index in
 * the dynamic changes data.
 * Intended to be used by \link LWM2M_DYNAMIC_CHANGES_SET_FLOAT_MIN_VALUE \endlink .
 *
 * @param[in,out] changes dynamic changes data
 * @param[in] resourceTable resource table the resource belongs to
 * @param[in,out] resource resource of the resource table
 * @param[in] value new value
 *
 * @return true, if resource value was changed, false, otherwise
 */
extern bool Lwm2mUtil_SetFloatMinValue(Lwm2mDynamicChanges_T* changes, const Lwm2mResource_T* resourceTable, Lwm2mResource_T* resource, float value);

/**
 * @brief set new float resource value, if its larger then the current value.
 *
 * Check, if the new value is larger then the current resource value.
 * If so, change the resource value and store the resource index in
 * the dynamic changes data.
 * Intended to be used by \link LWM2M_DYNAMIC_CHANGES_SET_FLOAT_MAX_VALUE \endlink .
 *
 * @param[in,out] changes dynamic changes data
 * @param[in] resourceTable resource table the resource belongs to
 * @param[in,out] resource resource of the resource table
 * @param[in] value new value
 *
 * @return true, if resource value was changed, false, otherwise
 */
extern bool Lwm2mUtil_SetFloatMaxValue(Lwm2mDynamicChanges_T* changes, const Lwm2mResource_T* resourceTable, Lwm2mResource_T* resource, float value);

/**
 * @brief Report changes from dynamic changes data.
 *
 * Intended to be used by \link LWM2M_DYNAMIC_CHANGES_REPORT \endlink .
 *
 * @param[in] changes dynamic changes data
 * @param[in] instanceUriPath uri path to changed instance
 */
extern void Lwm2mUtil_ReportChanges(Lwm2mDynamicChanges_T* changes, const Lwm2m_URI_Path_T* instanceUriPath);

/*lint -emacro(956, LWM2M_MUTEX_INSTANCE) */
/**
 * @brief Declare/define an instance of a mutex.
 * @param[in] M mutex name
 */
#define LWM2M_MUTEX_INSTANCE(M)	      		xSemaphoreHandle M
/**
 * @brief Initial value of a mutex.
 */
#define LWM2M_MUTEX_INIT_VALUE	           	NULL
/**
 * @brief Check, if mutex is already created.
 * @param[in] M mutex name
 */
#define LWM2M_MUTEX_IS_CREATED(M)	   		(NULL != (M))
/**
 * @brief Creates mutex.
 * @param[in] M mutex name
 */
#define LWM2M_MUTEX_CREATE(M)	       		(M) = xSemaphoreCreateMutex()
/**
 * @brief Lock mutex with timeout.
 * @param[in] M mutex name
 * @param[in] T timeout in milliseconds
 * @return true, it the mutex could be locked within the timeout, false otherwise.
 */
#define LWM2M_MUTEX_LOCK_TIMEOUT(M, T) 		(pdTRUE == xSemaphoreTake((M), (T)))
/**
 * @brief Lock mutex.
 * @param[in] M mutex name
 * @return true, it the mutex could be locked within 2000ms, false otherwise.
 */
#define LWM2M_MUTEX_LOCK(M)  	       		LWM2M_MUTEX_LOCK_TIMEOUT((M), 2000)

/*lint -emacro(534, LWM2M_MUTEX_UNLOCK) */
/**
 * @brief Unlock mutex.
 * @param[in] M mutex name
 */
#define LWM2M_MUTEX_UNLOCK(M)          		xSemaphoreGive(M)

/**
 * @brief Function to set a single float value.
 * @param[in] value value to be set.
 */
typedef void (*lwm2m_set_single_float)(float value);

/**
 * @brief Function to set a two float values.
 * @param[in] value1 first value to be set.
 * @param[in] value2 second value to be set.
 */
typedef void (*lwm2m_set_pair_float)(float value1, float value2);

/**
 * @brief Function to set a three float values.
 * @param[in] value1 first value to be set.
 * @param[in] value2 second value to be set.
 * @param[in] value3 third value to be set.
 */
typedef void (*lwm2m_set_tripple_float)(float value1, float value2, float value3);


/*lint -esym(956, AsyncCall_*) */

/**
 * Data to synchronize call with scheduler.
 */
typedef struct
{
    Callable_T callback; /**< callback for serval scheduler */
    void (*call)(void); /**< callback function. Required to be initialized. Called in context of scheduler */
} Lwm2m_Call_T;

/**
 * Data to synchronize resource transfer for a single value.
 */
typedef struct
{
    Callable_T callback; /**< callback for serval scheduler */
    LWM2M_MUTEX_INSTANCE(mutex); /**< mutex to synchronize access to values. Required to be initialized.*/
    lwm2m_set_single_float set_single; /**< callback function to set the value in the reosurces. Required to be initialized. Called in context of scheduler */
    float value; /**< value to be transferred to resources */
} Lwm2m_Single_Resource_Update_T;

/**
 * Data to synchronize resource transfer for two values.
 */
typedef struct
{
    Callable_T callback; /**< callback for serval scheduler */
    LWM2M_MUTEX_INSTANCE(mutex); /**< mutex to synchronize access to values. Required to be initialized.*/
    lwm2m_set_pair_float set_pair; /**< callback function to set the values in the reosurces. Required to be initialized. Called in context of scheduler */
    float value1; /**< value1 to be transferred to resources */
    float value2; /**< value2 to be transferred to resources */
} Lwm2m_Pair_Resource_Update_T;

/**
 * Data to synchronize resource transfer for three values.
 */
typedef struct
{
    Callable_T callback; /**< callback for serval scheduler */
    LWM2M_MUTEX_INSTANCE(mutex); /**< mutex to synchronize access to values. Required to be initialized. */
    lwm2m_set_tripple_float set_tripple; /**< callback function to set the values in the reosurces. Required to be initialized. Called in context of scheduler */
    float value1; /**< value1 to be transferred to resources */
    float value2; /**< value2 to be transferred to resources */
    float value3; /**< value3 to be transferred to resources */
} Lwm2m_Tripple_Resource_Update_T;

/*lint -ecall(534, Lwm2mUtil_SetFloat*) return value is only additional information not required to be processed */

/**
 * Update single float resource value.
 * Delegate the transfer to the scheduler.
 * @param[in] value new value
 * @param[in] updater data for synchronized resource transfer
 */
extern void Lwm2mUtil_UpdateSingleResource(float value, Lwm2m_Single_Resource_Update_T * updater);

/**
 * Update two resource values.
 * Delegate the transfer to the scheduler.
 * @param[in] value1 new value
 * @param[in] value2 new value
 * @param[in] updater data for synchronized resource transfer
 */
extern void Lwm2mUtil_UpdatePairResources(float value1, float value2, Lwm2m_Pair_Resource_Update_T* updater);

/**
 * Update three resource values.
 * Delegate the transfer to the scheduler.
 * @param[in] value1 new value
 * @param[in] value2 new value
 * @param[in] value3 new value
 * @param[in] updater data for synchronized resource transfer
 */
extern void Lwm2mUtil_UpdateTrippleResources(float value1, float value2, float value3, Lwm2m_Tripple_Resource_Update_T* updater);

/**
 * Schedule a call to lwm2m scheduler.
 * @param[in] call callback executed in context of scheduler
 */
extern void Lwm2mUtil_Schedule(Lwm2m_Call_T* call);

/**
 * @brief Initialize dynamic changes data.
 * @param C local name of changes
 */
#define INIT_LWM2M_DYNAMIC_CHANGES(C) Lwm2mDynamicChanges_T C = { .changes = 0 }

/**
 * @brief set new float resource value.
 *
 * Check, if the new value changes the resource value.
 * If changed, store the resource index in the dynamic changes data.
 *
 * @param[in,out] C dynamic changes data
 * @param[in] T resource table the resource belongs to
 * @param[in,out] R resource of the resource table
 * @param[in] V new value
 *
 * @return true, if resource value was changed, false, otherwise
 * @see Lwm2mUtil_SetFloatValue
 */
#define LWM2M_DYNAMIC_CHANGES_SET_FLOAT_VALUE(C, T, R, V) Lwm2mUtil_SetFloatValue(&C, (const Lwm2mResource_T*) &T, &(T.R), V)

/**
 * @brief set new float resource value, if its less then the current value.
 *
 * Check, if the new value is less then the current resource value.
 * If so, change the resource value and store the resource index in
 * the dynamic changes data.
 *
 * @param[in,out] C dynamic changes data
 * @param[in] T resource table the resource belongs to
 * @param[in,out] R resource of the resource table
 * @param[in] V new value
 *
 * @return true, if resource value was changed, false, otherwise
 * @see Lwm2mUtil_SetFloatMinValue
 */
#define LWM2M_DYNAMIC_CHANGES_SET_FLOAT_MIN_VALUE(C, T, R, V) Lwm2mUtil_SetFloatMinValue(&C, (const Lwm2mResource_T*) &T, &(T.R), V)

/**
 * @brief set new float resource value, if its larger then the current value.
 *
 * Check, if the new value is larger then the current resource value.
 * If so, change the resource value and store the resource index in
 * the dynamic changes data.
 *
 * @param[in,out] C dynamic changes data
 * @param[in] T resource table the resource belongs to
 * @param[in,out] R resource of the resource table
 * @param[in] V new value
 *
 * @return true, if resource value was changed, false, otherwise
 * @see Lwm2mUtil_SetFloatMaxValue
 */
#define LWM2M_DYNAMIC_CHANGES_SET_FLOAT_MAX_VALUE(C, T, R, V) Lwm2mUtil_SetFloatMaxValue(&C, (const Lwm2mResource_T*) &T, &(T.R), V)

/**
 * @brief Report changes from dynamic changes data.
 *
 * @param[in] C dynamic changes data
 * @param[in] U uri path to changed instance
 * @see Lwm2mUtil_ReportChanges
 */
#define LWM2M_DYNAMIC_CHANGES_REPORT(C, U) if (0 < C.changes) Lwm2mUtil_ReportChanges(&C, &U)

#endif /* SOURCE_LWM2MUTIL_H_ */

/** ************************************************************************* */
