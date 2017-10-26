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
 * @defgroup CFG_PARSER Config Parser
 * @{
 *
 * @brief This file provides the Implementation of parser of configuration file present on the micro SD card
 *
 * @details Configuration parser Processing source file
 * Module that parses the configuration file present on the micro SD card
 * 
 * Configuration file has following constraints:
 * 1) TOKENs should not have any space.
 * 2) Value should not have any trailing spaces.
 * 3) TOKENs are case sensitive.
 * 4) CFG_FILE_READ_BUFFER size is currently 2024 bytes.
 * 5) Default value of TOKENs are not considered from '.c' and '.h' files.
 * \code{.unparsed}
 *
 * Example for config.txt file:
 * ----------------------------
 * #give your router configuration ssid name
 * SSID=twist-wifi
 * #give your router configuration password
 * PASSWORD=twist-wifi
 * #If YES please ensure correct MASK,DNSSERVER&GATEWAY are valid
 * STATIC=YES/NO
 * #This field gives static IP address
 * STATICIP=192.168.1.5
 * #This field gives DNS address
 * DNSSERVER=192.168.1.1
 * #This field gives Gateway address
 * GATEWAY=192.168.1.1
 * #This field gives IP address MASK
 * MASK=255.255.255.0
 * #This field gives COAP address
 * LWM2MDEFSRV=coap://192.168.1.141:5685
 * #This field gives LwM2M End point address Name
 * #Token "#---MAC----#" is replaced by MAC of device. e.g.: "lwm2m:#---MAC----#"
 * #will be expanded to "lwm2m:001122334455" (if 00112334455 is the MAC)
 * LWM2MENDPOINT=XDK110_DEMO_LWM2M
 * #This field gives DTLS/PSK identity of the configuration file
 * #Token "#---MAC----#" is replaced by MAC of device. e.g.: "pskid:#---MAC----#"
 * #will be expanded to "pskid:001122334455" (if 00112334455 is the MAC)
 * #If DTLSPSKIDENTITY is empty, LWM2MENDPOINT will be used
 * DTLSPSKIDENTITY = XDK110_DEMO_LWM2M
 * #This field gives DTLS/PSK secret key of the configuration file (ex: Leshan has a key convert into Hex)
 * #Only used if DTLSPSKSECRETKEYHEX is empty (ex: Leshan has a key convert into Hex)
 * DTLSPSKSECRETKEY = Leshan
 * #This field gives DTLS/PSK hexadecimal secret key of the configuration file
 * DTLSPSKSECRETKEYHEX = 4C657368616E
 * # This Field used to Enable the Buttons & LEDS States
 * TESTMODE = YES/NO/MIX
 * # This Field gives the LWM2M lifetime in seconds
 * LWM2MLIFETIME = 240
 * \endcode
 *
 * @file
 */

/* module includes ********************************************************** */

//lint -esym(956,*) /* Suppressing Non const, non volatile static or external variable */
/* own header files */
#include "CfgParser.h"
#include "DefaultConfig.h"

/* system header files */
#include <stdio.h>
#include "BCDS_Basics.h"
#include "BCDS_Assert.h"

/* additional interface header files */

#include "ff.h"

/* constant definitions ***************************************************** */
#define CFG_EMPTY                       ""
#define CFG_YES                         "YES"
#define CFG_NO                          "NO"

#ifndef LWM2M_DTLS_PSK_SECRET_KEY
#define LWM2M_DTLS_PSK_SECRET_KEY        ""
#endif

#ifndef LWM2M_DTLS_PSK_SECRET_KEY_HEX
#define LWM2M_DTLS_PSK_SECRET_KEY_HEX        ""
#endif

