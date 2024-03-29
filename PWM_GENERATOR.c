/**********************************************************************************************************************
 * \file PWM_AND_digitalReadWrite.c
 * \copyright Copyright (C) Infineon Technologies AG 2019
 * 
 * Use of this file is subject to the terms of use agreed between (i) you or the company in which ordinary course of 
 * business you are acting and (ii) Infineon Technologies AG or its licensees. If and as long as no such terms of use
 * are agreed, use of this file is subject to following:
 * 
 * Boost Software License - Version 1.0 - August 17th, 2003
 * 
 * Permission is hereby granted, free of charge, to any person or organization obtaining a copy of the software and 
 * accompanying documentation covered by this license (the "Software") to use, reproduce, display, distribute, execute,
 * and transmit the Software, and to prepare derivative works of the Software, and to permit third-parties to whom the
 * Software is furnished to do so, all subject to the following:
 * 
 * The copyright notices in the Software and this entire statement, including the above license grant, this restriction
 * and the following disclaimer, must be included in all copies of the Software, in whole or in part, and all 
 * derivative works of the Software, unless such copies or derivative works are solely in the form of 
 * machine-executable object code generated by a source language processor.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE 
 * COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN 
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
 * IN THE SOFTWARE.
 *********************************************************************************************************************/


/*********************************************************************************************************************/
/*-----------------------------------------------------Includes------------------------------------------------------*/
/*********************************************************************************************************************/
#include <PWM_GENERATOR.h>
#include "Ifx_Types.h"
#include "IfxGtm_Tom_Pwm.h"
#include "IfxGtm_Atom_Pwm.h"
/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/
#define ISR_PRIORITY_TOM    20
#define ISR_PRIORITY_ATOM  19    /* Interrupt priority number                    */
#define CLK_FREQ           500000.0f
/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/

uint16 g_fadeValue = 0;   /* Fade value, starting from 0                  */
uint16 g_fadeValue2 = 0;
sint8 g_fadeDir = 1;
sint8 g_fadeDir2 = 1; /* Fade direction variable                      */

/*********************************************************************************************************************/
/*--------------------------------------------Private Variables/Constants--------------------------------------------*/
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
/*********************************************************************************************************************/

void setDutyCycle_tom(IfxGtm_Tom_Pwm_Config* mytomconfig,IfxGtm_Tom_Pwm_Driver* mytomdriver,uint16 dutyCycle);
void setDutyCycle_atom(IfxGtm_Atom_Pwm_Config* myatomconfig,IfxGtm_Atom_Pwm_Driver* myatomdriver,uint16 dutyCycle);
/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
/*********************************************************************************************************************/





/* This function initializes the TOM */
void initTomPwm(IfxGtm_Tom_Pwm_Config* mytomconfig,IfxGtm_Tom_Pwm_Driver* mytomdriver,uint16 period,uint16 clock,IfxGtm_Tom_ToutMap pin)
{
    IfxGtm_enable(&MODULE_GTM);                                     /* Enable GTM                                   */

    IfxGtm_Cmu_enableClocks(&MODULE_GTM, IFXGTM_CMU_CLKEN_FXCLK);   /* Enable the FXU clock                         */

    /* Initialize the configuration structure with default parameters */
    IfxGtm_Tom_Pwm_initConfig(mytomconfig, &MODULE_GTM);

    mytomconfig->tom = pin.tom;                                      /* Select the TOM depending on the LED          */
    mytomconfig->tomChannel = pin.channel; /* Select the channel depending on the LED      */
    mytomconfig->clock = clock;
    mytomconfig->period = period;                                /* Set the timer period                         */
    mytomconfig->pin.outputPin = &pin;                               /* Set the LED port pin as output               */
    mytomconfig->synchronousUpdateEnabled = TRUE;                    /* Enable synchronous update                    */

    IfxGtm_Tom_Pwm_init(mytomdriver, mytomconfig);                /* Initialize the GTM TOM                       */
    IfxGtm_Tom_Pwm_start(mytomdriver, TRUE);                       /* Start the PWM                                */
}

