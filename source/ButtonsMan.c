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
 * @defgroup BUTTONS_MAN Buttons Man
 * @{
 *
 * @brief Buttons Processing source file
 *
 * @details This file provides the Implementation of ButtonsMan.
 *
 * @file
 **/

/* module includes ********************************************************** */
/* own header files */

#include "XDKAppInfo.h"

#undef BCDS_MODULE_ID  /* Module ID define before including Basics package*/
#define BCDS_MODULE_ID XDK_APP_MODULE_ID_SENSOR_LWM2M_BUTTON_MODULE


#include "ButtonsMan.h"
#include "SntpTime.h"
#include "Lwm2mInterface.h"
#include "Lwm2mObject_AlertNotification.h"
#include "Lwm2mClient.h"

/* system header files */
#include "BCDS_Basics.h"

/* additional interface header files */
#include "BSP_BoardType.h"
#include "BCDS_BSP_Button.h"


/* constant definitions ***************************************************** */

/* local variables ********************************************************** */

/* global variables ********************************************************* */

/*Application Command Processor Instance */

static bool UseButtonsForLed = false;
static volatile bool Button1State = false;
static volatile bool Button2State = false;

/* inline functions ********************************************************* */

/* local functions ********************************************************** */

void processButton1Data(void * param1, uint32_t buttonstatus)
{
    BCDS_UNUSED(param1);
    char buf[100]={0};
    switch (buttonstatus)
    {
    /*  Button 1 press/release */
    case BSP_XDK_BUTTON_PRESS:
        Button1State = true;
        if (UseButtonsForLed)
        {
            if (Button2State)
            {
                printf("Both Button's pressed - force registration!\n\r");
                Lwm2mInterfaceTriggerRegister();
                return;
            }
            /* use button 1 in test mode */
            printf("Button-1 pressed  - Red ON\n\r");
            Lwm2mInterfaceRedLedSetState(true);
            /* button 1 triggers notification */
            sprintf(buf, "Button Alert:%lu:Button-1 Push:MEDIUM:BTN1", (unsigned long) GetUtcTime());
            ObjectAlertNotification_SetValue(buf);
        }
        else
        {
            printf("Button-1 pressed\n\r");
        }
        break;
    case BSP_XDK_BUTTON_RELEASE:
        Button1State = false;
        if (UseButtonsForLed)
        {
            /* use button 1 in test mode */
            printf("Button-1 released - Red OFF\n\r");
            Lwm2mInterfaceRedLedSetState(false);
            /* button 1 triggers notification */
            sprintf(buf, "Button Alert:%lu:Button-1 Release:MEDIUM:BTN1", (unsigned long) GetUtcTime());
            ObjectAlertNotification_SetValue(buf);
        }
        else
        {
            printf("Button-1 released\n\r");
        }
        break;
    default:
        printf("FATAL Error:Unsolicited button event occurred for PB1 \n\r");
        break;
    }

}

/*Callback for Button 1 */
void Button1Callback(uint32_t data)
{
    Retcode_T returnValue = CmdProcessor_enqueueFromIsr(GetAppCmdProcessorHandle(), processButton1Data, NULL, data);
    if (RETCODE_OK != returnValue)
    {
        printf("Enqueuing for Button 1 callback failed\n\r");
    }
}

void processButton2Data(void * param1, uint32_t buttonstatus)
{
    BCDS_UNUSED(param1);
    switch (buttonstatus)
    {
    /*  Button 2 press/release */
    case BSP_XDK_BUTTON_PRESS:
        Button2State = true;
        if (UseButtonsForLed)
        {
            if (Button1State)
            {
                printf("Both Button's pressed - force registration!\n\r");
                Lwm2mInterfaceTriggerRegister();
                return;
            }
            /* use button 2 in test mode */
            printf("Button-2 pressed  - Orange OFF\n\r");
            Lwm2mInterfaceOrangeLedSetState(false);
            /* button 2 is currently not intended to trigger a notification */
        }
        else
        {
            printf("Button-2 pressed\n\r");
        }
        break;
    case BSP_XDK_BUTTON_RELEASE:
        Button2State = false;
        if (UseButtonsForLed)
        {
            /* use button 2 in test mode */
            printf("Button-2 released - Orange ON\n\r");
            Lwm2mInterfaceOrangeLedSetState(true);
            /* button 2 is currently not intended to trigger a notification */
        }
        else
        {
            printf("Button-2 released\n\r");
        }
        break;
    default:
        printf("FATAL Error:Unsolicited button event occurred for PB2 \n\r");
        break;
    }

}

/*Callback for Button 2 */
void Button2Callback(uint32_t data)
{
    Retcode_T returnValue = CmdProcessor_enqueueFromIsr(GetAppCmdProcessorHandle(), processButton2Data, NULL, data);
    if (RETCODE_OK != returnValue)
    {
        printf("Enqueuing for Button 2 callback failed\n\r");
    }
}

static Retcode_T ButtonInitialize(BSP_Button_Callback_T Button1CallbackRotuine, BSP_Button_Callback_T Button2CallbackRoutine)
{
    Retcode_T returnVal = RETCODE_OK;
    returnVal = BSP_Button_Connect();
    if (RETCODE_OK == returnVal)
    {
        returnVal = BSP_Button_Enable((uint32_t) BSP_XDK_BUTTON_1, Button1CallbackRotuine);
    }
    if (RETCODE_OK == returnVal)
    {
        returnVal = BSP_Button_Enable((uint32_t) BSP_XDK_BUTTON_2, Button2CallbackRoutine);
    }
    if (RETCODE_OK == returnVal)
    {
        printf("BUTTON Initialization success");
    }
    else
    {
        printf(" Error occurred in BUTTON Initialization routine %u \n", (unsigned int) returnVal);
    }

    return returnVal;
}

/* global functions ********************************************************* */
/**
 * @brief Initializes the button module
 * @param[in] testMode
 *            Read the configuration from SD card parser using  CfgParser_TestMode() and pass this input to
 *            configure the sensor client application in this mode.
 *            In either testmode is ON or MIX, button 1 is bound to the red LED and triggers a alert notification
 *            and button 2 is bound to the orange LED.
 * @retval FAILURE means Button initialization failed
 * @retval SUCCESS means Button initialization success
 */
Return_t InitButtonMan(bool testMode)
{
    Return_t returnValue = FAILURE;
    Retcode_T ButtonRetVal = (Retcode_T)RETCODE_FAILURE;

    UseButtonsForLed = testMode;
    ButtonRetVal = ButtonInitialize(Button1Callback, Button2Callback);
    if (RETCODE_OK == ButtonRetVal)
    {
        returnValue = SUCCESS;
    }
    else
    {
        returnValue = FAILURE;

    }
    return (returnValue);
}

/**@} */

/** ************************************************************************* */