#define CFG_DEFAULT_STATIC              CFG_NO        /**< Network static IP Address Default*/
#define CFG_DEFAULT_STATICIP            ""            /**< Network static IP Address Default*/
#define CFG_DEFAULT_DNSSERVER           ""            /**< Network DNS server Default*/
#define CFG_DEFAULT_GATEWAY             ""            /**< Network gateway Default*/
#define CFG_DEFAULT_MASK                ""            /**< Network mask Default*/
#define CFG_DEFAULT_LWM2M_BINDING       "U"           /**< LWM2M binding by Default*/
#define CFG_DEFAULT_LWM2M_NOTIFIES      CFG_NO        /**< LWM2M binding by Default*/
#define CFG_TEST_MODE_MIX               "MIX"         /**< BLE is not activated */

#define CFG_DEFAULT_DRIVE               ""              /**< SD Card default Drive location */
#define CFG_FORCE_MOUNT                 UINT8_C(1)      /**< Macro to define force mount */
#define CFG_CONFIG_FILENAME             "0:config.txt"  /**< Configuration file name */
#define CFG_SEEK_FIRST_LOCATION         UINT8_C(0)      /**< File seek to the first location */
#define CFG_MAX_LINE_SIZE               UINT8_C(65)
#define CFG_FILE_READ_BUFFER            UINT16_C(2024)

#define CFG_NUMBER_UINT8_ZERO           UINT8_C(0)    /**< Zero value */

#define CFG_WHITESPACE                  "\t\n\r "
#define CFG_SPACE                       "\t "

/* local variables ********************************************************** */

/** Attribute names on the configuration file */
static const char A1Name[] = "SSID";
static const char A2Name[] = "PASSWORD";
static const char A3Name[] = "STATIC";
static const char A4Name[] = "STATICIP";
static const char A5Name[] = "DNSSERVER";
static const char A6Name[] = "GATEWAY";
static const char A7Name[] = "MASK";
static const char A8Name[] = "LWM2MDEFSRV";
static const char A9Name[] = "LWM2MENDPOINT";
static const char A10Name[] = "DTLSPSKIDENTITY";
static const char A11Name[] = "DTLSPSKSECRETKEY";
static const char A12Name[] = "DTLSPSKSECRETKEYHEX";
static const char A13Name[] = "TESTMODE";
static const char A14Name[] = "LWM2MLIFETIME";
static const char A15Name[] = "LWM2MBINDING";
static const char A16Name[] = "LWM2MCONNOTIFIES";
static const char A17Name[] = "LWM2MDTLSSUFFIX";
static const char A18Name[] = "ACTIVATEDTLSPSK";

/** Variable containers for configuration values */
static char AttValues[ATT_IDX_SIZE][CFG_MAX_LINE_SIZE];

static uint8_t FileReadBuffer[CFG_FILE_READ_BUFFER];
static FATFS s_FatFileSystemObject; /** File system specific objects */

/*
 * Configuration holder structure array
 */
static ConfigLine_T ConfigStructure[ATT_IDX_SIZE] =
        {
                { A1Name, WLAN_CONNECT_WPA_SSID, CFG_FALSE, CFG_FALSE, AttValues[0] },
                { A2Name, WLAN_CONNECT_WPA_PASS, CFG_FALSE, CFG_FALSE, AttValues[1] },
                { A3Name, CFG_DEFAULT_STATIC, CFG_FALSE, CFG_FALSE, AttValues[2] },
                { A4Name, CFG_DEFAULT_STATICIP, CFG_FALSE, CFG_FALSE, AttValues[3] },
                { A5Name, CFG_DEFAULT_DNSSERVER, CFG_FALSE, CFG_FALSE, AttValues[4] },
                { A6Name, CFG_DEFAULT_GATEWAY, CFG_FALSE, CFG_FALSE, AttValues[5] },
                { A7Name, CFG_DEFAULT_MASK, CFG_FALSE, CFG_FALSE, AttValues[6] },
                { A8Name, LWM2M_SERVER_IP, CFG_FALSE, CFG_FALSE, AttValues[7] },
                { A9Name, LWM2M_ENDPOINT_NAME, CFG_FALSE, CFG_FALSE, AttValues[8] },
                { A10Name, LWM2M_DTLS_PSK_IDENTITY, CFG_FALSE, CFG_FALSE, AttValues[9] },
                { A11Name, LWM2M_DTLS_PSK_SECRET_KEY, CFG_FALSE, CFG_FALSE, AttValues[10] },
                { A12Name, LWM2M_DTLS_PSK_SECRET_KEY_HEX, CFG_FALSE, CFG_FALSE, AttValues[11] },
                { A13Name, CFG_TEST_MODE_MIX, CFG_FALSE, CFG_FALSE, AttValues[12] },
                { A14Name, LWM2M_DEFAULT_LIFETIME, CFG_FALSE, CFG_FALSE, AttValues[13] },
                { A15Name, CFG_DEFAULT_LWM2M_BINDING, CFG_FALSE, CFG_FALSE, AttValues[14] },
                { A16Name, CFG_DEFAULT_LWM2M_NOTIFIES, CFG_FALSE, CFG_FALSE, AttValues[15] },
                { A17Name, CFG_EMPTY, CFG_TRUE, CFG_FALSE, AttValues[16] }, /* ignore */
                { A18Name, CFG_EMPTY, CFG_TRUE, CFG_FALSE, AttValues[17] }, /* ignore */
        };


