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
/**
 * @ingroup BOSCH_XDK_CLOUD_CONNECTIVITY
 *
 * @defgroup LWM2M_INTERFACE LWM2M Interface
 * @{
 *
 * @brief This file provides the Implementation of Lwm2mInterface.
 *
 * @details LWM2M Interface source file
 *
 * @file
 **/
/* own header files */

/*lint -emacro(413, CALLABLE_GET_CONTEXT, GET_CONTEXT_OF_MEMBER) */
/*lint -ecall(534, Callable_assign) */
/*lint -emacro(641, RC_RESOLVE) */

#include "XDKAppInfo.h"
#undef BCDS_MODULE_ID  /* Module ID define before including Basics package*/
#define BCDS_MODULE_ID XDK_APP_MODULE_ID_SENSOR_LWM2M_INTERFACE

#include "Lwm2mInterface.h"

#if SERVAL_ENABLE_COAP_SERVER

/* additional interface header files */
#include <PAL_initialize_ih.h>
#include <Serval_Log.h>
#include <Serval_Timer.h>
#include <Serval_CoapClient.h>
#include <Serval_Clock.h>
#include <Serval_Scheduler.h>
#include <Serval_Lwm2m.h>
#include <Serval_OpControl.h>

#include "BCDS_CmdProcessor.h"
#include "BCDS_BSP_Board.h"

#include "BSP_BoardType.h"
#include "BCDS_BSP_LED.h"

#include "Lwm2mObjects.h"
#include "DnsUtil.h"
#include "CfgParser.h"

#include "BCDS_WlanConnect.h"
#include "FotaDownload.h"

#define LOG_MODULE "CCA"

#define LWM2M_REPORTING_SUPPORT_SUSPEND 1

#if SERVAL_ENABLE_DTLS_SESSION_ID
#define LWM2M_MAX_DTLS_SESSION_ID_SIZE  32
#else
#define LWM2M_MAX_DTLS_SESSION_ID_SIZE  0
#endif

#define SYSTEM_TIMER_INTERVAL_IN_MS UINT16_C(1000)

#define REBOOT_STATE_INIT           UINT8_C(0) /**< Initial state for reboot */
#define REBOOT_STATE_START          UINT8_C(1) /**< reboot started, stop registration handling, wait sometime for the ACK to be sent */
#define REBOOT_STATE_EXECUTE        UINT8_C(5) /**< execute reboot, gracefully, set serval to sleep (stores MID/Token, if possible) */
#define REBOOT_STATE_EMERGENCY      UINT8_C(15) /**< emergency reboot, if graceful reboot could not be done in time */

#define LWM2M_REGISTRATION_SCHEDULED 1  /**< calls to Lwm2mRegistration_update/registration from serval scheduler */
#define LWM2M_REGISTRATION_UPDATE_INTERVAL(LIFETIME) (((LIFETIME)*4)/5) /**< calculate update interval from lifetime */

#define REBOOT_MSG_DELAY_IN_MS      UINT8_C(100) /**< last sleep on reboot to give logging a chance */

#define DNS_REPEAT_TIME_IN_S        UINT16_C(60 * 5) /**< repeat DNS lookup after 5 min (after tests => 12 h) */
#define MAX_SERVAL_SCHEDULER_TRIES  UINT8_C(20)  /**< maximum number of TimeChanged calls with pending registration callback before LED alert */

#define RED_LED_HOLD_TIME_IN_MS     UINT32_C(5000) /**< time in milliseconds to stick at least on the value set by \link Lwm2mInterfaceRedLedSetState \endlink */
#define ORANGE_LED_HOLD_TIME_IN_MS  UINT32_C(5000) /**< time in milliseconds to stick at least on the value set by \link Lwm2mInterfaceOrangeLedSetState \endlink */
#define YELLOW_LED_HOLD_TIME_IN_MS  UINT32_C(2000) /**< time in milliseconds to stick at least on the value set by \link Lwm2mInterfaceYellowLedSetState \endlink */

#define LED_MUTEX_TIMEOUT           (100/(portTICK_PERIOD_MS)) /**< timeout for LED mutex functions */

#define MILLIS_TO_SEC(MS)           ((MS)/UINT32_C(1000)) /**< convert milliseconds to seconds */

/*lint -save -e956 */

/* global variables ********************************************************* */

/* local variables ********************************************************** */

static uint32_t RedLedHandle = (uint32_t) BSP_XDK_LED_R; /**< variable to store red led handle */
static uint32_t OrangeLedHandle = (uint32_t) BSP_XDK_LED_O; /**< variable to store orange led handle */
static uint32_t YellowLedHandle = (uint32_t) BSP_XDK_LED_Y; /**< variable to store yellow led handle */

static LedStateChangeHandler_T RedLedStateChangeHandler = NULL; /**< led state change handler to report changes of the red LED */
static LedStateChangeHandler_T OrangeLedStateChangeHandler = NULL; /**< led state change handler to report changes of the orange LED */
static LedStateChangeHandler_T YellowLedStateChangeHandler = NULL; /**< led state change handler to report changes of the yellow LED */
static volatile uint64_t LastSetRed = 0; /**< system time in milliseconds of the last set by \link Lwm2mInterfaceRedLedSetState \endlink */
static volatile uint64_t LastSetOrange = 0; /**< system time in milliseconds of the last set by \link Lwm2mInterfaceOrangeLedSetState \endlink */
static volatile uint64_t LastSetYellow = 0; /**< system time in milliseconds of the last set by \link Lwm2mInterfaceYellowLedSetState \endlink */
static bool UseSdCardConfig = false; /**< indicate, that configuration is read from sd-card. symmetric yellow blinking */
static bool UseLedsForRegistration = false; /**< indicate, to use LEDs for registration interface */

enum ServerRegistrationState_E
{
	SRS_NONE,
	SRS_ENABLED,
	SRS_RETRY_REGISTRATION,
	SRS_REGISTRATION_PENDING,
	SRS_REGISTRATED,
	SRS_UPDATE_PENDING,
	SRS_DEREGISTRATION_PENDING,
	SRS_SEND_FAILED,
	SRS_REGISTRATION_TIMEOUT,
	SRS_UPDATE_TIMEOUT,
};

typedef enum ServerRegistrationState_E ServerRegistrationState_T;

enum DnsState_E
{
	DNS_NONE,
	DNS_HOST,
	DNS_PENDING,
	DNS_RESOLVED,
	DNS_ERROR
};

typedef enum DnsState_E DnsState_T;

