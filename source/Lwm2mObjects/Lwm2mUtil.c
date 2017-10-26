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

/* additional interface header files */
#include "Serval_Scheduler.h"
#include "Serval_Exceptions.h"
#include "Serval_Log.h"

#define LOG_MODULE "LWU" /**< serval logging prefix */

bool Lwm2mUtil_SetFloatValue(Lwm2mDynamicChanges_T* changes, const Lwm2mResource_T* resourceTable, Lwm2mResource_T* resource, float value)
{
    if (resource->data.f != value)
    {
        resource->data.f = value;
        if (LWM2M_MAX_DYNAMIC_CHANGES > changes->changes)
        {
            changes->changeResourceIndexes[changes->changes++] = resource - resourceTable;
        }
        return true;
    }
    return false;
}

bool Lwm2mUtil_SetFloatMinValue(Lwm2mDynamicChanges_T* changes, const Lwm2mResource_T* resourceTable, Lwm2mResource_T* resource, float value)
{
    if (resource->data.f > value)
    {
        return Lwm2mUtil_SetFloatValue(changes, resourceTable, resource, value);
    }
    return false;
}

bool Lwm2mUtil_SetFloatMaxValue(Lwm2mDynamicChanges_T* changes, const Lwm2mResource_T* resourceTable, Lwm2mResource_T* resource, float value)
{
    if (resource->data.f < value)
    {
        return Lwm2mUtil_SetFloatValue(changes, resourceTable, resource, value);
    }
    return false;
}

void Lwm2mUtil_ReportChanges(Lwm2mDynamicChanges_T* changes, const Lwm2m_URI_Path_T* instanceUriPath)
{
    if (0 < changes->changes)
    {
        retcode_t rc = Lwm2mReporting_multipleResourcesChanged((Lwm2m_URI_Path_T*) instanceUriPath, changes->changes,
                changes->changeResourceIndexes[0],
                changes->changeResourceIndexes[1],
                changes->changeResourceIndexes[2],
                changes->changeResourceIndexes[3]); // ToDo - adjusted to LWM2M_MAX_DYNAMIC_CHANGES !
        if (RC_COAP_SERVER_SESSION_ALREADY_ACTIVE == rc)
        {
            if (0 <= instanceUriPath->objectInstanceIndex && instanceUriPath->objectInstanceIndex < DeviceResourceInfo.numberOfObjectInstances)
            {
#if (SERVAL_LOG_LEVEL >= SERVAL_LOG_LEVEL_ERROR)
                int oid = DeviceResourceInfo.objectInstances[instanceUriPath->objectInstanceIndex].objectId;
                int iid = DeviceResourceInfo.objectInstances[instanceUriPath->objectInstanceIndex].objectInstanceId;
                LOG_ERROR("/%d/%d Notify skipped!\n", oid, iid, instanceUriPath->objectInstanceIndex);
#endif
            }
            else
            {
                LOG_ERROR("/?/? Notify skipped!\n");
            }
        }
        else if (RC_OK != rc)
        {
            /*lint -e(641) suppresing since this is due to the implementation of servalstack*/
            LOG_ERROR("Notify " RC_RESOLVE_FORMAT_STR "\n", RC_RESOLVE(rc));
        }
    }
}

static retcode_t ScheduleUpdateSingleResource(Callable_T *callable_ptr, retcode_t status)
{
    (void) status;
    /*lint -e(413) suppresing since this is due to the implementation of servalstack*/
    Lwm2m_Single_Resource_Update_T* updater = CALLABLE_GET_CONTEXT(Lwm2m_Single_Resource_Update_T, callback, callable_ptr);
    if (LWM2M_MUTEX_LOCK(updater->mutex))
    {
        float value = updater->value;
        Callable_clear(&updater->callback);
        LWM2M_MUTEX_UNLOCK(updater->mutex);
        updater->set_single(value);
    }
    return RC_OK;
}

void Lwm2mUtil_UpdateSingleResource(float value, Lwm2m_Single_Resource_Update_T * updater)
{
    if (LWM2M_MUTEX_LOCK(updater->mutex))
    {
        bool schedule = false;
        updater->value = value;
        if (!Callable_isAssigned(&updater->callback))
        {
            /*lint -e(534) purposefully suppressing the warning:Ignoring return value of function,since the function never fails and it will be captured during compilation itself */
            Callable_assign(&updater->callback, ScheduleUpdateSingleResource);
            schedule = true;
        }
        LWM2M_MUTEX_UNLOCK(updater->mutex);
        if (schedule)
        {
            if (RC_OK != Scheduler_enqueue(&updater->callback, RC_OK))
            {
                Callable_clear(&updater->callback);
            }
        }
    }
}

