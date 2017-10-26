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
#ifndef CFGPARSER_H_
#define CFGPARSER_H_

/* public interface declaration ********************************************* */
/* system header files */
#include <BCDS_Retcode.h>
#include <BCDS_Basics.h>

/* public type and macro definitions */

#define CFG_NUMBER_UINT8_ZERO            UINT8_C(0)     /**< Zero value */
#define CFG_POINTER_NULL                 NULL          /**< ZERO value for pointers */

#define CFG_TRUE                         UINT8_C(1)    /**< One value  */
#define CFG_FALSE                        UINT8_C(0)    /**< Zero value */

#define CFG_TESTMODE_OFF                 UINT8_C(0)
#define CFG_TESTMODE_ON                  UINT8_C(1)
#define CFG_TESTMODE_MIX                 UINT8_C(2)

/**
 * Configuration array cell element
 */
struct ConfigLine_S
{
    const char *attName; /**< Attribute name at the configuration file */
    const char *defaultValue; /**< Attribute default value */
    uint8_t ignore; /**< To specify if the attribute is ignored/deprecated */
    uint8_t defined; /**< To specify if the attribute has been read from the configuration file */
    char *attValue; /**< Attribute value  at the configuration file */
};

typedef struct ConfigLine_S ConfigLine_T;

/**
 * INDEX at the configuration array
 */
enum AttributesIndex_E
{
    ATT_IDX_SSID,
    ATT_IDX_PASSWORD,
    ATT_IDX_STATIC,
    ATT_IDX_STATICIP,
    ATT_IDX_DNSSERVER,
    ATT_IDX_GATEWAY,
    ATT_IDX_MASK,
    ATT_IDX_LWM2M_DEFAULT_SERVER,
    ATT_IDX_LWM2M_ENDPOINT,
    ATT_IDX_DTLS_PSK_IDENTITY,
    ATT_IDX_DTLS_PSK_SECRET_KEY,
    ATT_IDX_DTLS_PSK_SECRET_KEY_HEX,
    ATT_IDX_TEST_MODE,
    ATT_IDX_LWM2M_LIFETIME,
    ATT_IDX_LWM2M_BINDING,
    ATT_IDX_LWM2M_CON_NOTIFIES,
    ATT_IDX_LWM2M_DTLS_SUFFIX,
    ATT_IDX_ACTIVATE_DTLS_PSK,
    ATT_IDX_MAX = ATT_IDX_ACTIVATE_DTLS_PSK,
    ATT_IDX_SIZE = ATT_IDX_MAX + 1,
};

typedef enum AttributesIndex_E AttributesIndex_T;
/**
 *  The possible token types
 */

enum TokensType_E
{
    TOKEN_TYPE_UNKNOWN,
    TOKEN_EOF,
    TOKEN_ATT_NAME,
    TOKEN_EQUAL,
    TOKEN_VALUE,
};

typedef enum TokensType_E TokensType_T;
/**
 *  The states on the configuration file parser states machine
 */
enum States_E
{
    STAT_EXP_ATT_NONE,
    STAT_EXP_ATT_NAME,
    STAT_EXP_ATT_EQUAL,
    STAT_EXP_ATT_VALUE,
};

typedef enum States_E States_T;

/**
 * To represent possible conditional values of the token in configuration file
 */
enum CfgParser_ConditionalValues_E
{
    CGF_CONDITIONAL_VALUE_NOT_DEFINED = 0,
    CGF_CONDITIONAL_VALUE_YES,
    CGF_CONDITIONAL_VALUE_NO,
    CGF_CONDITIONAL_VALUE_OUT_OF_CHOICE,
};

typedef enum CfgParser_ConditionalValues_E CfgParser_ConditionalValues_T;

enum Retcode_CfgParser_E
{
    RETCODE_CFG_PARSER_SD_CARD_MOUNT_ERROR = RETCODE_FIRST_CUSTOM_CODE,
    RETCODE_CFG_PARSER_SD_CARD_FILE_NOT_EXIST,
    RETCODE_CFG_PARSER_SD_CARD_CONFIG_DATA_WRONG,
    RETCODE_CFG_PARSER_SD_CARD_FILE_CLOSE_ERROR,
};

/* local function prototype declarations */

/* public function prototype declarations */


/**
 * @brief initialize configuration parser.
 */
void CfgParser_Initialize(void);

/**
 * @brief   Print list of configured values.
 * @param[in] Title title for list
 * @param[in] defaultsOnly list only default values
 */
void CfgParser_List(const char* Title, uint8_t defaultsOnly);

/**
 * @brief   Function parses the Configuration file if present Typedef to represent the Conditional Value of the token
 * @return RETCODE_OK - if the parsing is completed successfully
 *
 * @note: Precondition before calling this function user has to ensure that SD card is inserted
 * and config.txt file is available
 */

Retcode_T CfgParser_ParseConfigFile(void);

/**
 * @brief returns the attribute value for the token SSID as defined at the configuration file
 *
 * If attribute is not defined in configuration file it returns empty string
 */