typedef struct RegistrationInfo_S
{
	ServerRegistrationState_T state;
	DnsState_T dnsState;
	uint32_t lastRegistrationLifetime;
	uint32_t lastRegistrationSent;
	uint32_t lastRegistrationMessage;
	uint32_t lastDns;
	Callable_T refreshCallable;
	char destination[64];
	char destinationResolved[64];
#if LWM2M_MAX_DTLS_SESSION_ID_SIZE
	uint8_t session[LWM2M_MAX_DTLS_SESSION_ID_SIZE + 1];
#endif
	bool retry;
}RegistrationInfo_T;

static RegistrationInfo_T ServerRegistrationInfo[LWM2M_MAX_NUMBER_SERVERS];

/** Reboot task for serval scheduler */
static Callable_T RebootCallable;

/** variable to store timer handle for time changed*/
static xTimerHandle TimeChangeTimer_ptr = NULL;

/** Mutex for LED timing */
static xSemaphoreHandle LedMutexHandle;
/*lint -restore */

/** start indicator. Used for fast registration update to validate proper registration. */
static volatile uint8_t Started = 0;
static volatile uint8_t RebootState = REBOOT_STATE_INIT; /**< reboot state. */

/* local functions ********************************************************** */

static void AdjustLastSetLedTime(volatile uint64_t* ledTime)
{
	uint64_t now;
	if (RC_OK == Clock_getTimeMillis(&now))
	{
		if (pdTRUE == xSemaphoreTake(LedMutexHandle, LED_MUTEX_TIMEOUT))
		{
			*ledTime = now;
			if (pdTRUE != xSemaphoreGive(LedMutexHandle))
			{
				assert(false);
			}
		}
	}
}

static bool CheckLastSetLedTime(volatile uint64_t* ledTime, uint64_t now, uint32_t timeout)
{
	uint64_t time = 0;
	if (pdTRUE == xSemaphoreTake(LedMutexHandle, LED_MUTEX_TIMEOUT))
	{
		time = (*ledTime);
		if (pdTRUE != xSemaphoreGive(LedMutexHandle))
		{
			assert(false);
		}
	}
	return (time + timeout) < now;
}

/**
 * @brief Get registration state name. Usually used by logging.
 * @param[in] state registration state
 * @return name of registration state
 */
static const char* GetRegistrationStateName(ServerRegistrationState_T state)
{
	switch (state)
	{
		case SRS_NONE:
		return "NONE";
		case SRS_ENABLED:
		return "ENABLED";
		case SRS_RETRY_REGISTRATION:
		return "RETRY REGISTER";
		case SRS_REGISTRATION_PENDING:
		return "REGISTER PENDING";
		case SRS_REGISTRATED:
		return "REGISTERED";
		case SRS_UPDATE_PENDING:
		return "UPDATE PENDING";
		case SRS_DEREGISTRATION_PENDING:
		return "DEREG PENDING";
		case SRS_SEND_FAILED:
		return "SEND FAILED";
		case SRS_REGISTRATION_TIMEOUT:
		return "REGISTER TIMEOUT";
		case SRS_UPDATE_TIMEOUT:
		return "UPDATE TIMEOUT";
	}
	return "?!?";
}
/**
 * @brief Convert Decimal/Hex value to String Format to display Version Information.
 *
 * @param[in] data - Number which needs to be convert into string to display in Decimal instead of HEX values.
 *
 * @param[in/out] *str - pointer to Store/write the Version number in String Format.
 *
 */
static uint8_t * convDecToStr(uint8_t data , uint8_t * str )
{
	uint8_t arr[3] = {0,0,0};
	uint8_t noOfDigits = 0x00;
	uint8_t index =0x00;
	do
	{
		arr[index] = data % 10;
		data = data/10;
		index++;
		noOfDigits++;
	}while (data != (0x00));

	for(int8_t i=(noOfDigits-1);(i>=0);i--)
	{
		*str = arr[i]+0x30;
		str++;
	}
	return str;
}

/* Function Description available in private header file */
uint8_t* convert32IntegerToVersionString(uint32_t appVersion, uint32_t xdkVersion, uint8_t* str)
{
	uint8_t shiftValue = UINT8_C(16);
	uint8_t* startPtrAppVersion = NULL;
	uint8_t byteVar=0x00;
	*str = 'v';
	for (uint8_t i = 0; i < 3; i++)
	{
		str++;
		byteVar = ((xdkVersion >> shiftValue) & 0xFF);
		str = convDecToStr(byteVar,str);
		*str = '.';
		shiftValue = shiftValue - 8;
	}
	*str = '-';
	startPtrAppVersion = str;
	shiftValue = UINT8_C(16);

	for (uint8_t i = 0; i < 3; i++)
	{
		str++;
		byteVar = ((appVersion >> shiftValue) & 0xFF);
		str = convDecToStr(byteVar,str);
		*str = '.';
		shiftValue = shiftValue - 8;
	}
	*str = '\0';
	return (startPtrAppVersion+1);
}

/**
 * @brief Get DNS state name. Usually used by logging.
 * @param[in] state DNS state
 * @return name of DNS state
 */
static const char* GetDnsStateName(DnsState_T state)
{
	switch (state)
	{
		case DNS_NONE:
		return "";
		case DNS_HOST:
		return "DNS HOST ";
		case DNS_PENDING:
		return "DNS PENDING ";
		case DNS_RESOLVED:
		return "DNS RESOLVED ";
		case DNS_ERROR:
		return "DNS ERROR ";
	}
	return "?!?";
}

/**
 * @brief Get server index by registration info.
 * @param[in] info registration info
 * @return server index, LWM2M_MAX_NUMBER_SERVERS, if no server index for provided server info was not found.
 */
static uint8_t GetServerIdxByInfo(RegistrationInfo_T * info)
{
	uint8_t ServerIdx;

	for (ServerIdx = 0; ServerIdx < LWM2M_MAX_NUMBER_SERVERS; ++ServerIdx)
	{
		if (info == &ServerRegistrationInfo[ServerIdx])
		{
			break;
		}
	}

	return ServerIdx;
}

/**
 * @brief Set led state.
 * @param[in] handle handle to LED
 * @param[in] operation LED operation such as ON , OFF etc
 * @param[in] handler led state change handler
 */
static void Interface_SetLedState(uint32_t handle, uint32_t operation, LedStateChangeHandler_T handler)
{
	/*At present the handle can either be BSP_XDK_LED_R ,BSP_XDK_LED_O , BSP_XDK_LED_Y */
	assert((uint32_t )BSP_XDK_LED_Y >= handle);
	Retcode_T Result = BSP_LED_Switch(handle, operation);
	if (RETCODE_OK == Result)
	{
		if (NULL != handler)
		{
			switch (operation)
			{
				case BSP_LED_COMMAND_OFF:
				handler(false);
				break;
				case BSP_LED_COMMAND_ON:
				handler(true);
				break;
				default:
				break;
			}
		}
	}
}