/* global variables ********************************************************* */

/* inline functions ********************************************************* */

/* local functions ********************************************************** */
static const char* getAttValue(int index)
{
    if (0 <= index && index < ATT_IDX_SIZE)
    {
        if (CFG_TRUE == ConfigStructure[index].defined)
        {
            return ConfigStructure[index].attValue;
        }
        else
        {
            return ConfigStructure[index].defaultValue;
        }
    }
    return "";
}

/**
 * @brief extracts tokens from the input buffer , copy into the token buffer and returns its
 * size at tokensize
 *
 * @param[in] buffer
 *            The input buffer
 * @param[in] idxAtBuffer
 *            The index from which we have to look after tokens
 * @param[in] bufSize
 *            The size of the input buffer (param one)
 * @param[out] token
 *            The buffer that will contain the token (must be different to NULL )
 * @param[in] expTokenType
 *            expected token type awaited from attribute parser
 *
 * @param[out] tokenSize
 *            The pointer to the variable that receive the size of the buffer
 *
 * @return the TOKEN types found (TOKEN_TYPE_UNKNOWN is EOF)
 */
static TokensType_T GetToken(const char *buffer, uint16_t *idxAtBuffer,
        uint16_t bufSize, char *token, uint16_t *tokenSize, States_T expTokenType)
{
    TokensType_T Result = TOKEN_TYPE_UNKNOWN;

    /* erase the OUTPUT token buffer*/
    memset(token, 0, *tokenSize);

    uint8_t index = UINT8_C(0);
    /* don't skip line end if next token is STAT_EXP_ATT_VALUE */
    /* support empty attribute values to deleted compiled defaults */
    const char *skipSet = (expTokenType == STAT_EXP_ATT_VALUE) ? CFG_SPACE : CFG_WHITESPACE;
    /* Bypass all chars in skip set */
    while ((*idxAtBuffer < bufSize) && (NULL != strchr(skipSet, buffer[*idxAtBuffer])))
    {
        *idxAtBuffer = *idxAtBuffer + 1;
    }

    if (expTokenType == STAT_EXP_ATT_NAME)
    {
        while (buffer[*idxAtBuffer] == '#') // Handling in-line comments for key value pair in the configuration file
        {
            while ((buffer[*idxAtBuffer] != '\n')

            && (buffer[*idxAtBuffer] != '\r')
                    && (*idxAtBuffer < bufSize)) // skip to EOL if since Start of comment tag '#' was found
            {
                *idxAtBuffer = *idxAtBuffer + 1;
            }
            while ((*idxAtBuffer < bufSize) && (NULL != strchr(CFG_WHITESPACE, buffer[*idxAtBuffer])))
            {
                *idxAtBuffer = *idxAtBuffer + 1;
            }
        }
    }

    /* is the next char the EQUAL sign */
    if (buffer[*idxAtBuffer] == '=')
    {
        /* YES */
        token[index] = '=';
        *idxAtBuffer = *idxAtBuffer + 1;
        index = index + 1;
        Result = TOKEN_EQUAL;
    }
    else
    {
        /* No , its a string ...*/
        if (expTokenType == STAT_EXP_ATT_VALUE)
        {
            while (
            (buffer[*idxAtBuffer] != '\r') &&
                    (buffer[*idxAtBuffer] != '\n') &&
                    (buffer[*idxAtBuffer] != '\t') &&
                    (*idxAtBuffer < bufSize)
            )
            {
                if (index < *tokenSize - 1)
                {
                    token[index] = buffer[*idxAtBuffer];
                    *idxAtBuffer = *idxAtBuffer + 1;
                    index = index + 1;
                }
            };
        }
        else
        {
            while (
            (buffer[*idxAtBuffer] != '\r') &&
                    (buffer[*idxAtBuffer] != '\n') &&
                    (buffer[*idxAtBuffer] != '\t') &&
                    (buffer[*idxAtBuffer] != ' ') &&
                    (buffer[*idxAtBuffer] != '=') &&
                    (*idxAtBuffer < bufSize)
            )
            {
                if (index < *tokenSize - 1)
                {
                    token[index] = buffer[*idxAtBuffer];
                    *idxAtBuffer = *idxAtBuffer + 1;
                    index = index + 1;
                }
            };
        }
        /* Is this string is a known string ,i.e. attribute name ?*/
        for (uint8_t ConfigIndex = UINT8_C(0); ConfigIndex < ATT_IDX_SIZE; ConfigIndex++)
        {
            if (0 == strcmp(token, ConfigStructure[ConfigIndex].attName))
            {
                Result = TOKEN_ATT_NAME;
                break;
            }
        }
        /* The string is not known one, so it's a value */
        if ((index > 0) && (TOKEN_TYPE_UNKNOWN == Result))
        {
            Result = TOKEN_VALUE;
        }
    }
    /* At this point nothing has been found ! */
    if ((index == 0) && (TOKEN_TYPE_UNKNOWN == Result)) /* (index == 0) check was added due to :Warning 438: Last value assigned to variable 'index' (defined at line 218) not used*/
    {
        Result = TOKEN_EOF;
    }
    return (Result);
}