void initAtomPwm(IfxGtm_Atom_Pwm_Config* myatomconfig,IfxGtm_Atom_Pwm_Driver* myatomdriver,uint16 period,IfxGtm_Atom_ToutMap pin)
{
    IfxGtm_enable(&MODULE_GTM); /* Enable GTM */

    IfxGtm_Cmu_setClkFrequency(&MODULE_GTM, IfxGtm_Cmu_Clk_0, CLK_FREQ);        /* Set the CMU clock 0 frequency    */
    IfxGtm_Cmu_enableClocks(&MODULE_GTM, IFXGTM_CMU_CLKEN_CLK0);                /* Enable the CMU clock 0           */

    IfxGtm_Atom_Pwm_initConfig(myatomconfig, &MODULE_GTM);                     /* Initialize default parameters    */

    myatomconfig->atom = pin.atom;    /* Select the ATOM depending on the LED     */
    myatomconfig->atomChannel = pin.channel;                             /* Select the channel depending on the LED  */
    myatomconfig->mode = 2;
    myatomconfig->period = period;                                   /* Set timer period                         */
    myatomconfig->pin.outputPin = &pin;                                  /* Set LED as output                        */
    myatomconfig->synchronousUpdateEnabled = TRUE;                       /* Enable synchronous update                */

    IfxGtm_Atom_Pwm_init(myatomdriver, myatomconfig);                 /* Initialize the PWM                       */
    IfxGtm_Atom_Pwm_start(myatomdriver, TRUE);                         /* Start the PWM                            */
}
void fadeLED(IfxGtm_Tom_Pwm_Config* mytomconfig,IfxGtm_Tom_Pwm_Driver* mytomdriver,uint16 period,uint16 step)
{
    if((g_fadeValue + step) >= period)
    {
        g_fadeDir = -1;                                             /* Set the direction of the fade                */
    }
    else if((g_fadeValue - step) <= 0)
    {
        g_fadeDir = 1;                                              /* Set the direction of the fade                */
    }
    g_fadeValue += g_fadeDir * step;                           /* Calculation of the new duty cycle            */
    setDutyCycle_tom(mytomconfig,mytomdriver,g_fadeValue);                            /* Set the duty cycle of the PWM                */
}

void fadeLED2(IfxGtm_Atom_Pwm_Config* myatomconfig,IfxGtm_Atom_Pwm_Driver* myatomdriver,uint16 period,uint16 step)
{
    if((g_fadeValue2 + step) >= period)
    {
        g_fadeDir2 = -1;                                             /* Set the direction of the fade                */
    }
    else if((g_fadeValue2 - step) <= 0)
    {
        g_fadeDir2 = 1;                                              /* Set the direction of the fade                */
    }
    g_fadeValue2 += g_fadeDir2 * step;                           /* Calculation of the new duty cycle            */
    setDutyCycle_atom(myatomconfig,myatomdriver,g_fadeValue2);                            /* Set the duty cycle of the PWM                */
}


/* This function sets the duty cycle of the PWM */
void setDutyCycle_tom(IfxGtm_Tom_Pwm_Config* mytomconfig,IfxGtm_Tom_Pwm_Driver* mytomdriver,uint16 dutyCycle)
{
    mytomconfig->dutyCycle = dutyCycle;                             /* Change the value of the duty cycle           */
    IfxGtm_Tom_Pwm_init(mytomdriver, mytomconfig);                /* Re-initialize the PWM                        */
}

void setDutyCycle_atom(IfxGtm_Atom_Pwm_Config* myatomconfig,IfxGtm_Atom_Pwm_Driver* myatomdriver,uint16 dutyCycle)
{
    myatomconfig->dutyCycle = dutyCycle;                             /* Change the value of the duty cycle           */
    IfxGtm_Atom_Pwm_init(myatomdriver, myatomconfig);                /* Re-initialize the PWM                        */
}