/**
 * @brief Adjust LEDs according registration state.
 * @param[in] serverIdx server index. Currently only index 0 is supported.
 */
static void Interface_RegisterStateToLed(uint8_t serverIdx)
{
	if (UseLedsForRegistration && (0 == serverIdx))
	{
		switch (ServerRegistrationInfo[serverIdx].state)
		{
			case SRS_NONE:
			Interface_SetLedState(RedLedHandle, (uint32_t) BSP_LED_COMMAND_OFF, RedLedStateChangeHandler);
			Interface_SetLedState(OrangeLedHandle, (uint32_t) BSP_LED_COMMAND_OFF, OrangeLedStateChangeHandler);
			break;
			case SRS_ENABLED:
			Interface_SetLedState(RedLedHandle, (uint32_t) BSP_LED_COMMAND_ON, RedLedStateChangeHandler);
			Interface_SetLedState(OrangeLedHandle, (uint32_t) BSP_LED_COMMAND_OFF, OrangeLedStateChangeHandler);
			break;
			case SRS_REGISTRATED:
			Interface_SetLedState(RedLedHandle, (uint32_t) BSP_LED_COMMAND_OFF, RedLedStateChangeHandler);
			Interface_SetLedState(OrangeLedHandle, (uint32_t) BSP_LED_COMMAND_ON, OrangeLedStateChangeHandler);
			break;
			default:
			break;
		}
	}
}

enum RegistrationSessionState_E
{
	NO_SESSION,
	SAME_SESSION,
	NEW_SESSION,
	LOST_SESSION,
	CHANGED_SESSION,
};

typedef enum RegistrationSessionState_E RegistrationSessionState_T;

#if LWM2M_MAX_DTLS_SESSION_ID_SIZE

static RegistrationSessionState_T RegistrationUpdateSession(uint8_t ServerIdx)
{
	uint8_t session[LWM2M_MAX_DTLS_SESSION_ID_SIZE + 1] = {LWM2M_MAX_DTLS_SESSION_ID_SIZE};
	retcode_t rc = Lwm2mRegistration_getSecureConnId(ServerIdx, &session[0], &session[1]);

	if (RC_OK != rc)
	{
		session[0] = 0;
	}

	if ((0 < ServerRegistrationInfo[ServerIdx].session[0]) && (0 < session[0]))
	{
		if (0 == memcmp(ServerRegistrationInfo[ServerIdx].session, session, session[0]))
		{
			return SAME_SESSION;
		}
		else
		{
			memset(ServerRegistrationInfo[ServerIdx].session, 0, sizeof(ServerRegistrationInfo[ServerIdx].session));
			memcpy(ServerRegistrationInfo[ServerIdx].session, session, session[0] + 1);
			return CHANGED_SESSION;
		}
	}
	else if (0 < ServerRegistrationInfo[ServerIdx].session[0])
	{
		memset(ServerRegistrationInfo[ServerIdx].session, 0, sizeof(ServerRegistrationInfo[ServerIdx].session));
		return LOST_SESSION;
	}
	else if (0 < session[0])
	{
		memset(ServerRegistrationInfo[ServerIdx].session, 0, sizeof(ServerRegistrationInfo[ServerIdx].session));
		memcpy(ServerRegistrationInfo[ServerIdx].session, session, session[0] + 1);
		return NEW_SESSION;
	}
	else
	{
		return NO_SESSION;
	}
}
#endif

/**
 * @brief Update registration state. Set state and adjust lastRegistrationMessage and lastRegistrationLifetime
 * according the current values. After startup only called within serval scheduler.
 * @param[in] serverIdx server index
 * @param[in] state server state do be set.
 */
static void UpdateRegistrationState(uint8_t serverIdx, enum ServerRegistrationState_E state)
{
	Lwm2mServer_T* Server = Lwm2m_getServer(serverIdx);
	RegistrationInfo_T * Info = &ServerRegistrationInfo[serverIdx];
	if (NULL != Server && NULL != Info)
	{
#if LWM2M_REPORTING_SUPPORT_SUSPEND
		bool changed = false;
#endif
		bool Failed = (SRS_SEND_FAILED == state);
		bool Pending = (SRS_UPDATE_PENDING == state) || (SRS_REGISTRATION_PENDING == state);

		if (Failed)
		{
			/* SRS_SEND_FAILED is only used to trigger time updates */
			Clock_getTime(&Info->lastRegistrationSent); //lint !e534, ignore return value
			Info->lastRegistrationMessage = Info->lastRegistrationSent;
			Info->lastRegistrationLifetime = Server->lifetime;
			state = SRS_ENABLED;
		}
#if LWM2M_REPORTING_SUPPORT_SUSPEND
		if (SRS_REGISTRATED == state)
		{
			changed = (SRS_REGISTRATED != Info->state && SRS_UPDATE_PENDING != Info->state);
		}
		else if (SRS_ENABLED == state)
		{
			changed = (SRS_ENABLED != Info->state && SRS_REGISTRATION_PENDING != Info->state);
		}
#endif
		Info->state = state;
		if (Pending)
		{
			Clock_getTime(&Info->lastRegistrationSent); //lint !e534, ignore return value
			Info->lastRegistrationLifetime = Server->lifetime;
		}
		else
		{
			if (SRS_ENABLED == state)
			{
				Info->retry = false;
				if (DNS_RESOLVED == ServerRegistrationInfo[serverIdx].dnsState)
				{
					/** reset DNS in case of changed address caused failure */
					ServerRegistrationInfo[serverIdx].dnsState = DNS_HOST;
				}
#if LWM2M_REPORTING_SUPPORT_SUSPEND
				if (changed)
				{
					Lwm2mReporting_suspendNotifications(serverIdx);
				}
#endif
			}
			else if (SRS_REGISTRATED == state)
			{
				uint32_t now;

				Clock_getTime(&now); //lint !e534, ignore return value
				Info->retry = false;
				Info->lastRegistrationMessage = Info->lastRegistrationSent;
				printf("Registration in %u [s]\n", (unsigned int) (now - Info->lastRegistrationSent));

#if LWM2M_REPORTING_SUPPORT_SUSPEND
				if (changed)
				{
					Lwm2mReporting_resumeNotifications(serverIdx, LWM2M_SEND_NOTIFICATIONS);
				}
#endif
#if LWM2M_MAX_DTLS_SESSION_ID_SIZE
				RegistrationUpdateSession(serverIdx); //lint !e534, ignore return value
#endif
			}
			else if (SRS_RETRY_REGISTRATION == state)
			{
				Info->retry = true;
			}
			Interface_RegisterStateToLed(serverIdx);
		}
	}
}