/**
 * @brief Parse the config file configurations to the Buffer
 *
 * @param[in] buffer
 *            The buffer containing the configuration file
 *
 * @param[in] bufSize
 *            The size of the buffer (first param)
 * @return CFG_TRUE if configuration file is correct and contains necessary attribute/values
 *
 */
static uint8_t CfgParseConfigFileBuffer(const char *buffer, uint16_t bufSize)
{
    uint16_t IndexAtBuffer = UINT16_C(0);
    uint8_t Result = CFG_TRUE;
    States_T State = STAT_EXP_ATT_NAME;
    int8_t CurrentConfigToSet = INT8_C(0);
    char Token[CFG_MAX_LINE_SIZE] = { 0 };
    uint16_t OutputTokenSize = UINT16_C(0);
    while (CFG_TRUE == Result)
    {
        OutputTokenSize = CFG_MAX_LINE_SIZE;
        TokensType_T GetTokenType = GetToken(buffer, &IndexAtBuffer, bufSize, Token, &OutputTokenSize, State);
        if (GetTokenType == TOKEN_EOF)
            break;

        switch (State)
        {
        case STAT_EXP_ATT_NAME:
            {
            if (GetTokenType != TOKEN_ATT_NAME)
            {
                printf("Expecting attname at %u\r\n", (uint16_t) (IndexAtBuffer - strlen(Token)));
                Result = CFG_FALSE;
                break;
            }
            for (uint8_t i = UINT8_C(0); i < ATT_IDX_SIZE; i++)
            {
                if (strcmp(Token, ConfigStructure[i].attName) == UINT8_C(0))
                {
                    CurrentConfigToSet = i;
                    State = STAT_EXP_ATT_EQUAL;
                    break;
                }
            }

            break;
        }
        case STAT_EXP_ATT_EQUAL:
            {
            if (GetTokenType != TOKEN_EQUAL)
            {
                printf("Expecting sign '=' at %u\r\n", (uint16_t) (IndexAtBuffer - strlen(Token)));
                Result = CFG_FALSE;
                break;
            }
            State = STAT_EXP_ATT_VALUE;
            break;
        }
        case STAT_EXP_ATT_VALUE:
            {
            if (GetTokenType != TOKEN_VALUE)
            {
                printf("Expecting value string at %u\r\n", (uint16_t) (IndexAtBuffer - strlen(Token)));
                Result = CFG_FALSE;
                break;
            }

            strcpy(ConfigStructure[CurrentConfigToSet].attValue, Token);

            if (ConfigStructure[CurrentConfigToSet].defined == 0)
            {

                ConfigStructure[CurrentConfigToSet].defined = 1;
                State = STAT_EXP_ATT_NAME;

            }
            else
            {
                Result = CFG_FALSE;
                printf("Twice definition of  attribute %s! \r\n", ConfigStructure[CurrentConfigToSet].attName);
            }
            break;
        }
        default:
            printf("Unexpected state %d \r\n", State);
            break;
        }
    }

    /* special defaults */
    if (CFG_TRUE == ConfigStructure[ATT_IDX_DTLS_PSK_SECRET_KEY].defined)
    {
        /**
         * If the secret key is provided in the config.txt, disable the
         * compiled default for secret key hex to ensure, that the provided
         * key is used and not the default hexadecimal key.
         */
        ConfigStructure[ATT_IDX_DTLS_PSK_SECRET_KEY_HEX].defaultValue = CFG_EMPTY;
    }
    if (CFG_TRUE == ConfigStructure[ATT_IDX_LWM2M_ENDPOINT].defined)
    {
        /**
         * If the LWM2M endpoint name is provided in the config.txt, disable the
         * compiled default for DTLS psk identity to ensure, that the provided
         * LWM2M endpoint name is used and not the default DTLS psk identity.
         */
        ConfigStructure[ATT_IDX_DTLS_PSK_IDENTITY].defaultValue = CFG_EMPTY;
    }

    CfgParser_List("SD-Card config.txt:", CFG_FALSE);

    return Result;
}

