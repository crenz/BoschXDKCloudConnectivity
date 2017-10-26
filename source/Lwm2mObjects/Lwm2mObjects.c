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
#include "BCDS_FotaStateMachine.h"

/* system header files */
#include <stdio.h>

/* additional interface header files */
#include <Serval_Lwm2m.h>

#define LWM2M_OBJECTID_FIRMWARE 5

/*lint -esym(956, ObjectInstances) ?*/
/* Rule: object instances sequence should keep increasing ObjectID's/ObjectID-Instances */
Lwm2mObjectInstance_T ObjectInstances[] =
        {
                /* 0=OBJECTS_IX_DEVICE_0 see header */
                { LWM2M_OBJECTID_DEVICE, /* 3 */
                LWM2M_SINGLE_INSTANCE,
                        LWM2M_RESOURCES(DeviceResources),
                        .permissions = { LWM2M_ACCESS_CONTROL_OWNER, 0x0, 0x0, 0x0 }
                },
                /* 1=OBJECTS_IX_CONNECTIVITY_MONITORING_0 see header */
                { LWM2M_OBJECTID_CONNECTIVITY_MONITORING, /* 4 */
                LWM2M_SINGLE_INSTANCE,
                        LWM2M_RESOURCES(ConnMonResources),
                        .permissions = { LWM2M_ACCESS_CONTROL_OWNER, 0x0, 0x0, 0x0 }
                },
                /* 2=OBJECTS_IX_FIRMWARE_0 */
                { LWM2M_OBJECTID_FIRMWARE, /* 5 */
                LWM2M_SINGLE_INSTANCE,
                        LWM2M_RESOURCES(FotaStateMachine_FotaResource),
                        .permissions = { LWM2M_ACCESS_CONTROL_OWNER, 0x0, 0x0, 0x0 }
                },
                /* 3=OBJECTS_IX_ILLUMINANCE_0 */
                { LWM2M_OBJECTID_IPSO_ILLUMINANCE, /* 3301/0 */
                0,
                        LWM2M_RESOURCES(IlluminanceResources),
                        .permissions = { LWM2M_ACCESS_CONTROL_OWNER, 0x0, 0x0, 0x0 }
                },
                /* 4=OBJECTS_IX_TEMPERATUR_0 */
                { LWM2M_OBJECTID_IPSO_TEMPERATURE, /* 3303/0 */
                0,
                        LWM2M_RESOURCES(TemperatureResources),
                        .permissions = { LWM2M_ACCESS_CONTROL_OWNER, 0x0, 0x0, 0x0 }
                },
                /* 5=OBJECTS_IX_HUMIDITY_0 */
                { LWM2M_OBJECTID_IPSO_HUMIDITY, /* 3304/0 */
                0,
                        LWM2M_RESOURCES(HumidityResources),
                        .permissions = { LWM2M_ACCESS_CONTROL_OWNER, 0x0, 0x0, 0x0 }
                },
                /* 6=OBJECTS_IX_LIGHTCONTROL_0 */
                { LWM2M_OBJECTID_IPSO_LIGHTCONTROL, /* 3310/0 */
                0,
                        LWM2M_RESOURCES(LightRedResources),
                        .permissions = { LWM2M_ACCESS_CONTROL_OWNER, 0x0, 0x0, 0x0 }
                },
                /* 7=OBJECTS_IX_LIGHTCONTROL_1 */
                { LWM2M_OBJECTID_IPSO_LIGHTCONTROL, /* 3310/1 */
                1,
                        LWM2M_RESOURCES(LightOrangeResources),
                        .permissions = { LWM2M_ACCESS_CONTROL_OWNER, 0x0, 0x0, 0x0 }
                },
                /* 8=OBJECTS_IX_LIGHTCONTROL_2 */
                { LWM2M_OBJECTID_IPSO_LIGHTCONTROL, /* 3310/2 */
                2,
                        LWM2M_RESOURCES(LightYellowResources),
                        .permissions = { LWM2M_ACCESS_CONTROL_OWNER, 0x0, 0x0, 0x0 }
                },
                /* 9=OBJECTS_IX_ACCELEROMETER_0 */
                { LWM2M_OBJECTID_IPSO_ACCELEROMETER, /* 3313/0 */
                0,
                        LWM2M_RESOURCES(AccelerometerResources),
                        .permissions = { LWM2M_ACCESS_CONTROL_OWNER, 0x0, 0x0, 0x0 }
                },
                /* 10=OBJECTS_IX_MAGNETOMETER_0 */
                { LWM2M_OBJECTID_IPSO_MAGNETOMETER, /* 3314/0 */
                0,
                        LWM2M_RESOURCES(MagnetometerResources),
                        .permissions = { LWM2M_ACCESS_CONTROL_OWNER, 0x0, 0x0, 0x0 }
                },
                /* 11=OBJECTS_IX_BAROMETER_0 */
                { LWM2M_OBJECTID_IPSO_BAROMETER, /* 3315/0 */
                0,
                        LWM2M_RESOURCES(BarometerResources),
                        .permissions = { LWM2M_ACCESS_CONTROL_OWNER, 0x0, 0x0, 0x0 }
                },
                /* 12=OBJECTS_IX_GYROSCOPE_0 */
                { LWM2M_OBJECTID_GYROSCOPE, /* 3334/0 */
                0,
                        LWM2M_RESOURCES(GyroscopeResources),
                        .permissions = { LWM2M_ACCESS_CONTROL_OWNER, 0x0, 0x0, 0x0 }
                },
                /* 13=OBJECTS_IX_SENSORDEVICE_0 */
                { LWM2M_OBJECTID_SENSORDEVICE, /* 15000 */
                LWM2M_SINGLE_INSTANCE,
                        LWM2M_RESOURCES(SensorDeviceResources),
                        .permissions = { LWM2M_ACCESS_CONTROL_OWNER, 0x0, 0x0, 0x0 }
                },
                /* 14=OBJECTS_IX_ALERTNOTIFICATION_0 */
                { LWM2M_OBJECTID_ALERTNOTIFICATION, /* 15003/0 */
                0,
                        LWM2M_RESOURCES(AlertNotificationResources),
                        .permissions = { LWM2M_ACCESS_CONTROL_OWNER, 0x0, 0x0, 0x0 }
                },
        };

Lwm2mDevice_T DeviceResourceInfo =
        {
                .name = NULL,
                .binding = UDP,
                .sms = NULL,
                .numberOfObjectInstances = LWM2M_OBJECT_INSTANCE_COUNT(ObjectInstances),
                .objectInstances = ObjectInstances,
        };

static void Objects_EnableConNotifies(Lwm2mObjectInstance_T* Object)
{
    int Index = 0;
    Lwm2mResource_T* Resources = Object->resources;
    for (; Index < Object->maxNumberOfResources; ++Index)
    {
        /*lint -e(641) Suppressing as this is related to serval stack implementation*/
        if (0 == (LWM2M_NOT_READABLE & Resources[Index].type))
        {
            Resources[Index].type |= LWM2M_CONFIRM_NOTIFY;
        }
    }
}

void Objects_Init(bool ConNotifies)
{
    if (ConNotifies)
    {
        int Index = 0;
        for (; Index < DeviceResourceInfo.numberOfObjectInstances; ++Index)
        {
            Objects_EnableConNotifies(&(DeviceResourceInfo.objectInstances[Index]));
        }
    }
}