/**
 * @brief Check and send registration interface request.
 *        Intended to be called by serval scheduler. May be called also from timer (experimental), depending on
 * \link LWM2M_REGISTRATION_SCHEDULED \endlink.
 * @param[in] callable serval callable
 * @param[in] status status
 * @return RC_OK, if sent successful or if no request is required.
 *                Otherwise a error code from sending the request is returned.
 */
static retcode_t SendRegistration(Callable_T * callable, retcode_t status)
{
	(void) status;
	static int Counter = 0; //lint !e956, only called by serval scheduler

	retcode_t rc = RC_OK;
	RegistrationSessionState_T sessionState = NO_SESSION;
	RegistrationInfo_T* Info = CALLABLE_GET_CONTEXT(RegistrationInfo_T, refreshCallable, callable);
	uint8_t ServerIdx = GetServerIdxByInfo(Info);

#if (INCLUDE_uxTaskGetStackHighWaterMark == 1)
	/**return the remaining stack */
	uint32_t HighWaterMark = uxTaskGetStackHighWaterMark( NULL );
	printf("scheduler remaining stack = %lu\n\r", HighWaterMark);
#endif

	++Counter;

#if LWM2M_MAX_DTLS_SESSION_ID_SIZE
	sessionState = RegistrationUpdateSession(ServerIdx);
	if ((SAME_SESSION < sessionState) && (SRS_REGISTRATED == Info->state))
	{
		printf("==> DTLS session changed, registration forced\r\n");
		Info->state = SRS_ENABLED;
	}
#endif

	switch (Info->state)
	{
		case SRS_REGISTRATED:
		printf("==> registration update\r\n");
		rc = Lwm2mRegistration_update(ServerIdx);
		if (RC_OK == rc)
		{
			UpdateRegistrationState(ServerIdx, SRS_UPDATE_PENDING);
		}
		break;
		case SRS_RETRY_REGISTRATION:
		case SRS_ENABLED:
		printf("==> registration retry\r\n");
		rc = Lwm2mRegistration_register(ServerIdx);
		if (RC_OK == rc)
		{
			UpdateRegistrationState(ServerIdx, SRS_REGISTRATION_PENDING);
		}
		break;
		case SRS_REGISTRATION_TIMEOUT:
		if (!Info->retry && SAME_SESSION == sessionState)
		{
			/* retry with new DTLS session */
			/*lint -e(534) todo:Need to check on this time being suppressing*/
			Lwm2mRegistration_closeSecureConn(ServerIdx);
			UpdateRegistrationState(ServerIdx, SRS_RETRY_REGISTRATION);
			printf("==> registration timeout, retry\r\n");
		}
		else
		{
#if !SERVAL_ENABLE_DTLS_SESSION_ID
			/*lint -e(534) todo:Need to check on this time being suppressing*/
			Lwm2mRegistration_closeSecureConn(ServerIdx);
#endif
			printf("==> registration timeout\r\n");
			UpdateRegistrationState(ServerIdx, SRS_ENABLED);
		}
		Callable_clear(callable);
		return RC_OK;
		case SRS_UPDATE_TIMEOUT:
		if (!Info->retry && SAME_SESSION == sessionState)
		{
			/* retry with new DTLS session => trigger register instead of update to ensure new observes */
			/*lint -e(534) todo:Need to check on this time being suppressing*/
			Lwm2mRegistration_closeSecureConn(ServerIdx);
			UpdateRegistrationState(ServerIdx, SRS_RETRY_REGISTRATION);
			printf("==> registration update timeout, retry register\r\n");
		}
		else
		{
			printf("==> registration update timeout\r\n");
			UpdateRegistrationState(ServerIdx, SRS_ENABLED);
		}
		Callable_clear(callable);
		return RC_OK;
		default:
		break;
	}

	if (RC_OK == rc)
	{
		printf("==> send registration succeeded %d!\r\n", Counter);
	}
	else
	{
		uint32_t now = 0;
		if (RC_COAP_CLIENT_SESSION_ALREADY_ACTIVE == rc)
		{
			printf("==> BUSY, send registration pending %d!\r\n", Counter);
		}
		else if (RC_COAP_SECURE_CONNECTION_ERROR == rc)
		{
			printf("==> send registration %d, secure error or handshake pending!\r\n", Counter);
		}
		else
		{
			printf("==> send registration failed %d " RC_RESOLVE_FORMAT_STR "\n", Counter, RC_RESOLVE(rc));
		}
		if (RC_OK == Clock_getTime(&now))
		{
			uint32_t last = Info->retry ? Info->lastRegistrationSent : Info->lastRegistrationMessage;
			if ((last + Info->lastRegistrationLifetime) < now)
			{
				printf("==> Registration expired %d!\r\n", Counter);
				/*lint -e(534) todo:Need to check on this time being suppressing*/
				Lwm2mRegistration_closeSecureConn(ServerIdx);
				UpdateRegistrationState(ServerIdx, SRS_SEND_FAILED);
			}
		}
	}
	Callable_clear(callable);

	return rc;
}

/**
 * @brief Reboot device.
 *        Prints last message and wait before reboot the device, if SERVAL_LOG_LEVEL is at least SERVAL_LOG_LEVEL_ERROR.
 * @param msg message to be printed
 */
void Lwm2mInterfaceRebootNow(const char *msg)
{
#if (SERVAL_LOG_LEVEL >= SERVAL_LOG_LEVEL_ERROR)
// this message may be lost because of the reboot
	LOG_ERROR(msg);
// wait to give the above logging a chance
	vTaskDelay(REBOOT_MSG_DELAY_IN_MS / portTICK_PERIOD_MS);
#endif
	BSP_Board_SoftReset();
}

/**
 * @brief Scheduler callback for reboot task.
 * Calls OpControl_trySleep and, if not RC_RETRY_SLEEP_LATER, calls rebootNow.
 * @param callable_ptr (not used)
 * @param status (not used)
 * @return (not used)
 */
static retcode_t CallRebootNow(Callable_T* callable_ptr, retcode_t status)
{
	(void) callable_ptr;
	(void) status;
	retcode_t rc = OpControl_trySleep();
	if (RC_RETRY_SLEEP_LATER != rc)
	{
		Lwm2mInterfaceRebootNow("REBOOTING ...");
	}
	return RC_OK;
}

/**
 * @brief Callback from asynchrony DNS.
 * @param[in] rc result code of DNS lookup.
 * @param[in] resolvedURL resolved URL, if DNS lookup was successful.
 * @param[in] context context pointer.
 */
