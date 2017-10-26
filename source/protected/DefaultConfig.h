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
 * @brief
 *
 * @file
 **/

/* header definition ******************************************************** */
#ifndef DEFAULTCONFIG_H_
#define DEFAULTCONFIG_H_

/* public type and macro definitions */
#warning Please enter WLAN configurations with valid SSID & WPA key in below Macros and remove this line to avoid warning.
#define WLAN_CONNECT_WPA_SSID           "NETWORK_SSID"                  /**< Macros to define WPA/WPA2 network settings */
#define WLAN_CONNECT_WPA_PASS           "NETWORK_WPA_KEY"               /**< Macros to define WPA/WPA2 network settings */

#warning Please enter LWM2M server configuration and remove this line to avoid warning.
/* use "coap://leshan.eclipse.org:5683" or "coaps://leshan.eclipse.org:5684" to select plain or encrypted communication to communicate with eclipse leshan server for testing purpose */
/* use "coaps://lwm2m.eu-1.bosch-iot-suite.com:5684" to have encrypted Communication with the Bosch IOT Cloud */
#define LWM2M_SERVER_IP                 "coap://leshan.eclipse.org:5683" /**< using standard CoAP ports unsecure 5683 secure 5684, giving an appending port will override */

#define LWM2M_DEFAULT_LIFETIME          "240"                            /**< Macro to represent the default lifetime in s. */
#define LWM2M_ENDPOINT_NAME             "lwm2m.xdk:#---MAC----#"         /**< Macro to represent the lwm2m endpoint name. Magic #*# is replaced by MAC */

#warning Please enter identity and SecretKey, if encrypted communication is wanted and remove this line to avoid warning.
/* LWM2M_DTLS_PSK_IDENTITY, if empty LWM2M_ENDPOINT_NAME is used */
#define LWM2M_DTLS_PSK_IDENTITY          ""            /**< DTLS/PSK default identity */
#define LWM2M_DTLS_PSK_SECRET_KEY_HEX    ""            /**< DTLS/PSK default secret key hexadecimal*/

#endif /* DEFAULTCONFIG_H_ */

/** ************************************************************************* */