void CfgParser_Initialize(void)
{
    /* Initialize the attribute values holders */
    for (uint8_t i = UINT8_C(0); i < ATT_IDX_SIZE; i++)
    {
        ConfigStructure[i].defined = CFG_FALSE;
        memset(ConfigStructure[i].attValue, CFG_NUMBER_UINT8_ZERO, CFG_MAX_LINE_SIZE);
        if (NULL == ConfigStructure[i].defaultValue)
        {
            ConfigStructure[i].defaultValue = CFG_EMPTY;
        }
    }
}

void CfgParser_List(const char* Title, uint8_t defaultsOnly)
{
    printf("%s\n\r", Title);
    for (uint8_t i = UINT8_C(0); i < ATT_IDX_SIZE; i++)
    {
        if (CFG_FALSE == ConfigStructure[i].ignore)
        {
            if (CFG_TRUE == ConfigStructure[i].defined)
            {
                printf("[%19s] = [%s]\r\n", ConfigStructure[i].attName, ConfigStructure[i].attValue);
            }
            else
            {
                if (CFG_TRUE == defaultsOnly)
                {
                    if (0 != *ConfigStructure[i].defaultValue)
                    {
                        printf("[%19s] * [%s]\r\n", ConfigStructure[i].attName, ConfigStructure[i].defaultValue);
                    }
                }
                else
                {
                    printf("[%19s] * [%s] (DEFAULT)\r\n", ConfigStructure[i].attName, ConfigStructure[i].defaultValue);
                }
            }
        }
        else if (CFG_TRUE == ConfigStructure[i].defined)
        {
            printf("[%19s] is deprecated and should be removed\r\n", ConfigStructure[i].attName);
        }
    }
}

