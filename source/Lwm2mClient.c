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
 * @defgroup LWM2M_CLIENT LWM2M Client
 * @{
 *
 * @brief This file provides the Implementation of Lwm2mClient.
 *
 * @details
 *
 * Please have a look on <a href=http://www.xdk.io/cloud>Bosch XDK Cloud Connectivity</a>
 *
 * Please have also a look into the \ref XDK_BOSCH_XDK_CLOUD_CONNECTIVITY_USER_GUIDE "Bosch XDK Cloud Connectivity Guide"
 * @file
 **/

/* module includes ********************************************************** */

/* own header files */
#include "XDKAppInfo.h"
#undef BCDS_MODULE_ID  /* Module ID define before including Basics package*/
#define BCDS_MODULE_ID XDK_APP_MODULE_ID_SENSOR_LWM2M_CLIENT

//#include <Serval_Lwm2m.h>
#include "Lwm2mUtil.h"
#include "SensorDeviceGyroscope.h"
#include "SntpTime.h"
#include "CfgParser.h"
#include "Lwm2mInterface.h"
#include "Lwm2mObjects.h"
#include "FotaDownload.h"
#include "ButtonsMan.h"
#include "XdkVersion.h"

/* system header files */
#include "BCDS_Basics.h"

/* additional interface header files */
#include <Serval_Log.h>
#include <Serval_Version.h>
#include "netcfg.h"
#include "PAL_initialize_ih.h"
#include "PAL_socketMonitor_ih.h"
#include "BCDS_WlanConnect.h"
#include "BCDS_NetworkConfig.h"
#include "BCDS_SDCard_Driver.h"
#include "BCDS_FWContainer.h"

/* constant definitions ***************************************************** */
#define EXPANDED_LENGTH               UINT8_C(65)          /**< length of expanded lwm2m endpoint name and dtls identity */
#define MAC_LENGTH                    UINT8_C(18)          /**< length of MAC as string */
#define WLAN_CONTROL_TASK_STACK_SIZE  UINT8_C(512)         /**< TASK stack size for Wlan Control Task */
#define WLAN_CONTROL_TASK_PRIORITY    UINT8_C(3)           /**< Task Priority for Wlan Control Task*/
#define WLAN_FAILED_REBOOTTIME_IN_S   UINT16_C(60)         /**< reboot time in seconds, if initial wlan failed */
#define SDCARD_DRIVE_NUMBER           UINT8_C(0)           /**< SD Card Drive 0 location */
#define SERIAL_CONNECT_TIME_INTERVAL  UINT32_C(5000)       /**< Macro to represent connect time interval */
#define CLIENT_CONNECT_PORT           UINT32_C(12345)      /**< Server Connecting Port is user configurable */
#define CONFIG_TASK_STACK_SIZE        UINT32_C(2048)       /**< Macro to represent the task stack size */
#define CONFIG_TASK_PRIORITY          UINT32_C(2)          /**< Macro to represent the task priority */
#define FOTA_TASK_STACK_SIZE          UINT32_C(3048)       /**< Macro to represent the task stack size */
#define FOTA_TASK_PRIORITY            UINT32_C(2)          /**< Macro to represent the task priority */
#define MAGIC_MAC_EXPAND              "#---MAC----#"       /**< Magic token expanded to MAC */
#define MAGIC_EP_EXPAND               "#-LWM2M-EP-#"       /**< Magic token expanded to LWM2M endpoint name */

/* local variables ********************************************************** */

/*lint -e956 */
static char MacAddress[MAC_LENGTH];
static char ExpandedEndpointName[EXPANDED_LENGTH];
static char ExpandedPskIdentity[EXPANDED_LENGTH];
static char KeyBuffer[EXPANDED_LENGTH];
static xSemaphoreHandle WlanControlSemaHandle = NULL;
static xTaskHandle WlanControlTaskHandle = NULL;
static xTimerHandle WlanControlTimerHandle = NULL;
static const char* HexToBin = "0123456789abcdefABCDEF";
static uint8_t versionNo[27] = { 0 }; /**< versionNo holds the Firmware Version Information */

/* global variables ********************************************************* */

CmdProcessor_T *AppCmdProcessorHandle; /**< Application Command Processor Handle */

/* WLAN connection status */
static WlanConnect_Status_T WlanConnectStatus = WLAN_DISCONNECTED;
static NetworkConfig_IpStatus_T IpAquiredStatus = NETWORKCONFIG_IP_NOT_ACQUIRED;

xTimerHandle getWlanTimerHandle(void)
{
    return WlanControlTimerHandle;
}

/* inline functions ********************************************************* */

/* local functions ********************************************************** */

/**
 * @brief Convert hexadecimal character/digit to number.
 *
 * '0' ... '9' to 0 ... 9
 * 'a' ... 'f' to 10 ... 15
 * 'A' ... 'F' to 10 ... 15
 *
 * @param[in] c character to be converted
 *
 * @return number, or -1, if provided character wasN#t a valid hexadecimal character.
 */
static int digitHexToBin(char c)
{
    int r = 0;
    char* p = strchr(HexToBin, c);
    if (NULL == p)
    {
        printf("Character '%c' is no hex-digit!\r\n", c);
        assert(false);
        return -1;
    }
    r = p - HexToBin;
    if (15 < r)
    {
        /* upper case character */
        r -= 6;
    }
    return (r & 0xf);
}

/**
 * @brief Decode hexadecimal string to binary buffer.
 *
 * @param[in] source - hexadecimal string
 * @param[out] dest - binary destination buffer.
 * @param[in] terminatedString - dest should be handled as string (0 terminated)
 *
 * @return number of converted bytes dest (excluding 0 termination, if applied):
 */
static unsigned int decodeHexToBin(const char* source, char* dest, bool terminatedString)
{
    unsigned int srcIndex = 0;
    unsigned int dstIndex = 0;
    unsigned int len = strlen(source);
    if (len & 1)
    {
        printf("Hex %s has odd length!\r\n", source);
        assert(false);
    }
    while (srcIndex < len)
    {
        char b = 0;
        int d = digitHexToBin(source[srcIndex++]);
        if (0 <= d)
        {
            b = (d & 0xf) << 4;
            d = digitHexToBin(source[srcIndex++]);
            if (0 <= d)
            {
                b |= (d & 0xf);
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }
        if (terminatedString && 0 == b)
        {
            printf("Hex 00 is not supported!\r\n");
            break;
        }
        dest[dstIndex++] = b;
    }
    if (terminatedString)
    {
        dest[dstIndex] = 0;
    }
    return dstIndex;
}

static void expandToken(const char* value, const char* magicToken, const char* expand, char* expandedValue, size_t expandedSize)
{
    const char* magic = strstr(value, magicToken);
    size_t length = 0;
    size_t left = expandedSize - 1;

    memset(expandedValue, 0, expandedSize);

    if (NULL == magic)
    {
        /* no magic token expand, just copy */
        length = strlen(value);
        if (left < length)
        {
            printf("value too long! max %u, length %u\n", left, length);
        }
        strncpy(expandedValue, value, left);
    }
    else
    {
        size_t length = magic - value;

        if (0 < length)
        {
            /* copy head */
            if (left < length)
            {
                printf("header too long! max %u, length %u\n", left, length);
                length = left;
            }
            memcpy(expandedValue, value, length);
            expandedValue += length;
            left -= length;
        }
        /* expand token */
        length = strlen(expand);
        if (left < length)
        {
            printf("expand too long! max left %u, length %u\n", left, length);
            length = left;
        }
        memcpy(expandedValue, expand, length);
        expandedValue += length;
        left -= length;

        /* move magic to tail after MAGIC token in the source */
        magic += strlen(magicToken);
        if (0 != *magic)
        {
            /* copy tail */
            length = strlen(magic);
            if (left < length)
            {
                printf("tail too long! max left %u, length %u\n", left, length);
                length = left;
            }
            memcpy(expandedValue, magic, length);
        }
    }
}

static void LogWlanVersion(void)
{
    SlVersionFull Version;
    _u8 ConfigOpt;
    _u8 ConfigLen = (_u8) sizeof(Version);
    _i32 ret;

    memset(&Version, 0, sizeof(Version));
    ConfigOpt = SL_DEVICE_GENERAL_VERSION;
    ret = sl_DevGet(SL_DEVICE_GENERAL_CONFIGURATION, &ConfigOpt, &ConfigLen, (_u8 *) (&Version));
    if (SL_OS_RET_CODE_OK == ret)
    {
        if (Version.ChipFwAndPhyVersion.ChipId & 0x10)
            printf("\nThis is a CC3200");
        else
            printf("\nThis is a CC3100");

        if (Version.ChipFwAndPhyVersion.ChipId & 0x2)
            printf("Z device\r\n");
        else
            printf("R device\r\n");

        printf("CHIP 0x%lx\r\nMAC 31.%lu.%lu.%lu.%lu\r\nPHY %u.%u.%u.%u\r\nNWP %lu.%lu.%lu.%lu\r\nROM 0x%x\r\nHOST %lu.%lu.%lu.%lu\r\n",
                Version.ChipFwAndPhyVersion.ChipId,
                Version.ChipFwAndPhyVersion.FwVersion[0], Version.ChipFwAndPhyVersion.FwVersion[1],
                Version.ChipFwAndPhyVersion.FwVersion[2], Version.ChipFwAndPhyVersion.FwVersion[3],
                Version.ChipFwAndPhyVersion.PhyVersion[0], Version.ChipFwAndPhyVersion.PhyVersion[1],
                Version.ChipFwAndPhyVersion.PhyVersion[2], Version.ChipFwAndPhyVersion.PhyVersion[3],
                Version.NwpVersion[0], Version.NwpVersion[1], Version.NwpVersion[2], Version.NwpVersion[3],
                Version.RomVersion,
                SL_MAJOR_VERSION_NUM, SL_MINOR_VERSION_NUM, SL_VERSION_NUM, SL_SUB_VERSION_NUM);
    }
    else
    {
        printf("DevGet %d\n", (int) ret);
    }
}

static bool EmptyCfg(const char *value)
{
    return (NULL == value) || (0 == *value);
}

/*lint -esym(534, AdjustIpAddress) */
static bool AdjustIpAddress(bool log)
{
    NetworkConfig_IpSettings_T MyIpSettings;
    char IpAddress[PAL_IP_ADDRESS_SIZE] = { 0 };
    Ip_Address_T* IpAddressBin = Ip_getMyIpAddr();
    Ip_Address_T NewIpAddressBin;

    if (RETCODE_OK != NetworkConfig_GetIpSettings(&MyIpSettings))
    {
        printf("Failed to get WLAN IP settings\n");
        return false;
    }

    NewIpAddressBin = Basics_ntohl(MyIpSettings.ipV4);
    if ((NewIpAddressBin != *IpAddressBin) || log)
    {
        *IpAddressBin = NewIpAddressBin;
        (void) Ip_convertAddrToString(IpAddressBin, IpAddress);
        printf("IP address of device  %s\r\n", IpAddress);
        ObjectConnectivityMonitoring_SetIpAddress(IpAddress);
        NewIpAddressBin = Basics_ntohl(MyIpSettings.ipV4Mask);
        (void) Ip_convertAddrToString(&NewIpAddressBin, IpAddress);
        printf("              Mask    %s\r\n", IpAddress);
        NewIpAddressBin = Basics_ntohl(MyIpSettings.ipV4Gateway);
        (void) Ip_convertAddrToString(&NewIpAddressBin, IpAddress);
        printf("              Gateway %s\r\n", IpAddress);
        NewIpAddressBin = Basics_ntohl(MyIpSettings.ipV4DnsServer);
        (void) Ip_convertAddrToString(&NewIpAddressBin, IpAddress);
        printf("              DNS     %s\r\n", IpAddress);
        return true;
    }
    return false;
}

static Retcode_T SetStaticIpSettings(void)
{
    retcode_t rc = RC_OK;
    const char* StaticIpSetting = NULL;
    Ip_Address_T Address;
    NetworkConfig_IpSettings_T IpSet;

    memset(&IpSet, 0, sizeof(IpSet));
    IpSet.isDHCP = (uint8_t) NETWORKCONFIG_DHCP_DISABLED;

    /* setting static IP parameters*/
    StaticIpSetting = CfgParser_GetStaticIpAddress();
    rc = Ip_convertStringToAddr(StaticIpSetting, &Address);
    if (RC_OK == rc)
    {
        printf("   STATIC IP      %s\r\n", StaticIpSetting);
        IpSet.ipV4 = Basics_htonl(Address);
    }
    /* setting static IP Dns Server parameters*/
    StaticIpSetting = CfgParser_GetDnsServerAddress();
    rc = Ip_convertStringToAddr(StaticIpSetting, &Address);
    if (RC_OK == rc)
    {
        printf("   STATIC DNS     %s\r\n", StaticIpSetting);
        IpSet.ipV4DnsServer = Basics_htonl(Address);
    }
    /* setting static IP Gateway parameters*/
    StaticIpSetting = CfgParser_GetGatewayAddress();
    rc = Ip_convertStringToAddr(StaticIpSetting, &Address);
    if (RC_OK == rc)
    {
        printf("   STATIC GATEWAY %s\r\n", StaticIpSetting);
        IpSet.ipV4Gateway = Basics_htonl(Address);
    }
    /* setting static IP subnet mask parameters*/
    StaticIpSetting = CfgParser_GetSubnetMask();
    rc = Ip_convertStringToAddr(StaticIpSetting, &Address);
    if (RC_OK == rc)
    {
        printf("   STATIC MASK    %s\r\n", StaticIpSetting);
        IpSet.ipV4Mask = Basics_htonl(Address);
    }
    return NetworkConfig_SetIpStatic(IpSet);
}

/**
 * @brief This API is called when the LWM2M Client
 *      Connecting to a WLAN Access point.
 *       This function connects to the required AP (SSID_NAME).
 *       The function will return once we are connected and have acquired IP address
 *   @warning
 *      If the WLAN connection fails or we don't acquire an IP address, We will be stuck in this function forever.
 *      Check whether the callback "SimpleLinkWlanEventHandler" or "SimpleLinkNetAppEventHandler" hits once the
 *      sl_WlanConnect() API called, if not check for proper GPIO pin interrupt configuration or for any other issue
 *
 * @param[in] ssid   SSID of the Access Point
 *
 * @param[in] pwd     Password for Access Point
 *
 *
 * @retval     RC_OK       IP address returned successfully
 *
 * @retval     RC_PLATFORM_ERROR         Error occurred in fetching the ip address
 *
 */

static retcode_t WlanConnect(const char* ssid, const char* pwd)
{
    if (RETCODE_OK != WlanConnect_Init())
    {
        return (RC_PLATFORM_ERROR);
    }

    int tries = 1;
    Retcode_T rc;
    WlanConnect_SSID_T ConnectSSID;
    WlanConnect_PassPhrase_T ConnectPassPhrase;

    ConnectSSID = (WlanConnect_SSID_T) ssid;
    ConnectPassPhrase = (WlanConnect_PassPhrase_T) pwd;

    SlSecParams_t SecParams;

    /* Set network parameters */
    SecParams.Key = (signed char *) ConnectPassPhrase;
    SecParams.KeyLen = strlen((const char*) ConnectPassPhrase);
    SecParams.Type = 0x02; //WLI_SECURITY_TYPE_WPA;

    LogWlanVersion();

    if (CGF_CONDITIONAL_VALUE_YES == CfgParser_IsStaticModeActivated())
    {
        printf("STATIC IP\r\n");
        if (RETCODE_OK != SetStaticIpSettings())
        {
            printf("Error in setting static IP\n\r");
            return (RC_PLATFORM_ERROR);
        }
    }
    else
    {
        printf("DHCP\r\n");
        if (RETCODE_OK != NetworkConfig_SetIpDhcp(NULL))
        {
            printf("Error in setting DHCP\n\r");
            return (RC_PLATFORM_ERROR);
        }
    }

    printf("Start connecting to WPA network\r\n");

    rc = WlanConnect_WPA(ConnectSSID, ConnectPassPhrase, NULL);
    while (((int32_t)RETCODE_ERROR_IP_NOT_ACQ == Retcode_GetCode(rc) || (int32_t)RETCODE_ERROR_WRONG_PASSWORD == Retcode_GetCode(rc))
            && 3 >= tries)
    {
        /* retry, sometimes RETCODE_ERROR_IP_NOT_ACQ or RETCODE_ERROR_WRONG_PASSWORD are reported */
        rc = WlanConnect_WPA(ConnectSSID, ConnectPassPhrase, NULL);
        ++tries;
    }
    if (RETCODE_OK == rc)
    {
        int SlRet;
        printf("Connected to WPA network successfully\r\n");
        SlRet = sl_WlanProfileDel(0xFF);
        if (SL_RET_CODE_OK > SlRet)
        {
            printf("  WLAN: failed to delete all profiles\n\r");
        }
        SlRet = sl_WlanProfileAdd((signed char *) ConnectSSID, strlen((const char*) ConnectSSID), 0, &SecParams, 0, 1, 0);
        if (SL_RET_CODE_OK > SlRet)
        {
            printf("  WLAN: failed to add profile\n\r");
        }
        SlRet = sl_WlanPolicySet(SL_POLICY_CONNECTION, SL_CONNECTION_POLICY(0, 0, 0, 0, 0), NULL, 0); // reset
        if (SL_RET_CODE_OK > SlRet)
        {
            printf("  WLAN: failed to reset connection policy\n\r");
        }
        SlRet = sl_WlanPolicySet(SL_POLICY_CONNECTION, SL_CONNECTION_POLICY(1, 1, 0, 0, 0), NULL, 0); // autoreconnect, fast
        if (SL_RET_CODE_OK > SlRet)
        {
            printf("  WLAN: failed to set connection policy\n\r");
        }
        SlRet = sl_WlanRxStatStart();
        if (SL_RET_CODE_OK > SlRet)
        {
            printf("  WLAN: failed to start Rx Statistic\n\r");
        }
        AdjustIpAddress(true);
        return (RC_OK);
    }
    else
    {
        if ((int32_t)RETCODE_ERROR_WRONG_PASSWORD == Retcode_GetCode(rc))
        {
            printf("  WLAN: connect WPA failed! Wrong password.\n\r");
        }
        else if ((int32_t)RETCODE_ERROR_IP_NOT_ACQ == Retcode_GetCode(rc))
        {
            printf("  WLAN: connect WPA failed! IP not acquired. (%d tries)\n\r", tries);
        }
        else
        {
            printf("  WLAN: connect WPA failed %lx %lu\n\r", (long unsigned) rc, (long unsigned) Retcode_GetCode(rc));
        }

        return (RC_PLATFORM_ERROR);
    }
}

static const char* GetWlanConnectDescription(WlanConnect_Status_T Status)
{
    switch (Status)
    {
    case WLAN_CONNECTED:
        return "online";
    case WLAN_DISCONNECTED:
        return "offline";
    case WLAN_CONNECTION_ERROR:
        return "error";
    case WLAN_CONNECTION_PWD_ERROR:
        return "pwd error";
    case WLAN_DISCONNECT_ERROR:
        return "disconnect error";
    default:
        return NULL;
    }
}

static void WlanControl(void)
{
    bool log = false;
    WlanConnect_Status_T Status = WlanConnect_GetStatus();
    if (WlanConnectStatus != Status)
    {
        const char* Description = GetWlanConnectDescription(Status);
        WlanConnectStatus = Status;
        if (NULL == Description)
        {
            printf("WLAN: connect status: --- unknown %d ---\n\r", Status);
        }
        else
        {
            printf("WLAN: connect status: --- %s ---\n\r", Description);
        }
        if (WlanConnectStatus != WLAN_CONNECTED)
        {
            *Ip_getMyIpAddr() = *Ip_getUnspecifiedAddr();
            IpAquiredStatus = NETWORKCONFIG_IP_NOT_ACQUIRED;
            printf("IP Address of the Device is: --.--.--.--\r\n");
            ObjectConnectivityMonitoring_SetIpAddress("");
        }
    }
    if (WlanConnectStatus == WLAN_CONNECTED)
    {
        NetworkConfig_IpStatus_T IpStatus = NetworkConfig_GetIpStatus();
        if (IpAquiredStatus != IpStatus)
        {
            IpAquiredStatus = IpStatus;
            log = true;
        }
        if (NETWORKCONFIG_IPV4_ACQUIRED == IpStatus)
        {
            AdjustIpAddress(log);
        }
    }
    SlGetRxStatResponse_t RxStatResp;
    if (SL_RET_CODE_OK == sl_WlanRxStatGet(&RxStatResp, 0))
    {
        ObjectConnectivityMonitoring_SetRadioSignalStrength(RxStatResp.AvarageDataCtrlRssi);
    }
}

static void WlanControlTask(void *pvParameters)
{
    BCDS_UNUSED(pvParameters);

    for (;;)
    {
        if (xSemaphoreTake(WlanControlSemaHandle, portMAX_DELAY) == pdTRUE)
        {
            WlanControl();
        }
        else
        {
            assert(false);
        }
    }
}

static void WlanControlTimerCB(void *pvParameters)
{
    BCDS_UNUSED(pvParameters);
    /*lint -e(534) also OK, if semaphore is already given */
    xSemaphoreGive(WlanControlSemaHandle);
}

static void InitWlanControl(void)
{
    uint32_t Ticks = 10000 / portTICK_RATE_MS;
    /**create a binary semaphore*/
    WlanControlSemaHandle = xSemaphoreCreateBinary();
    if (NULL == WlanControlSemaHandle)
    {
        /* Failed to create semaphore */
        assert(false);
    }
    if (xSemaphoreGive(WlanControlSemaHandle) != pdTRUE)
    {
        /* Failed to Semaphore Give */
        assert(false);
    }
    /** Create a task */
    if (xTaskCreate(WlanControlTask, (const char * const) "WLanControlTask", WLAN_CONTROL_TASK_STACK_SIZE, NULL, WLAN_CONTROL_TASK_PRIORITY, // 224: 24Bytes remaining
            &WlanControlTaskHandle) != pdPASS)
    {
        assert(false);
    }

    /** this is the timer (10sec) task which signals the task using semaphore() */
    WlanControlTimerHandle = xTimerCreate((const char * const ) "wlanControlTimer", Ticks, pdTRUE,
    NULL, WlanControlTimerCB);
    if (NULL == WlanControlTimerHandle) /* Timer was not created */
    {
        assert(false);
    }
    if (xTimerStart(WlanControlTimerHandle, portMAX_DELAY) != pdTRUE)
    {
        assert(false);
    }
}

Retcode_T CheckSdCardAndFileParser(void)
{
    Retcode_T RetVal = RETCODE_OK;
    Retcode_T getRetCode = (uint32_t) RETCODE_OK;
    if (SDCARD_INSERTED == SDCardDriver_GetDetectStatus())
    {
        RetVal = SDCardDriver_DiskInitialize(SDCARD_DRIVE_NUMBER); /* Initialize SD card */
        if (RetVal != RETCODE_OK)
        {
            printf("disk initialization failed");
            RetVal = SDCardDriver_Deinitialize();
            assert(false);
        }
        printf("Success in initializing the SD card disk \r\n");
        RetVal = CfgParser_ParseConfigFile();
        getRetCode = Retcode_GetCode(RetVal);
        if ((int32_t)RETCODE_CFG_PARSER_SD_CARD_MOUNT_ERROR == getRetCode)
        {
            printf("disk mount failed, check SD card is in damaged state \r\n");
            assert(false);
        }
    }
    else
    {
        CfgParser_Initialize();
        CfgParser_List("Default Configuration:", CFG_TRUE);
    }

    return (RetVal);
}

/**
 * @brief This function to Get the Firmware Version by combining the XDK SW Release (i.e., Workbench Release) version (i.e., 3.0.1) & Fota Container Firmware Version (i.e., 0.0.1)
 * 			So , the GetFwVersionInfo returns the Version Information as like (i.e., 3.0.1-0.0.1 )
 *
 * @param[out] versionNo - Firmware Version (i.e., 3.0.1-0.0.1 if new bootloader or 3.0.1-xx.xx.xx for old bootloader).
 */
static const uint8_t * GetFwVersionInfo(void)
{
    Retcode_T retval = RETCODE_OK;
	uint32_t xdkVersion;
	uint32_t fwVersionValue = UINT32_C(0x00000000);
	uint8_t* startOfAppVersionPtr = NULL;
	uint8_t index = 0;
	xdkVersion = XdkVersion_GetVersion();
#if XDK_FOTA_ENABLED_BOOTLOADER == 1
	retval = FWContainer_ReadFWVersion(FotaStorageAgentHandle, FOTA_PARTITION_PRIMARY, (uint8_t *) &fwVersionValue);
#endif
	if (RETCODE_OK == retval) {
		startOfAppVersionPtr = convert32IntegerToVersionString(fwVersionValue,
				xdkVersion, versionNo);
	}
	index = startOfAppVersionPtr - versionNo;
	versionNo[index - UINT8_C(1)] = '-';
#if XDK_FOTA_ENABLED_BOOTLOADER == 0
	/*Replace with default string in case the FOTA version is not there*/
	versionNo[index] = 'x';
	versionNo[index + UINT8_C(1)] = 'x';
	versionNo[index + UINT8_C(2)] = '.';
	versionNo[index + UINT8_C(3)] = 'x';
	versionNo[index + UINT8_C(4)] = 'x';
	versionNo[index + UINT8_C(5)] = '.';
	versionNo[index + UINT8_C(6)] = 'x';
	versionNo[index + UINT8_C(7)] = 'x';
	versionNo[index + UINT8_C(8)] = '\0';
#endif
	return versionNo;
}

/** @brief This function does the initialization of required setup to interact with lwm2m Server and the objects that are defined for this project.
 *  It includes providing the necessary data for the objects like object and resource ID, object name,
 *  its instances.
 */
static void Init(void)
{
    Lwm2mConfiguration_T Configuration;
    Configuration.localPort = CLIENT_CONNECT_PORT;
    Configuration.sdCardConfig = FALSE;
    bool EnableConNotifies = FALSE;
    uint8_t TestMode = 0;
    const char* Ssid = NULL;
    const char* Pwd = NULL;
    const char* Endpoint = NULL;
    const char* Binding = "U";
    char version[19];
    uint8_t WlanMacBytes[SL_MAC_ADDR_LEN];
    uint8_t WlanMacBytesLength = SL_MAC_ADDR_LEN;
    int32_t ret;
    retcode_t rc = RC_OK;
    Retcode_T ReturnValue = RETCODE_OK;
    Retcode_T getReturnValueCode = RETCODE_OK;

    /* Startup delay to allow to Serial Port */
    vTaskDelay((SERIAL_CONNECT_TIME_INTERVAL / portTICK_RATE_MS));

    ReturnValue = SDCardDriver_Initialize();
    if (RETCODE_OK == ReturnValue)
    {
        ReturnValue = CheckSdCardAndFileParser();
        getReturnValueCode = Retcode_GetCode(ReturnValue);

        if (((int32_t)SDCARD_NOT_INITIALIZED == getReturnValueCode) || ((int32_t)RETCODE_CFG_PARSER_SD_CARD_FILE_NOT_EXIST == getReturnValueCode) ||
                ((int32_t)RETCODE_CFG_PARSER_SD_CARD_FILE_CLOSE_ERROR == getReturnValueCode))
        {
            printf("Dear User error in parsing SD card or SD card damaged or SD card initialization not happened properly\r \n");
            assert(false);
        }
        Configuration.sdCardConfig = TRUE;
    }
    else
    {
        printf("Error in SD Card Initialize code may run properly if the local credentials are fine\n");
    }
    ObjectConnectivityMonitoring_Init();
    SensorDeviceGyroscope_InitRand();
    EnableConNotifies = CfgParser_UseLwm2mConNotifies();
    TestMode = CfgParser_TestMode();
    Ssid = CfgParser_GetWlanSSID();
    Pwd = CfgParser_GetWlanPassword();
    sprintf(version, "%s", GetFwVersionInfo());
    if (EmptyCfg(version))
    {
        printf("Missing VersionInfo !!!\r\n");
        ReturnValue = RETCODE_FAILURE;
    }
    else
    {
    	printf("Firmware Version : %s \r\n",version);
    }
    Configuration.lifetime = CfgParser_GetLwm2mLifetime();
    Configuration.destServer = CfgParser_GetLwm2mServerAddress();
    Endpoint = CfgParser_GetLwm2mEndpointName();
    if (EmptyCfg(Ssid))
    {
        printf("Missing SSID !!!\r\n");
        ReturnValue = RETCODE_FAILURE;
    }
    if (EmptyCfg(Pwd))
    {
        printf("Missing PASSWORD !!!\r\n");
        ReturnValue = RETCODE_FAILURE;
    }
    if (EmptyCfg(Configuration.destServer))
    {
        printf("Missing LWM2MDEFSRV !!!\r\n");
        ReturnValue = RETCODE_FAILURE;
    }
    if (EmptyCfg(Endpoint))
    {
        printf("Missing LWM2MENDPOINT !!!\r\n");
        ReturnValue = RETCODE_FAILURE;
    }
    if (RETCODE_OK != ReturnValue)
    {
        /* required configuration missing! */
        assert(false);
    }

    printf("INIT Starting up\r\n");

    printf("wlanConnect\r\n");
    rc = WlanConnect(Ssid, Pwd);
    if (RC_OK != rc)
    {
        /*lint -e(641) This warning is related to implementation of serval stack hence suppressed*/
        printf("Network init/connection failed " RC_RESOLVE_FORMAT_STR "\n", RC_RESOLVE(rc));
        vTaskDelay((WLAN_FAILED_REBOOTTIME_IN_S * 1000) / portTICK_PERIOD_MS);
        Lwm2mInterfaceRebootNow("NETWORK FAILED!");
        return;
    }
    printf("Network Initialization done\r\n");

    ret = sl_NetCfgGet((_u8) SL_MAC_ADDRESS_GET, NULL, &WlanMacBytesLength, WlanMacBytes);
    if (SL_RET_CODE_OK > ret)
    {
        printf("WLAN MAC status: %d\r\n", (int) ret);
        return;
    }
    snprintf(MacAddress, sizeof(MacAddress), "%02X-%02X-%02X-%02X-%02X-%02X", WlanMacBytes[0], WlanMacBytes[1], WlanMacBytes[2], WlanMacBytes[3], WlanMacBytes[4], WlanMacBytes[5]);
    printf("WLAN got MAC: %s\r\n", MacAddress);

    snprintf(KeyBuffer, sizeof(KeyBuffer), "%02X%02X%02X%02X%02X%02X", WlanMacBytes[0], WlanMacBytes[1], WlanMacBytes[2], WlanMacBytes[3], WlanMacBytes[4], WlanMacBytes[5]);

    rc = PAL_initialize();
    if (RC_OK != rc)
    {
        printf("PAL and network initialize %i \r\n", rc);
        return;
    }
    printf("PAL Initialization done\r\n");

    LOG_INFO("Logging enabled\r\n");

    PAL_socketMonitorInit();
    printf("PAL SocketMonitor done\n");

    if (strncmp("coaps://", Configuration.destServer, 8) == 0)
    {
        Configuration.useDtlsPsk = TRUE;
    }
    else if (strncmp("coap://", Configuration.destServer, 7) == 0)
    {
        Configuration.useDtlsPsk = FALSE;
    }
    else
    {
        printf("Only coap:// (plain) or coaps:// (encrypted) are supported!\r\n");
        assert(false);
    }

    expandToken(Endpoint, MAGIC_MAC_EXPAND, KeyBuffer, ExpandedEndpointName, sizeof(ExpandedEndpointName));

    if (Configuration.useDtlsPsk)
    {
        Configuration.pskIdentity = CfgParser_GetDtlsPskIdentity();
        if (EmptyCfg(Configuration.pskIdentity))
        {
            Configuration.pskIdentity = ExpandedEndpointName;
        }
        else
        {
            /* expand MAC */
            expandToken(Configuration.pskIdentity, MAGIC_MAC_EXPAND, KeyBuffer, ExpandedPskIdentity, sizeof(ExpandedPskIdentity));
            /* expand LWM2M EP */
            expandToken(ExpandedPskIdentity, MAGIC_EP_EXPAND, ExpandedEndpointName, KeyBuffer, sizeof(KeyBuffer));
            /* copy expanded identity to destination */
            strncpy(ExpandedPskIdentity, KeyBuffer, sizeof(ExpandedPskIdentity) - 1);
            Configuration.pskIdentity = ExpandedPskIdentity;
        }
        printf("DTLS/PSK identity   %s\r\n", Configuration.pskIdentity);
        Configuration.pskSecretKey = CfgParser_GetDtlsPskSecretKeyHex();
        if (EmptyCfg(Configuration.pskSecretKey))
        {
            Configuration.pskSecretKey = CfgParser_GetDtlsPskSecretKey();
            if (EmptyCfg(Configuration.pskSecretKey))
            {
                /* Need to provide the SecretKey whenever trying to registering with the security this
                 * conditional check will avoid the user to understand the problem while serval restrict to register without
                 * secretkey */
                printf("DTLS/PSK please Provide the secret key!\r\n");
                assert(false);
            }
            Configuration.pskSecretKeyLen = strlen(Configuration.pskSecretKey);

            printf("DTLS/PSK secret key  %s\r\n", Configuration.pskSecretKey);
        }
        else
        {
            /* Serval 1.7 uses a string for Lwm2mSecurityInfo_T.secret_key. */
            /* This limitation is removed in serval 1.8, */
            /* Therefore the decoding below must be adjusted when updating to serval 1.8! */
            Configuration.pskSecretKeyLen = decodeHexToBin(CfgParser_GetDtlsPskSecretKeyHex(), KeyBuffer, FALSE);
            Configuration.pskSecretKey = KeyBuffer;
            printf("DTLS/PSK secret key in hex, %d bytes\r\n", Configuration.pskSecretKeyLen);
            printf("   %s\r\n", CfgParser_GetDtlsPskSecretKeyHex());
        }
    }
    Binding = CfgParser_GetLwm2mDeviceBinding();

    printf("Serval %d.%d.%d %s %s\r\n", SERVAL_VERSION_MAJOR, SERVAL_VERSION_MINOR, SERVAL_VERSION_PATCH, __DATE__, __TIME__);
    printf("Lwm2m Interface endpointName: %s, binding: %s, notifies: %s\r\n", ExpandedEndpointName, Binding, EnableConNotifies ? "CON" : "NON");
    rc = Lwm2mInterfaceInitialize(ExpandedEndpointName, MacAddress, Binding, Configuration.useDtlsPsk, EnableConNotifies, TestMode);
    if (RC_OK != rc)
    {
        printf("LWM2M Interface Failed to Initialize %i \r\n", rc);
        return;
    }

    InitWlanControl();
    if (SUCCESS != InitButtonMan(CFG_TESTMODE_OFF != TestMode))
    {
        printf("ButtonManagerFailed to Initialize\r\n");
    }

    InitSntpTime();

    rc = Lwm2mInterfaceStart(&Configuration);
    if (RC_OK == rc)
    {
        printf("Lwm2m Interface started, port: %u\r\n", Configuration.localPort);
    }
    else
    {
        printf("LWM2M Interface Failed to start %i \r\n", rc);
    }

}

/* global functions ********************************************************** */

/**
 * @brief This is a template function where the user can write his custom application.
 *
 */
void appInitSystem(void * CmdProcessorHandle, uint32_t param2)
{
    if (CmdProcessorHandle == NULL)
    {
        printf("Command processor handle is null \n\r");
        assert(false);
    }
    AppCmdProcessorHandle = (CmdProcessor_T *)CmdProcessorHandle;
    BCDS_UNUSED(param2);
    Init();
}


CmdProcessor_T * GetAppCmdProcessorHandle(void)
{
    return AppCmdProcessorHandle;
}

/**@} */
/** ************************************************************************* */