static void DnsCallback(retcode_t rc, const char* resolvedURL, void * context)
{
	RegistrationInfo_T * Info = context;
	uint8_t ServerIdx = GetServerIdxByInfo(Info);
	Lwm2mServer_T* Server = Lwm2m_getServer(ServerIdx);

	if (RC_OK != rc || sizeof(Server->serverAddress) <= strlen(resolvedURL))
	{
		memset(Server->serverAddress, 0, sizeof(Server->serverAddress));
		Info->dnsState = DNS_ERROR;
		printf("LWM2M server resolve error " RC_RESOLVE_FORMAT_STR "\n", RC_RESOLVE(rc));
	}
	else
	{
		strcpy(Server->serverAddress, resolvedURL);
		Info->dnsState = DNS_RESOLVED;
		printf("LWM2M server resolved: %s\r\n", Server->serverAddress);
	}
}

/**
 * @brief Timer callback to handler registration interface.
 * @param[in] serverIdx server index
 * @param[in] timeMillis system time in milliseconds
 */
static void RegistrationTimer(uint8_t serverIdx, uint64_t timeMillis)
{
	static uint8_t AlertCounter = 0; //lint !e956, only called by timer
	uint32_t Time = MILLIS_TO_SEC(timeMillis);
	Lwm2mServer_T* Server = Lwm2m_getServer(serverIdx);
	RegistrationInfo_T* Info = &ServerRegistrationInfo[serverIdx];
	uint32_t Elapsed = Time - Info->lastRegistrationMessage;
	bool Log = true;
	bool scheduleRegistration = (SRS_REGISTRATION_TIMEOUT == Info->state) || (SRS_UPDATE_TIMEOUT == Info->state) || (SRS_RETRY_REGISTRATION == Info->state);

	if (!scheduleRegistration)
	{
		bool RefreshRegistration = (Info->lastRegistrationLifetime != Server->lifetime);

		if (!RefreshRegistration)
		{
			uint32_t Interval = LWM2M_REGISTRATION_UPDATE_INTERVAL(Info->lastRegistrationLifetime);
			if (0 == Started && SRS_ENABLED == Info->state)
			{
				/* initial register */
				Interval = 0;
				Started = 1;
			}
			else if (1 == Started && SRS_REGISTRATED == Info->state && Interval > 10)
			{
				/* fast register update after first registration since restart */
				Interval = 10;
			}
			RefreshRegistration = (Interval <= Elapsed);
		}

		if (RefreshRegistration)
		{
			if (SRS_REGISTRATED == Info->state || SRS_ENABLED == Info->state)
			{
				if (DNS_NONE != Info->dnsState)
				{
					if (DNS_HOST != Info->dnsState && Time > (Info->lastDns + DNS_REPEAT_TIME_IN_S))
					{
						/* DNs retry */
						Info->dnsState = DNS_HOST;
					}
					if (DNS_HOST == Info->dnsState)
					{
						retcode_t rc = Dns_ExtendedResolveHostname(Info->destination, Info->destinationResolved, sizeof(Info->destinationResolved), DNS_TIMEOUT_IN_S, DnsCallback, Info);
						if (RC_DNS_OK != rc)
						{
							if (RC_DNS_PENDING == rc)
							{
								Info->lastDns = Time;
								Info->dnsState = DNS_PENDING;
								printf("LWM2M server resolve started: %s\r\n", Info->destination);
							}
							else if (RC_DNS_BUSY == rc)
							{
								printf("LWM2M server resolve pending: %s\r\n", Info->destination);
							}
							else
							{
								Info->lastDns = Time;
								Info->dnsState = DNS_ERROR;
								printf("LWM2M server not resolved: %s\r\n", Info->destination);
							}
						}
						else
						{
							printf("LWM2M server resolved: %s\r\n", Info->destinationResolved);
							strncpy(Server->serverAddress, Info->destinationResolved, sizeof(Server->serverAddress) - 1);
							if (sizeof(Server->serverAddress) <= strlen(Info->destinationResolved))
							{
								LOG_ERROR("Resolved LWM2M server truncated to '%s'\n", Server->serverAddress);
							}
							Info->lastDns = Time;
							Info->dnsState = DNS_RESOLVED;
						}
						Log = false;
					}
					RefreshRegistration = (DNS_RESOLVED == Info->dnsState);
				}
				scheduleRegistration = RefreshRegistration;
			}
		}
	}

	if (scheduleRegistration)
	{
		if (!Callable_isAssigned(&Info->refreshCallable))
		{
			printf("Try refresh registration %s (%u sec)\r\n", GetRegistrationStateName(Info->state), (unsigned) Elapsed);
			Callable_assign(&Info->refreshCallable, SendRegistration);
#if LWM2M_REGISTRATION_SCHEDULED
			if (RC_OK != Scheduler_enqueue(&Info->refreshCallable, RC_OK))
			{
				Callable_clear(&Info->refreshCallable);
			}
#else
			Callable_call(&Info->refreshCallable, RC_OK);
#endif
			Log = false;
			AlertCounter = 0;
		}
		else
		{
			if (SRS_REGISTRATED == Info->state && Elapsed > Info->lastRegistrationLifetime)
			{
				Info->state = SRS_ENABLED;
			}
			if (MAX_SERVAL_SCHEDULER_TRIES > AlertCounter)
			{
				++AlertCounter;
			}
		}
	}
	else if (!Callable_isAssigned(&Info->refreshCallable))
	{
		AlertCounter = 0;
	}

	if (Log && (0 == (Elapsed % 20)))
	{
		printf("   %s (since %u sec, lifetime %u sec)\r\n", GetRegistrationStateName(Info->state), (unsigned) Elapsed, (unsigned) Info->lastRegistrationLifetime);
		if (DNS_NONE != Info->dnsState)
		{
			printf("   %s(since %u sec)\r\n", GetDnsStateName(Info->dnsState), (unsigned) (Time - Info->lastDns));
		}
		if (Callable_isAssigned(&Info->refreshCallable))
		{
			printf("   scheduled registration task pending!\r\n");
		}
	}
	if (UseLedsForRegistration && (0 == serverIdx))
	{
		uint32_t Operation = (Elapsed & 1) ? (uint32_t) BSP_LED_COMMAND_OFF : (uint32_t) BSP_LED_COMMAND_ON;
		switch (Info->state)
		{
			case SRS_ENABLED:
			if (MAX_SERVAL_SCHEDULER_TRIES <= AlertCounter)
			{
				/* serval scheduler seems to be blocked */
				if (CheckLastSetLedTime(&LastSetRed, timeMillis, RED_LED_HOLD_TIME_IN_MS)
						&& CheckLastSetLedTime(&LastSetOrange, timeMillis, ORANGE_LED_HOLD_TIME_IN_MS))
				{
					Interface_SetLedState(RedLedHandle, Operation, RedLedStateChangeHandler);
					Interface_SetLedState(OrangeLedHandle, Operation, OrangeLedStateChangeHandler);
				}
			}
			break;
			case SRS_REGISTRATION_PENDING:
			if (CheckLastSetLedTime(&LastSetRed, timeMillis, RED_LED_HOLD_TIME_IN_MS))
			{
				Interface_SetLedState(RedLedHandle, Operation, RedLedStateChangeHandler);
			}
			break;
			case SRS_UPDATE_PENDING:
			if (CheckLastSetLedTime(&LastSetOrange, timeMillis, ORANGE_LED_HOLD_TIME_IN_MS))
			{
				Interface_SetLedState(OrangeLedHandle, Operation, OrangeLedStateChangeHandler);
			}
			break;
			default:
			break;
		}
		if (CheckLastSetLedTime(&LastSetYellow, timeMillis, YELLOW_LED_HOLD_TIME_IN_MS))
		{
			if (!UseSdCardConfig)
			Operation = ((Time - Info->lastRegistrationMessage) & 3) ? (uint32_t) BSP_LED_COMMAND_ON : (uint32_t) BSP_LED_COMMAND_OFF;
			Interface_SetLedState(YellowLedHandle, Operation, YellowLedStateChangeHandler);
		}
	}
}