/** @brief For description of the function please refer interface header CfgParser.h  */
Retcode_T CfgParser_ParseConfigFile(void)
{
    Retcode_T RetVal = (Retcode_T) RETCODE_FAILURE;

    FRESULT FileSystemResult;
    FIL FileReadHandle;
    UINT BytesRead;

    /* Initialize the attribute values holders */
    CfgParser_Initialize();

    /*Initialize file system */
    if (f_mount(&s_FatFileSystemObject, CFG_DEFAULT_DRIVE, CFG_FORCE_MOUNT) != FR_OK)
    {
        printf("f_mount failed !!\r\n");
        return (RETCODE(RETCODE_SEVERITY_ERROR, RETCODE_CFG_PARSER_SD_CARD_MOUNT_ERROR));
    }

    /* Open the file */
    FileSystemResult = f_open(&FileReadHandle, CFG_CONFIG_FILENAME, FA_OPEN_EXISTING | FA_READ);
    if (FileSystemResult != FR_OK)
    {
        printf("f_open failed !!\r\n");
        return (RETCODE(RETCODE_SEVERITY_ERROR, RETCODE_CFG_PARSER_SD_CARD_FILE_NOT_EXIST));
    }
    printf("f_open done \r\n");

    /*Step5 - Set the file read pointer to first location */
    FileSystemResult = f_lseek(&FileReadHandle, CFG_SEEK_FIRST_LOCATION);
    if (FileSystemResult != FR_OK)
    {
        /* Error. Cannot set the file write pointer */
        printf("f_lseek failed !!\r\n");
        return (RETCODE(RETCODE_SEVERITY_ERROR, (Retcode_T ) RETCODE_FAILURE));
    }
    printf("f_lseek done \r\n");

    /*Step6 - Read a buffer from file BUGGY memset !!!! */
    memset(FileReadBuffer, CFG_NUMBER_UINT8_ZERO, CFG_FILE_READ_BUFFER);

    /* DO NOT put the buffer here ... 1024 bytes bigger than the stack size of the task calling this CfgParser_ParseConfigFile!!!!*/

    BytesRead = 0;
    /* FREAD stucks !!!!!!! */

    FileSystemResult = f_read(&FileReadHandle, FileReadBuffer, CFG_FILE_READ_BUFFER, &BytesRead);
    printf("f_read done, bytes Read = %d \r\n", BytesRead);

    if (BytesRead >= CFG_FILE_READ_BUFFER)
    {
        printf("f_read done, bytes Read = %d  is greater than or equal to CFG_FILE_READ_BUFFER \r\n", BytesRead);
        assert(false);
    }
    if (FileSystemResult == FR_OK)
    {
        if (CFG_TRUE == CfgParseConfigFileBuffer((const char*) FileReadBuffer, BytesRead))
        {
            RetVal = RETCODE_OK;
        }
        else
        {
            RetVal = RETCODE(RETCODE_SEVERITY_ERROR, RETCODE_CFG_PARSER_SD_CARD_CONFIG_DATA_WRONG);
        }
        FileSystemResult = f_close(&FileReadHandle);
        if (FileSystemResult != FR_OK)
        {
            printf("Error closing configuration file !!\r\n");
            RetVal = RETCODE(RETCODE_SEVERITY_ERROR, RETCODE_FAILURE);
        }
    }
    else
    {
        printf("Error read configuration file !!\r\n");
        RetVal = RETCODE(RETCODE_SEVERITY_ERROR, RETCODE_FAILURE);
    }

    return RetVal;
}



/**
 * @brief returns the WLAN SSID defined at the configuration file
 *
 * @return WLAN SSID
 */
const char *CfgParser_GetWlanSSID(void)
{
    return getAttValue(ATT_IDX_SSID);
}
/**
 * @brief returns the PASSWORD defined by the attribute PASSWORD of the configuration file
 *
 * @return WLAN PASSWORD
 */
const char *CfgParser_GetWlanPassword(void)
{
    return getAttValue(ATT_IDX_PASSWORD);
}

/**
 * @brief For description please refer CfgParser.h header file
 */
