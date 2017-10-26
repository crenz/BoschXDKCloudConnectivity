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
*/
/*----------------------------------------------------------------------------*/
/**
 * @ingroup BOSCH_XDK_CLOUD_CONNECTIVITY
 *
 * @defgroup FOTA_DOWNLOAD FotaDownload
 * @{
 *
 * @brief This file provides the Implementation of FotaDownload.
 *
 * @details Fota Download processing source file.
 *
 * @note Using the same endpoint for firmware download and registration would require to coordinate the registration and download request (caused by the RFC7252 CoAP definition of default number for outstanding request to 1, means only 1 request should be waiting for a response. Therefore you may get the RC_COAP_CLIENT_SESSION_ALREADY_ACTIVE when sending a request e.g. when calling Lwm2mRegistration_update() or DownloadClient_DownloadBlock().)\n
 * It is a required to use different end points for firmware downlaod and LWM2M communication for proper application behaviour.
 *
 * @file
 **/

#include "FotaDownload.h"
#include "BCDS_Basics.h"
#include "BCDS_Retcode.h"
#include "BCDS_NVMConfig.h"
#include "BCDS_BSP_Board.h"
#include "BCDS_NetworkConfig.h"
#include "BCDS_WlanConnect.h"
#include "Lwm2mObject_SensorDevice.h"
#include "Lwm2mClient.h"
#include "BCDS_Accelerometer.h"
#include "BCDS_Environmental.h"
#include "BCDS_Gyroscope.h"
#include "BCDS_LightSensor.h"
#include "BCDS_Magnetometer.h"
#include "XdkSensorHandle.h"
#include "BCDS_SDCard_Driver.h"
#include "BCDS_WlanDriver.h"
#include "CfgParser.h"
#include <semphr.h>
#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>

/* Put the type and macro definitions here */
#define FOTA_UPDATE_NOTIFY_TIME_IN_MS      UINT8_C(100) /**< Time for server to get the fota update status before rebooting */

/* Put constant and variable definitions here */
static CmdProcessor_T FotaCmdProcessor;

struct FotaStorageAgent_S * FotaStorageAgentHandle = &SDCardStorage;

/* Put private function declarations here */

/* Put function implementations here */

static bool CheckEmptyCfg(const char *value)
{
    return (NULL == value) || (0 == *value);
}

static void CleanPeripheralsBeforeReset(void)
{
    Retcode_T retStatus = RETCODE_OK;
    xTimerHandle handle = getWlanTimerHandle();
    if (NULL != handle)
    {
        if (pdFALSE == xTimerStop(handle, portMAX_DELAY))
        {
            printf("failed to stop wlan connection  timer \r\n");
        }
    }
    retStatus = ObjectSensorDevice_Disable();
    if (retStatus != RETCODE_OK)
    {
        printf("stop sensor timer failed on reset after FW update \r\n");
    }
    retStatus = SDCardDriver_Deinitialize();
    if (RETCODE_OK != retStatus)
    {
        printf("SD card deinit failed \r\n");
    }
    retStatus = Accelerometer_deInit(xdkAccelerometers_BMA280_Handle);
    if (RETCODE_OK != retStatus)
    {
        printf("ACCEL DEinit failed \r\n");
    }
    retStatus = Gyroscope_deInit(xdkGyroscope_BMG160_Handle);
    if (RETCODE_OK != retStatus)
    {
        printf("Gyro DEinit failed \r\n");
    }
    retStatus = LightSensor_deInit(xdkLightSensor_MAX44009_Handle);
    if (RETCODE_OK != retStatus)
    {
        printf("Light DEinit failed \r\n");
    }
    retStatus = Magnetometer_deInit(xdkMagnetometer_BMM150_Handle);
    if (RETCODE_OK != retStatus)
    {
        printf("Mag DEinit failed \r\n");
    }
    retStatus = Environmental_deInit(xdkEnvironmental_BME280_Handle);
    if (RETCODE_OK != retStatus)
    {
        printf("Env DEinit failed \r\n");
    }
    retStatus = WlanDriver_DeInit();
    if (RETCODE_OK != retStatus)
    {
        printf("Wlan driver deinit failed \r\n");
    }
}