const char * CfgParser_GetWlanSSID(void);

/**
 * @brief returns attribute value for the token PASSWORD as defined at the configuration file
 *
 *   If attribute is not defined in configuration file it returns empty string
 */
const char *CfgParser_GetWlanPassword(void);

/**
 * @brief returns attribute value for the token STATICIP as defined at the configuration file
 *
 *   If attribute is not defined in configuration file it returns empty string
 */
const char *CfgParser_GetStaticIpAddress(void);

/**
 * @brief returns attribute value for the token DNSSERVER as defined at the configuration file
 *
 *   If attribute is not defined in configuration file it returns empty string
 */
const char *CfgParser_GetDnsServerAddress(void);

/**
 * @brief returns attribute value for the token MASK as defined at the configuration file
 *
 *   If attribute is not defined in configuration file it returns empty string
 */
const char *CfgParser_GetSubnetMask(void);

/**
 * @brief returns attribute value for the token GATEWAY as defined at the configuration file
 *
 *   If attribute is not defined in configuration file it returns empty string
 */
const char *CfgParser_GetGatewayAddress(void);

/**
 * @brief returns attribute value for the token LWM2MDEFSRV as defined at the configuration file
 *
 *   If attribute is not defined in configuration file it returns empty string
 */
const char *CfgParser_GetLwm2mServerAddress(void);

/**
 * @brief returns attribute value for the token LWM2MENDPOINT as defined at the configuration file
 *
 *   If attribute is not defined in configuration file it returns empty string
 */
const char *CfgParser_GetLwm2mEndpointName(void);

/**
 * @brief returns the DTLS/PSK identity of the configuration file
 */
const char *CfgParser_GetDtlsPskIdentity(void);
/**
 * @brief returns the DTLS/PSK secret key of the configuration file
 */
const char *CfgParser_GetDtlsPskSecretKey(void);
/**
 * @brief returns the DTLS/PSK secret key hex of the configuration file
 */
const char *CfgParser_GetDtlsPskSecretKeyHex(void);

/**
 * @brief Returns the current configuration for the field TESTMODE set in the SD card config.txt file
 * @retval CFG_TESTMODE_OFF - If attribute token 'TESTMODE = NO',is defined in config file of SD card
 * @retval CFG_TESTMODE_ON  - If attribute token 'TESTMODE = YES', is defined in config file of SD card
 * @retval CFG_TESTMODE_MIX - If attribute token 'TESTMODE = MIX', is defined in config file of SD card
 * @note  If no entry for TESTMODE configuration in SD card, then the default return value is CFG_TESTMODE_MIX
 */
uint8_t CfgParser_TestMode(void);

/**
 * @brief returns lwm2m lifetime in seconds.
 */
uint32_t CfgParser_GetLwm2mLifetime(void);

/**
 * @brief returns the LWM2M device binding. Default "U".
 */
const char *CfgParser_GetLwm2mDeviceBinding(void);

/**
 * @brief Returns the current configuration for the field LWM2MCONNOTIFIES set in the SD card config.txt file
 * @retval CFG_FALSE - If  attribute token 'LWM2MCONNOTIFIES = NO', is defined in config file of SD card
 * @retval CFG_TRUE -  If attribute token  'LWM2MCONNOTIFIES = YES', is defined in config file of SD card
 * @note  If no entry for LWM2MCONNOTIFIES configuration in SD card, then the default return value is CFG_FALSE
 */
uint8_t CfgParser_UseLwm2mConNotifies(void);

/**
 * @brief This functions is called to know status of the attribute value for the token STATIC
 *
 * @retval CfgParser_ConditionalValues_T
 *
 * CGF_CONDITIONAL_VALUE_YES - If attribute value for the token STATIC is YES
 * CGF_CONDITIONAL_VALUE_NO - If attribute value for the token STATIC is NO
 * CGF_CONDITIONAL_VALUE_OUT_OF_CHOICE - If attribute value for the token STATIC is neither YES or NO but something else
 */
CfgParser_ConditionalValues_T CfgParser_IsStaticModeActivated(void);

/**
 * @brief returns attribute value for the token STATICIP as defined at the configuration file
 *
 *   If attribute is not defined in configuration file it returns empty string
 */
const char *CfgParser_GetStaticIpAddress(void);

/**
 * @brief returns attribute value for the token DNSSERVER as defined at the configuration file
 *
 *   If attribute is not defined in configuration file it returns empty string
 */
const char *CfgParser_GetDnsServerAddress(void);

/**
 * @brief returns attribute value for the token MASK as defined at the configuration file
 *
 *   If attribute is not defined in configuration file it returns empty string
 */
const char *CfgParser_GetSubnetMask(void);

/**
 * @brief returns attribute value for the token GATEWAY as defined at the configuration file
 *
 *   If attribute is not defined in configuration file it returns empty string
 */
const char *CfgParser_GetGatewayAddress(void);

/* inline function definitions */

#endif /* CFGPARSER_H_ */

/** ************************************************************************* */