const char *CfgParser_GetStaticIpAddress(void)
{
    return getAttValue(ATT_IDX_STATICIP);
}

/**
 * @brief For description please refer CfgParser.h header file
 */
const char *CfgParser_GetDnsServerAddress(void)
{
    return getAttValue(ATT_IDX_DNSSERVER);
}

/**
 * @brief For description please refer CfgParser.h header file
 */
const char *CfgParser_GetSubnetMask(void)
{
    return getAttValue(ATT_IDX_MASK);
}

/**
 * @brief For description please refer CfgParser.h header file
 */
const char *CfgParser_GetGatewayAddress(void)
{
    return getAttValue(ATT_IDX_GATEWAY);
}

/**
 * @brief For description please refer CfgParser.h header file
 */
const char *CfgParser_GetLwm2mServerAddress(void)
{
    return getAttValue(ATT_IDX_LWM2M_DEFAULT_SERVER);
}

/**
 * @brief For description please refer CfgParser.h header file
 */
const char *CfgParser_GetLwm2mEndpointName(void)
{
    return getAttValue(ATT_IDX_LWM2M_ENDPOINT);
}

/**
 * @brief For description please refer CfgParser.h header file
 */
CfgParser_ConditionalValues_T CfgParser_IsStaticModeActivated(void)
{
    CfgParser_ConditionalValues_T RetVal = CGF_CONDITIONAL_VALUE_OUT_OF_CHOICE;
    const char* Value = getAttValue(ATT_IDX_STATIC);
    if (0 == strcmp(CFG_YES, Value))
    {
        RetVal = CGF_CONDITIONAL_VALUE_YES;
    }
    else if (0 == strcmp(CFG_NO, Value))
    {
        RetVal = CGF_CONDITIONAL_VALUE_NO;
    }
    return (RetVal);
}

const char *CfgParser_GetDtlsPskIdentity(void)
{
    return getAttValue(ATT_IDX_DTLS_PSK_IDENTITY);
}

const char *CfgParser_GetDtlsPskSecretKey(void)
{
    return getAttValue(ATT_IDX_DTLS_PSK_SECRET_KEY);
}

const char *CfgParser_GetDtlsPskSecretKeyHex(void)
{
    if (CFG_FALSE == ConfigStructure[ATT_IDX_DTLS_PSK_SECRET_KEY_HEX].defined &&
        CFG_TRUE == ConfigStructure[ATT_IDX_DTLS_PSK_SECRET_KEY].defined)
    {
        return "";
    }

    return getAttValue(ATT_IDX_DTLS_PSK_SECRET_KEY_HEX);
}

/**
 * @brief For description please refer CfgParser.h header file
 */
uint8_t CfgParser_TestMode(void)
{
    uint8_t Result = CFG_TESTMODE_MIX;
    const char* Value = getAttValue(ATT_IDX_TEST_MODE);
    if (0 == strcmp(CFG_TEST_MODE_MIX, Value))
    {
        Result = CFG_TESTMODE_MIX;
    }
    else if (0 == strcmp(CFG_YES, Value))
    {
        Result = CFG_TESTMODE_ON;
    }
    else if (0 == strcmp(CFG_NO, Value))
    {
        Result = CFG_TESTMODE_OFF;
    }
    return Result;
}

/**
 * @brief For description please refer CfgParser.h header file
 */
uint32_t CfgParser_GetLwm2mLifetime(void)
{
    return (uint32_t) atol(getAttValue(ATT_IDX_LWM2M_LIFETIME));
}

const char *CfgParser_GetLwm2mDeviceBinding(void)
{
    return getAttValue(ATT_IDX_LWM2M_BINDING);
}

uint8_t CfgParser_UseLwm2mConNotifies(void)
{
    uint8_t Result = CFG_FALSE;
    const char* Value = getAttValue(ATT_IDX_LWM2M_CON_NOTIFIES);
    if (0 == strcmp(CFG_YES, Value))
    {
        Result = CFG_TRUE;
    }
    return Result;
}

/**@} */
/** ************************************************************************* */