/**
 * @brief Handler to check the reboot state machine and the registration interface.
 * @param[in] pxTimer
 */
static void TimeChangedHandler(void * param1, uint32_t data)
{
	BCDS_UNUSED(param1);
	BCDS_UNUSED(data);

	retcode_t rc = RC_OK;
	uint64_t TimeMillis = 0;
	static uint64_t LastTimeMillis = 0;
	if (REBOOT_STATE_INIT < RebootState)
	{
		// device reboots
		++RebootState;
		if (REBOOT_STATE_EMERGENCY <= RebootState)
		{
			Lwm2mInterfaceRebootNow("REBOOT timeout => emergency!");
		}
		else if (REBOOT_STATE_EXECUTE <= RebootState)
		{
#if (SERVAL_LOG_LEVEL >= SERVAL_LOG_LEVEL_INFO)
			LOG_INFO("REBOOT execute %u", RebootState);
#else
			printf("REBOOT execute %u\n", RebootState);
#endif
			rc = Scheduler_enqueue(&RebootCallable, RC_OK);
			if (RC_OK != rc)
			{
				LOG_ERROR("REBOOT execute %u failed " RC_RESOLVE_FORMAT_STR, RebootState, RC_RESOLVE(rc));
			}
		}
		return;
	}
	rc = Clock_getTimeMillis(&TimeMillis);
	if (RC_OK != rc)
	{
		LOG_ERROR("Clock_getTime failed! " RC_RESOLVE_FORMAT_STR "\n", RC_RESOLVE(rc));
	}

	if (0 < LastTimeMillis)
	{
		LastTimeMillis = TimeMillis - LastTimeMillis;
		if (SYSTEM_TIMER_INTERVAL_IN_MS < LastTimeMillis)
		{
			LastTimeMillis -= SYSTEM_TIMER_INTERVAL_IN_MS;
			if ((SYSTEM_TIMER_INTERVAL_IN_MS >> 2) < LastTimeMillis)
			{
				printf("Timer delayed %lu ms\r\n", (unsigned long) LastTimeMillis);
			}
		}
	}
	LastTimeMillis = TimeMillis;
	/* though the callback doesn't provide the server info, only one server is supported! */
	RegistrationTimer(0, TimeMillis);
	/* notify system time change after registration */
	ObjectDevice_NotifyTimeChanged();
}

static void TimeChanged(xTimerHandle pxTimer)
{
	BCDS_UNUSED(pxTimer);
	TimeChangedHandler(NULL, UINT32_C(0));
}

/**
 * @brief The function is to get the registration status of the Device while connection to LWM2M Server 
 *	and It will toggle the Orange LED to indicate the Registration Success State and Red LED will indicate the Registration Failure state.
 */
static void RegistrationCallback(uint8_t serverIdx, retcode_t status)
{
	static bool fotainit = true;
	Retcode_T ReturnValue = RETCODE_OK;
	if (status == RC_OK)
	{
		UpdateRegistrationState(serverIdx, SRS_REGISTRATED);
		printf("Registration succeeded\r\n");
		if(fotainit)
		{
		    ReturnValue = FotaInit();

		    if (RETCODE_OK == ReturnValue)
		    {
		    	fotainit = false;
		        printf("Fota Init success \n");
		    }
		    else
		    {
		    	fotainit = true;
		        printf("Fota Init Failed \n");
		    }
		}


	}
	else
	{
		if (RC_COAP_REQUEST_TIMEOUT == status)
		{
			UpdateRegistrationState(serverIdx, SRS_REGISTRATION_TIMEOUT);
		}
		else
		{
			UpdateRegistrationState(serverIdx, SRS_ENABLED);
		}
		printf("registration failed " RC_RESOLVE_FORMAT_STR "\n", RC_RESOLVE(status));
	}
}
/**
 * @brief The function is to get the Updated Registration status of the Device while re-connection to LWM2M Server.
 */
static void RegistrationUpdateCallback(uint8_t serverIdx, retcode_t status)
{
	if (status == RC_OK)
	{
		UpdateRegistrationState(serverIdx, SRS_REGISTRATED);
		printf("Registration update succeeded\r\n");
		if (1 == Started)
		{
			/* first registration update after a restart */
			LOG_INFO("First update register after restart");
			Started = 2;
		}
	}
	else
	{
		if (RC_COAP_REQUEST_TIMEOUT == status)
		{
			UpdateRegistrationState(serverIdx, SRS_UPDATE_TIMEOUT);
		}
		else
		{
			UpdateRegistrationState(serverIdx, SRS_ENABLED);
		}
		printf("registration update failed " RC_RESOLVE_FORMAT_STR "\n", RC_RESOLVE(status));
	}
}
/**
 * @brief The function is to deregister the device in LWM2M Server.
 */
static void DeregistrationCallback(uint8_t serverIdx, retcode_t status)
{
	UpdateRegistrationState(serverIdx, SRS_NONE);
	if (status == RC_OK)
	{
		printf("Deregistration succeeded\r\n");
	}
	else
	{
		printf("deregistration failed " RC_RESOLVE_FORMAT_STR "\n", RC_RESOLVE(status));
	}
}
/**
 * @brief The function will get called when we trying to register to the LWM2M Server with configured port Number and also It will provide the RED & Orange LED indications to represent the Registration process.
 */