static retcode_t ScheduleUpdatePairResources(Callable_T *callable_ptr, retcode_t status)
{
    (void) status;
    /*lint -e(413) suppresing since this is due to the implementation of servalstack*/
    Lwm2m_Pair_Resource_Update_T* updater = CALLABLE_GET_CONTEXT(Lwm2m_Pair_Resource_Update_T, callback, callable_ptr);
    if (LWM2M_MUTEX_LOCK(updater->mutex))
    {
        float value1 = updater->value1;
        float value2 = updater->value2;
        Callable_clear(&updater->callback);
        LWM2M_MUTEX_UNLOCK(updater->mutex);
        updater->set_pair(value1, value2);
    }
    return RC_OK;
}

void Lwm2mUtil_UpdatePairResources(float value1, float value2, Lwm2m_Pair_Resource_Update_T* updater)
{
    if (LWM2M_MUTEX_LOCK(updater->mutex))
    {
        bool schedule = false;
        updater->value1 = value1;
        updater->value2 = value2;
        if (!Callable_isAssigned(&updater->callback))
        {
            /*lint -e(534) purposefully suppressing the warning:Ignoring return value of function,since the function never fails and it will be captured during compilation itself */
            Callable_assign(&updater->callback, ScheduleUpdatePairResources);
            schedule = true;
        }
        LWM2M_MUTEX_UNLOCK(updater->mutex);
        if (schedule)
        {
            if (RC_OK != Scheduler_enqueue(&updater->callback, RC_OK))
            {
                Callable_clear(&updater->callback);
            }
        }
    }
}

static retcode_t ScheduleUpdateTrippleResources(Callable_T *callable_ptr, retcode_t status)
{
    (void) status;
    /*lint -e(413) suppresing since this is due to the implementation of servalstack*/
    Lwm2m_Tripple_Resource_Update_T* updater = CALLABLE_GET_CONTEXT(Lwm2m_Tripple_Resource_Update_T, callback, callable_ptr);
    if (LWM2M_MUTEX_LOCK(updater->mutex))
    {
        float value1 = updater->value1;
        float value2 = updater->value2;
        float value3 = updater->value3;
        Callable_clear(&updater->callback);
        LWM2M_MUTEX_UNLOCK(updater->mutex);
        updater->set_tripple(value1, value2, value3);
    }
    return RC_OK;
}

void Lwm2mUtil_UpdateTrippleResources(float value1, float value2, float value3, Lwm2m_Tripple_Resource_Update_T* updater)
{
    if (LWM2M_MUTEX_LOCK(updater->mutex))
    {
        bool schedule = false;
        updater->value1 = value1;
        updater->value2 = value2;
        updater->value3 = value3;
        if (!Callable_isAssigned(&updater->callback))
        {
            /*lint -e(534) purposefully suppressing the warning:Ignoring return value of function,since the function never fails and it will be captured during compilation itself */
            Callable_assign(&updater->callback, ScheduleUpdateTrippleResources);
            schedule = true;
        }
        LWM2M_MUTEX_UNLOCK(updater->mutex);
        if (schedule)
        {
            if (RC_OK != Scheduler_enqueue(&updater->callback, RC_OK))
            {
                Callable_clear(&updater->callback);
            }
        }
    }
}

static retcode_t ScheduleCall(Callable_T *callable_ptr, retcode_t status)
{
    (void) status;
    /*lint -e(413) suppresing since this is due to the implementation of servalstack*/
    Lwm2m_Call_T* call = CALLABLE_GET_CONTEXT(Lwm2m_Call_T, callback, callable_ptr);
    call->call();
    return RC_OK;
}

void Lwm2mUtil_Schedule(Lwm2m_Call_T* call)
{
    if (!Callable_isAssigned(&call->callback))
    {
        /*lint -e(534) purposefully suppressing the warning:Ignoring return value of function,since the function never fails and it will be captured during compilation itself */
        Callable_assign(&call->callback, ScheduleCall);
    }
    if (RC_OK != Scheduler_enqueue(&call->callback, RC_OK))
    {
        assert(0);
    }
}
