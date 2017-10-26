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
#ifndef XDK110_APPLWM2MINTERFACE_H_
#define XDK110_APPLWM2MINTERFACE_H_


#if SERVAL_ENABLE_LWM2M
#include <Serval_Network.h>

/* public type and macro definitions */

/** enum to represent LED return status */
enum led_return_E
{
    RETURN_FAILURE,
    RETURN_SUCCESS,
};

typedef enum led_return_E led_return_t;
/**
 * LWM2M configuration
 */
struct Lwm2mConfiguration_S
{
	bool        sdCardConfig; /**< LWM2M server is read from SD Card */
	uint16_t    localPort; /**< port number of LWM2M Interface between Device and LWM2M Server */
	const char* destServer; /**< LWM2M server for registering and data exchange */
	uint32_t    lifetime; /**< lifetime for registration updates */
	bool        useDtlsPsk; /**< indicates use of DTLS/PSK */
	const char* pskIdentity; /**< DTLS/PSK identity */
	const char* pskSecretKey; /**< DTLS/PSK secret key */
	uint8_t     pskSecretKeyLen; /**< DTLS/PSK secret key length */
};

typedef struct Lwm2mConfiguration_S Lwm2mConfiguration_T;

/**
 * Callback handler for LED state changes.
 *
 * @param[in] state new state of LED. true for LED on, false, for off
 */
typedef void (*LedStateChangeHandler_T)(bool state);


/**
 * @brief The function initializes the LWM2M Interface between Device and the LWM2M Server.
 * @param[in] endpointName  LWM2M endpoint name. The parameter is used as reference
 *                          therefore the endpoint name must stay valid after the call.
 * @param[in] macAddr MAC Address supporting the application. The parameter is used as reference
 *                          therefore MAC address must stay valid after the call.
 * @param[in] binding device binding. "U" for UDP, "UQ" for UDP with Queuing.
 * @param[in] secureMode  setting the security (currently implicit PSK)
 * @param[in] testMode    true, to enable test mode for LEDs, false, otherwise
 *
 * @return  RC_OK Interface Initialization was successful.
 * @return  RC_SERVAL_ERROR Initialization Failure.
 */
extern retcode_t Lwm2mInterfaceInitialize(const char* endpointName, const char* macAddr, const char* binding, bool secureMode, bool enableConNotifies, uint8_t testMode);

/**
 * @brief The function Starts the LWM2M Interface to the LWM2M Server to the configured port Number.
 * @param[in] configuration configuration of LWM2M
 *
 * @return  RC_OK Interface Started between Device and LWM2M Server.
 * @return  RC_SERVAL_ERROR Interface Not started to the Configured Port Number.
 */
extern retcode_t Lwm2mInterfaceStart(const Lwm2mConfiguration_T *configuration);

/**
 * @brief Reboot the device.
 *
 * When the function is called first, it reboots with a delay of 5s to have time to send a response.
 * Though reboot is sometimes used for undetermined working devices, the delay of 5 s may not work.
 * Therefore, a second call of this function reboots the device immediately (and without sending a response).
 */
extern void Lwm2mInterfaceReboot(void);

extern void Lwm2mInterfaceTriggerRegister(void);

/**
 * @brief Set state of the red LED.

 * Set state of the red LED and reports that to the associated handler.
 * @see Lwm2mInterfaceSetRedLedStateChangeHandler
 *
 * @param[in] on state of LED
 */
extern void Lwm2mInterfaceRedLedSetState(bool on);

/**
 * @brief Set state of the orange LED.
 *
 * Set state of the orange LED and reports that to the associated handler.
 * @see Lwm2mInterfaceSetOrangeLedStateChangeHandler
 *
 * @param[in] on state of LED
 */
extern void Lwm2mInterfaceOrangeLedSetState(bool on);

/**
 * @brief Set state of the yellow LED.
 *
 * Set state of the yellow LED and reports that to the associated handler.
 * @see Lwm2mInterfaceSetOrangeLedStateChangeHandler
 *
 * @param[in] on state of LED
 */
extern void Lwm2mInterfaceYellowLedSetState(bool on);

/**
 * @brief Set change handler for red LED.
 *
 * @param[in] handler callback for LED state changes
 */
extern void Lwm2mInterfaceSetRedLedStateChangeHandler(LedStateChangeHandler_T handler);

/**
 * @brief Set change handler for orange LED.
 *
 * @param[in] handler callback for LED state changes
 */
extern void Lwm2mInterfaceSetOrangeLedStateChangeHandler(LedStateChangeHandler_T handler);

/**
 * @brief Set change handler for yellow LED.
 *
 * @param[in] handler callback for LED state changes
 */
extern void Lwm2mInterfaceSetYellowLedStateChangeHandler(LedStateChangeHandler_T handler);

extern void Lwm2mInterfaceRebootNow(const char *msg);

/**
 * @brief This function is to convert the integer data into a string data
 *
 * @param[in] appVersion - 32 bit value containing the FOTA version information
 *
 * @param[in] xdkVersion - 32 bit value containing the XDK SW version information
 *
 * @param[in/out] str - pointer to the parsed version string
 *
 * @param [out] Returns index where the application version starts
 *
 * @Note 1. FOTA capable device only has the Valid application firmware version in that case output format
 *          will be example:v3.0.1-0.0.1( XDK Version + Application version) <br>
 *       2. For Non- FOTA capable device version number will be v3.0.1-xx.xx.xx( XDK Version-xx.xx.xx) <br>
 *       3. Also note that the XDK version number is represented in Decimal format (i.e if XDK version is 3.0.1).<br>
 *       	the following macros maintain to represent the XDK SW release version
 *				#define XDKVERSION_MAJOR        3
 *				#define XDKVERSION_MINOR        0
 *				#define XDKVERSION_PATCH        1
 *		 4. Also note that the Application version number is represented in Decimal format.
 *		 example: if user modifies the application version value macros under application.mk as follows
 *				MAJOR_SW_NO ?= 0xFF
 * 				MINOR_SW_NO ?= 0xFF
 * 				PATCH_SW_NO ?= 0xFF
 * 		so, the appVersion will be represented as 255.255.255
 *
 */
uint8_t* convert32IntegerToVersionString(uint32_t appVersion, uint32_t xdkVersion, uint8_t* str);

#endif /* SERVAL_ENABLE_LWM2M */

#endif /* XDK110_APPLWM2MINTERFACE_H_ */