static void ApplicationCallback(Lwm2m_Event_Type_T eventType, Lwm2m_URI_Path_T *path, retcode_t status)
{
	(void) path;
	uint8_t ServerIdx = 0; /* though the callback doesn't provide the server info, only one server is supported! */

	if (eventType == LWM2M_EVENT_TYPE_REGISTRATION)
	{
		RegistrationCallback(ServerIdx, status);
	}
	else if (eventType == LWM2M_EVENT_TYPE_REGISTRATION_UPDATE)
	{
		RegistrationUpdateCallback(ServerIdx, status);
	}
	else if (eventType == LWM2M_EVENT_TYPE_DEREGISTRATION)
	{
		DeregistrationCallback(ServerIdx, status);
	}
}

/* temporary, function will be exported by lwm2m_security in future version */
static inline retcode_t Lwm2mInterface_copyToBuffer(OutputBuffer_T* buffer, const char* data, uint16_t len)
{
	if (buffer->writableLength == 0)
	{
		buffer->length = len;
		buffer->content = (char*) data;
		return RC_OK;
	}
	else if (buffer->writableLength >= len)
	{
		buffer->length = len;
		memcpy(buffer->content, data, len);
		return RC_OK;
	}
	else
	{
		return RC_DTLS_INSUFFICIENT_MEMORY;
	}
}

static retcode_t Lwm2mInterface_securityCallback(SecurityToken_T token, SecurityData_T* tokenData)
{
	if (token.type != PSK_PEER_KEY_AND_ID)
	return RC_DTLS_UNSUPPORTED_TOKEN;

	retcode_t rc = Lwm2mSecurity_defaultCallback(token, tokenData);
	if (RC_DTLS_PEER_REJECTED == rc)
	{
		PeerKeyAndIdData_T* data = (PeerKeyAndIdData_T*) tokenData;
		Lwm2mServer_T* Server = Lwm2m_getServer(0);
		rc = Lwm2mInterface_copyToBuffer(&data->ourIdentity, Server->securityInfo.my_identity, strlen(Server->securityInfo.my_identity));
		if (RC_OK == rc)
		{
			/* Serval 1.7 uses a string for Lwm2mSecurityInfo_T.secret_key. */
			/* This limitation is removed in serval 1.8, */
			/* Therefore the code below must be adjusted when updating to serval 1.8! */
			rc = Lwm2mInterface_copyToBuffer(&data->key, (char *)Server->securityInfo.secret_key, Server->securityInfo.secret_key_length);
		}
	}
	return rc;
}

/* global functions ********************************************************* */

/* API documentation is in the header file lwm2mInterface.h*/
retcode_t Lwm2mInterfaceStart(const Lwm2mConfiguration_T *configuration)
{
	const char* Security = "without";
	Ip_Port_T LocalPort;
	retcode_t rc = RC_OK;
	uint32_t Time = 0;
	uint8_t ServerIdx = 0;

	rc = Clock_getTime(&Time);
	if (RC_OK != rc)
	{
		printf("Clock_getTime failed! " RC_RESOLVE_FORMAT_STR "\n", RC_RESOLVE(rc));
	}
	Security_setCallback(Lwm2mInterface_securityCallback);

	Lwm2mServer_T* server = Lwm2m_getServer(ServerIdx);
	if (configuration->useDtlsPsk && configuration->pskIdentity && configuration->pskSecretKey)
	{
		Security = "with";
		strncpy(server->securityInfo.peer_identity, "Leshan", sizeof(server->securityInfo.peer_identity) - 1); // ... hint!
		strncpy(server->securityInfo.my_identity, configuration->pskIdentity, sizeof(server->securityInfo.my_identity) - 1);// unique identity
		if (sizeof(server->securityInfo.my_identity) <= strlen(configuration->pskIdentity))
		{
			LOG_ERROR("PSK identity truncated to '%s'\n", server->securityInfo.my_identity);
		}
		/* Serval 1.7 uses a string for Lwm2mSecurityInfo_T.secret_key. */
		/* This limitation is removed in serval 1.8, */
		/* Therefore the code below must be adjusted when updating to serval 1.8! */
		memcpy(server->securityInfo.secret_key, configuration->pskSecretKey, configuration->pskSecretKeyLen); // as Hex: 4C657368616E

		server->securityInfo.secret_key_length = configuration->pskSecretKeyLen;

		if (sizeof(server->securityInfo.secret_key) <= strlen(configuration->pskSecretKey))
		{
			LOG_ERROR("PSK secret key truncated to '%s'\n", server->securityInfo.secret_key);
		}
	}
	printf("CoAP %s encryption, LWM2M server: %s\r\n", Security, configuration->destServer);

	ServerRegistrationInfo[ServerIdx].lastDns = Time;
	rc = Dns_ResolveHostname(configuration->destServer, ServerRegistrationInfo[ServerIdx].destinationResolved, sizeof(ServerRegistrationInfo[ServerIdx].destinationResolved), DNS_TIMEOUT_IN_S);
	if (RC_OK != rc)
	{
		return rc;
	}
	if (0 != strcmp(configuration->destServer, ServerRegistrationInfo[ServerIdx].destinationResolved))
	{
		printf("LWM2M server resolved: %s\r\n", ServerRegistrationInfo[ServerIdx].destinationResolved);
		ServerRegistrationInfo[ServerIdx].dnsState = DNS_RESOLVED;
		strncpy(ServerRegistrationInfo[ServerIdx].destination, configuration->destServer, sizeof(ServerRegistrationInfo[ServerIdx].destination) - 1);
		if (sizeof(ServerRegistrationInfo[ServerIdx].destination) <= strlen(configuration->destServer))
		{
			LOG_ERROR("URL LWM2M server truncated to '%s'\n", ServerRegistrationInfo[ServerIdx].destination);
		}
	}
	else
	{
		ServerRegistrationInfo[ServerIdx].dnsState = DNS_NONE;
	}
	strncpy(server->serverAddress, ServerRegistrationInfo[ServerIdx].destinationResolved, sizeof(server->serverAddress) - 1);
	if (sizeof(server->serverAddress) <= strlen(ServerRegistrationInfo[ServerIdx].destinationResolved))
	{
		LOG_ERROR("Resolved LWM2M server truncated to '%s'\n", server->serverAddress);
	}
	server->permissions[0] = LWM2M_READ_ALLOWED | LWM2M_WRITE_ALLOWED | LWM2M_EXECUTE_ALLOWED;
	server->lifetime = configuration->lifetime;

	Lwm2m_setNumberOfServers(1);
	UseSdCardConfig = configuration->sdCardConfig;

	LocalPort = Ip_convertIntToPort(configuration->localPort);
	rc = Lwm2m_start(LocalPort, ApplicationCallback);

	if (rc != RC_OK)
	{
		printf("Retry to Register once the registration failure\r\n");
		while (rc != RC_OK)
		{
			printf("Registration Request");
			rc = Lwm2m_start(LocalPort, ApplicationCallback);
		}
		goto exit;
	}

	TimeChangeTimer_ptr = xTimerCreate((const char * const ) "SysTime", /* text name, used for debugging. */
			SYSTEM_TIMER_INTERVAL_IN_MS / portTICK_PERIOD_MS, /* The timer period in ticks. */
			pdTRUE, /* The timers will auto-reload themselves when they expire. */
			(void *) NULL, /* Assign each timer a unique id equal to its array index. */
			TimeChanged /* On expire of the timer this function will be called. */
	);
	if (TimeChangeTimer_ptr == NULL)
	{
		assert(false);
	}

	if (pdTRUE != xTimerStart(TimeChangeTimer_ptr, portMAX_DELAY))
	{
		assert(false);
	}
	ServerRegistrationInfo[0].state = SRS_ENABLED;

// @todo remove after testing the functionality 
//    printf("==> initial registration\r\n");
//    UpdateRegistrationState(ServerIdx, SRS_REGISTRATION_PENDING);
//    if (RC_OK != Lwm2mRegistration_register(ServerIdx))
//    {
//        printf("Failed to send registration request\r\n");
//    }

	exit:
	return rc;
}