Retcode_T FotaNotificationcallback(FotaStateMachine_MsgNotify_T notify, void * info)
{
    BCDS_UNUSED(info);
    const char* Ssid = NULL;
    const char* Pwd = NULL;
    NetworkConfig_IpStatus_T ipstatus = NETWORKCONFIG_IP_NOT_ACQUIRED;
    NetworkConfig_IpSettings_T myIpGet;
    memset((NetworkConfig_IpSettings_T*) &myIpGet, 0, sizeof(NetworkConfig_IpSettings_T));
    Retcode_T retStatus = RETOCDE_MK_CODE((Retcode_T )RETCODE_FAILURE);

    switch (notify)
    {
    case FOTA_PREPARE_UPDATE:
        retStatus = RETCODE_OK;
        break;
    case FOTA_UPDATE:
        /*Do Reboot Here since the download is success give few milliseconds delay before Reboot*/
        vTaskDelay(FOTA_UPDATE_NOTIFY_TIME_IN_MS / portTICK_PERIOD_MS);
        CleanPeripheralsBeforeReset();
        BSP_Board_SoftReset();
        break;
    case FOTA_VALID_URI_RECEIVED:
        retStatus = RETCODE_OK;
        printf("The valid URI for FOTA download got received from LWM2M Server \r\n");
        break;
    case FOTA_EMPTY_URI_RECEIVED:
        retStatus = RETCODE_OK;
        printf("The User Cancel request is received from LWM2M Server \r\n");
        break;
    case FOTA_SEND_FAILED:
        printf("Fota send error retry will be done if the wifi connectivity is still available \r\n");
        ipstatus = NetworkConfig_GetIpStatus();
        if (ipstatus == NETWORKCONFIG_IPV4_ACQUIRED)
        {
            retStatus = NetworkConfig_GetIpSettings(&myIpGet);
            if (RETCODE_OK == retStatus)
            {
                printf("The IP was retrieved successfully");
            }

            if (UINT32_C(0) == (myIpGet.ipV4))
            {
                retStatus = RETOCDE_MK_CODE((Retcode_T )RETCODE_FAILURE);
                BSP_Board_SoftReset();
            }
        }
        else
        {
            Ssid = CfgParser_GetWlanSSID();
            Pwd = CfgParser_GetWlanPassword();

            if (CheckEmptyCfg(Ssid))
            {
                printf("Missing SSID !!!\r\n");
                retStatus = RETCODE_FAILURE;
            }

            if (CheckEmptyCfg(Pwd))
            {
                printf("Missing PASSWORD !!!\r\n");
                retStatus = RETCODE_FAILURE;
            }
            if (RETCODE_OK == retStatus)
            {
                WlanConnect_SSID_T ConnectSSID;
                WlanConnect_PassPhrase_T ConnectPassPhrase;

                ConnectSSID = (WlanConnect_SSID_T) Ssid;
                ConnectPassPhrase = (WlanConnect_PassPhrase_T) Pwd;
                retStatus = WlanConnect_WPA(ConnectSSID, ConnectPassPhrase, NULL);
            }
            if (RETCODE_OK != retStatus)
            {
                printf("Failed to connect to WiFi network upon firmware send failure /r/n hence rebooting ");
                BSP_Board_SoftReset();
            }
        }
        break;

    default:
        break;
    }
    return retStatus;
}

Retcode_T FotaInit(void)
{
    Retcode_T ReturnValue = RETOCDE_MK_CODE((Retcode_T )RETCODE_FAILURE);

    ReturnValue = CmdProcessor_initialize(&FotaCmdProcessor, (char*) "FotaCmdProcessor", TASK_PRIO_FOTA_CMD_PROCESSOR, TASK_STACK_SIZE_FOTA_CMD_PROCESSOR, TASK_Q_LEN_FOTA_CMD_PROCESSOR);
    if (RETCODE_OK == ReturnValue)
    {
        ReturnValue = FotaStateMachine_Init(&FotaCmdProcessor, FotaStorageAgentHandle, FotaNotificationcallback); // To be removed once FOTA is enabled
    }
    return ReturnValue;
}

/**@} */
