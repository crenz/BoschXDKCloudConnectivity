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
	#ifndef DNS_UTIL_H_
#define DNS_UTIL_H_

#include <Serval_Types.h>

#define RC_MODULE_DNS           (0x1B << RC_SHIFT_MODULE) /**< MODULE code for DNS */

#define RC_DNS_OK               RC_OK /**< DNS lookup OK */
#define RC_DNS_BUSY             ((retcode_t)(0x01 | RC_MODULE_DNS | RC_SEVERITY_MINOR)) /**< DNS lookup busy by already pending lookup */
#define RC_DNS_PENDING          ((retcode_t)(0x02 | RC_MODULE_DNS | RC_SEVERITY_MINOR)) /**< DNS lookup pending. Result will be reported by callback */

#define DNS_TIMEOUT_IN_S        UINT32_C(5) /**< timeout for repeat DNS lookup. Note, the DNS lookup may take longer but it's not repeated then! */

/**
 * Callback for asynchronous DNS lookup.
 *
 * @param rc RC_OK, if DNS lookup was successful, false, otherwise.
 * @param resolvedURL resolved URL, if rc is RC_OK
 * @param context context provided when call \link Dns_StartResolveHostname \endlink or
 * \link Dns_ExtendedResolveHostname \endlink
 */
typedef void (*DnsCallback_T)(retcode_t rc, const char* resolvedURL, void* context);

/**
 * @brief Initialize DNS module.
 *
 * Must be called before any other function of this module.
 *
 * @return RC_OK, if successful, false, otherwise.
 */
extern retcode_t Dns_Init(void);

/**
 * Start asynchronous DNS lookup. Report result using the callback.
 * @param URL url to resolve
 * @param timeout timeout for retries.
 * @param callback callback for result
 * @param context context to be passed to callback.
 * @return RC_DNS_PENDING, when starting was successful,
 *         RC_DNS_BUSY, when asynchronous DNS lookup is already pending,
 *         and RC_PLATFORM_ERROR in case of an error.
 */
extern retcode_t Dns_StartResolveHostname(const char * const URL, uint32_t timeout, const DnsCallback_T callback, void* context);

/**
 * Resolve URL.
 *
 * @param URL url to resolve
 * @param resolvedURL buffer for resolved url
 * @param sizeResolvedURL size of buffer for resolved url.
 * @param timeout retry timeout in seconds
 * @return RC_DNS_OK, if url was resolved successfully, RC_PLATFORM_ERROR, if an error occurred.
 */
extern retcode_t Dns_ResolveHostname(const char * const URL, char * const resolvedURL, const size_t sizeResolvedURL, uint32_t timeout);

/**
 * Extended resolve URL.
 * If URL uses literal host (e.g. "192.168.1.10"), the URL is just copied to provided resolvedURL buffer.
 * Otherwise, either synchronous, if no callback is provided (NULL), or asynchronous, if callback is provided.
 * @param URL url to resolve. Format "[proto://]host[:port][/path]".
 * @param resolvedURL buffer for resolved url, if URL contains literal host or callback is NULL
 * @param sizeResolvedURL size of buffer for resolved url, if URL contains literal host or callback is NULL
 * @param timeout retry timeout in seconds
 * @param callback callback for result, if asynchronous lookup should be executed.
 * @param context context for callback.
 * @return RC_DNS_OK, if url was resolved successfully,
 *         RC_DNS_PENDING, when starting was successful,
 *         RC_DNS_BUSY, when asynchronous DNS lookup is already pending,
 *         and RC_PLATFORM_ERROR in case of an error.*
 */
extern retcode_t Dns_ExtendedResolveHostname(const char * const URL, char * const resolvedURL, const size_t sizeResolvedURL, uint32_t timeout, const DnsCallback_T callback, void* context);

#endif /* DNS_UTIL_H_ */