/* API documentation is in the header file lwm2mInterface.h*/
void Lwm2mInterfaceReboot(void)
{
	if (REBOOT_STATE_INIT == RebootState)
	{
		LOG_INFO("REBOOT triggered");
		RebootState = REBOOT_STATE_START;
#if LWM2M_REPORTING_SUPPORT_SUSPEND
		Lwm2mReporting_suspendNotifications(0);
#endif
	}
	else
	{
		LOG_INFO("REBOOT already pending %d", RebootState);
	}
}

static inline Lwm2m_Binding_T Lwm2mGetBinding(const char* Binding)
{
	Lwm2m_Binding_T Result = UDP;

	if (NULL != Binding && 0 == strncmp("UQ", Binding, 3))
	{
		Result = UDP_QUEUED;
	}

	return Result;
}

/* API documentation is in the header file lwm2mInterface.h*/
retcode_t Lwm2mInterfaceInitialize(const char* endpointName, const char* macAddr, const char* binding, bool secureMode, bool enableConNotifies, uint8_t testMode)
{
	retcode_t rc2 = RC_OK;

	memset(ServerRegistrationInfo, 0, sizeof(ServerRegistrationInfo));
	for (int idx = 0; idx < LWM2M_MAX_NUMBER_SERVERS; ++idx)
	{
		ServerRegistrationInfo[idx].state = SRS_NONE;
		ServerRegistrationInfo[idx].dnsState = DNS_NONE;
		ServerRegistrationInfo[idx].retry = false;
	}
	DeviceResourceInfo.name = endpointName;
	DeviceResourceInfo.secure = secureMode;
	DeviceResourceInfo.binding = Lwm2mGetBinding(binding);

	LedMutexHandle = xSemaphoreCreateMutex();
	if (NULL == LedMutexHandle)
	{
		assert(false);
	}
	UseLedsForRegistration = CFG_TESTMODE_OFF != testMode;

	Callable_assign(&RebootCallable, CallRebootNow); //lint !e534, ignore return value

	printf("Lwm2m_initialize\r\n");
	rc2 = Lwm2m_initialize(&DeviceResourceInfo);
	if (RC_OK != rc2)
	{
		LOG_ERROR("Lwm2m_initialize failed " RC_RESOLVE_FORMAT_STR "\n", RC_RESOLVE(rc2));
		return rc2;
	}
	rc2 = Dns_Init();
	if (RC_OK != rc2)
	{
		LOG_ERROR("Dns_init failed " RC_RESOLVE_FORMAT_STR "\n", RC_RESOLVE(rc2));
		return rc2;
	}

	/* initialize LWM2M resource synchronization mutex */
	Objects_Init(enableConNotifies);
	ObjectDevice_Init();
	ObjectLightControl_Init();
	ObjectAlertNotification_Init();
	ObjectAccelerometer_Init();
	ObjectBarometer_Init();
	ObjectGyroscope_Init();
	ObjectHumidity_Init();
	ObjectIlluminance_Init();
	ObjectMagnetometer_Init();
	ObjectTemperature_Init();
	ObjectSensorDevice_Init(macAddr);

	/* enable LWM2M objects */
	ObjectDevice_Enable();
	ObjectAlertNotification_Enable();
	ObjectLightControl_Enable(CFG_TESTMODE_ON == testMode);
	ObjectConnectivityMonitoring_Enable();
	ObjectSensorDevice_Enable();

	printf("done\r\n");
	return rc2;
}

void Lwm2mInterfaceTriggerRegister(void)
{
	RegistrationInfo_T* Info = &ServerRegistrationInfo[0];

	if (SRS_REGISTRATED == Info->state)
	{
		printf("==> force re-registration\r\n");
		Info->state = SRS_RETRY_REGISTRATION;
	}
	else if (SRS_ENABLED == Info->state)
	{
		printf("==> force registration\r\n");
		Info->lastRegistrationMessage = 0;
		Started = 0;
	}
	printf("====> %s\r\n", GetRegistrationStateName(Info->state));
}

void Lwm2mInterfaceRedLedSetState(bool on)
{
	AdjustLastSetLedTime(&LastSetRed);
	Interface_SetLedState(RedLedHandle, on ? (uint32_t) BSP_LED_COMMAND_ON : (uint32_t) BSP_LED_COMMAND_OFF, RedLedStateChangeHandler);
}

void Lwm2mInterfaceOrangeLedSetState(bool on)
{
	AdjustLastSetLedTime(&LastSetOrange);
	Interface_SetLedState(OrangeLedHandle, on ? (uint32_t) BSP_LED_COMMAND_ON : (uint32_t) BSP_LED_COMMAND_OFF, OrangeLedStateChangeHandler);
}

void Lwm2mInterfaceYellowLedSetState(bool on)
{
	AdjustLastSetLedTime(&LastSetYellow);
	Interface_SetLedState(YellowLedHandle, on ? (uint32_t) BSP_LED_COMMAND_ON : (uint32_t) BSP_LED_COMMAND_OFF, YellowLedStateChangeHandler);
}

void Lwm2mInterfaceSetRedLedStateChangeHandler(LedStateChangeHandler_T handler)
{
	RedLedStateChangeHandler = handler;
}

void Lwm2mInterfaceSetOrangeLedStateChangeHandler(LedStateChangeHandler_T handler)
{
	OrangeLedStateChangeHandler = handler;
}

void Lwm2mInterfaceSetYellowLedStateChangeHandler(LedStateChangeHandler_T handler)
{
	YellowLedStateChangeHandler = handler;
}

#endif /* SERVAL_ENABLE_COAP_SERVER */

/**@} */
