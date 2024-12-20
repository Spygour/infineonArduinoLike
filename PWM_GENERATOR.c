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
#include "PWM_GENERATOR.h"
/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/

#define CLK_FREQ           20000.0f
/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/



/*********************************************************************************************************************/
/*--------------------------------------------Private Variables/Constants--------------------------------------------*/
/*********************************************************************************************************************/
IfxGtm_Tom_Pwm_Config tomPwmConfig;
IfxGtm_Atom_Pwm_Config atomPwmConfig;
boolean GtmModuleInit = FALSE;
/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
/*********************************************************************************************************************/
/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
/*********************************************************************************************************************/
/* Initialization functions */
static inline void initTomPwmConfig(uint16 period,uint16 dutyCycle,uint16 clock,IfxGtm_Tom_ToutMap* pin,Ifx_GTM *gtm)
{
    tomPwmConfig.gtm                      = gtm;
    tomPwmConfig.tom                      = pin->tom;
    tomPwmConfig.tomChannel               = pin->channel;
    tomPwmConfig.clock                    = clock;
    tomPwmConfig.period                   = period;
    tomPwmConfig.dutyCycle                = dutyCycle;
    tomPwmConfig.signalLevel              = Ifx_ActiveState_high;
    tomPwmConfig.oneShotModeEnabled       = FALSE;
    tomPwmConfig.synchronousUpdateEnabled = TRUE;
    tomPwmConfig.immediateStartEnabled    = FALSE;
    tomPwmConfig.interrupt.ccu0Enabled    = FALSE;
    tomPwmConfig.interrupt.ccu1Enabled    = FALSE;
    tomPwmConfig.interrupt.mode           = IfxGtm_IrqMode_pulseNotify;
    tomPwmConfig.interrupt.isrProvider    = IfxSrc_Tos_cpu0;
    tomPwmConfig.interrupt.isrPriority    = 0;
    tomPwmConfig.pin.outputPin            = pin;
    tomPwmConfig.pin.outputMode           = IfxPort_OutputMode_pushPull;
    tomPwmConfig.pin.padDriver            = IfxPort_PadDriver_cmosAutomotiveSpeed1;
}



static inline void initAtomPwmConfig(uint16 period,uint16 dutyCycle,IfxGtm_Atom_ToutMap* pin,Ifx_GTM *gtm)
{
    atomPwmConfig.gtm                      = gtm;
    atomPwmConfig.atom                     = pin->atom;
    atomPwmConfig.atomChannel              = pin->channel;
    atomPwmConfig.period                   = period;
    atomPwmConfig.dutyCycle                = dutyCycle;
    atomPwmConfig.signalLevel              = Ifx_ActiveState_high;
    atomPwmConfig.mode                     = IfxGtm_Atom_Mode_outputPwm;
    atomPwmConfig.oneShotModeEnabled       = FALSE;
    atomPwmConfig.synchronousUpdateEnabled = TRUE;
    atomPwmConfig.immediateStartEnabled    = FALSE;
    atomPwmConfig.interrupt.ccu0Enabled    = FALSE;
    atomPwmConfig.interrupt.ccu1Enabled    = FALSE;
    atomPwmConfig.interrupt.mode           = IfxGtm_IrqMode_pulseNotify;
    atomPwmConfig.interrupt.isrProvider    = IfxSrc_Tos_cpu0;
    atomPwmConfig.interrupt.isrPriority    = 0;
    atomPwmConfig.pin.outputPin            = pin;
    atomPwmConfig.pin.outputMode           = IfxPort_OutputMode_pushPull;
    atomPwmConfig.pin.padDriver            = IfxPort_PadDriver_cmosAutomotiveSpeed1;
}

static inline void initTomPwmDriver(IfxGtm_Tom_Pwm_Driver *Tomdriver)
{
    IfxGtm_Tom_Pwm_init(Tomdriver, &tomPwmConfig);                 /* Initialize the PWM                       */
    IfxGtm_Tom_Pwm_start(Tomdriver, TRUE);                         /* Start the PWM                            */
}


static inline void initAtomPwmDriver(IfxGtm_Atom_Pwm_Driver *Atomdriver)
{
    IfxGtm_Atom_Pwm_init(Atomdriver, &atomPwmConfig);                 /* Initialize the PWM                       */
    IfxGtm_Atom_Pwm_start(Atomdriver, TRUE);
}



/* This function initializes the TOM */
void TomPwm_Init(IfxGtm_Tom_Pwm_Driver* mytomdriver, uint16 period, uint16 dutyCycle, uint16 clock, IfxGtm_Tom_ToutMap* pin)
{
  if (GtmModuleInit == FALSE )
  {
    IfxGtm_enable(&MODULE_GTM);                                     /* Enable GTM                                   */

    GtmModuleInit = TRUE;
  }
  IfxGtm_Cmu_enableClocks(&MODULE_GTM, IFXGTM_CMU_CLKEN_FXCLK);   /* Enable the FXU clock                         */
  /* Initialize the configuration structure with default parameters */
  initTomPwmConfig(period,dutyCycle,clock,pin,&MODULE_GTM);
  initTomPwmDriver(mytomdriver);            /* Initialize the PWM                       */
}


void AtomPwm_Init(IfxGtm_Atom_Pwm_Driver* myatomdriver,uint16 period,uint16 dutyCycle,IfxGtm_Atom_ToutMap* pin)
{

  if (GtmModuleInit == FALSE)
  {
    GtmModuleInit = TRUE;
  }
  IfxGtm_Cmu_setClkFrequency(&MODULE_GTM, IfxGtm_Cmu_Clk_5, CLK_FREQ);        /* Set the CMU clock 0 frequency    */
  IfxGtm_Cmu_enableClocks(&MODULE_GTM, IFXGTM_CMU_CLKEN_CLK0);                /* Enable the CMU clock 0           */
  initAtomPwmConfig(period,dutyCycle,pin,&MODULE_GTM);

  initAtomPwmDriver(myatomdriver);            /* Initialize the PWM                       */

}


/* This function sets the duty cycle of the PWM */
void TomPwm_SetDutyCycle(IfxGtm_Tom_Pwm_Driver* mytomdriver,uint16 dutyCycle)
{
    IfxGtm_Tom_Tgc_enableChannelUpdate(mytomdriver->tgc[0],mytomdriver->tomChannel,0);
    IfxGtm_Tom_Ch_setCompareOneShadow(mytomdriver->tom, mytomdriver->tomChannel, dutyCycle);
    IfxGtm_Tom_Tgc_enableChannelUpdate(mytomdriver->tgc[0],mytomdriver->tomChannel,1);
}
void TomPwm_SetPeriod(IfxGtm_Tom_Pwm_Driver* mytomdriver,uint16 period)
{
    IfxGtm_Tom_Tgc_enableChannelUpdate(mytomdriver->tgc[0],mytomdriver->tomChannel,0);
    IfxGtm_Tom_Ch_setCompareZeroShadow(mytomdriver->tom, mytomdriver->tomChannel, period);
    IfxGtm_Tom_Tgc_enableChannelUpdate(mytomdriver->tgc[0],mytomdriver->tomChannel,1);
}

void TomPwm_SetClock(IfxGtm_Tom_Pwm_Driver* mytomdriver,uint16 clock)
{
    IfxGtm_Tom_Tgc_enableChannelUpdate(mytomdriver->tgc[0],mytomdriver->tomChannel,0);
    IfxGtm_Tom_Ch_setClockSource(mytomdriver->tom, mytomdriver->tomChannel, clock);
    IfxGtm_Tom_Tgc_enableChannelUpdate(mytomdriver->tgc[0],mytomdriver->tomChannel,1);
}

void AtomPwm_SetDutyCycle(IfxGtm_Atom_Pwm_Driver* myatomdriver,uint16 dutyCycle)
{
    IfxGtm_Atom_Agc_enableChannelUpdate(myatomdriver->agc,myatomdriver->atomChannel,0);
    IfxGtm_Atom_Ch_setCompareOneShadow(myatomdriver->atom, myatomdriver->atomChannel, dutyCycle);
    IfxGtm_Atom_Agc_enableChannelUpdate(myatomdriver->agc,myatomdriver->atomChannel,1);
}
void AtomPwm_SetPeriod(IfxGtm_Atom_Pwm_Driver* myatomdriver,uint16 period)
{
    IfxGtm_Atom_Agc_enableChannelUpdate(myatomdriver->agc,myatomdriver->atomChannel,0);
    IfxGtm_Atom_Ch_setCompareZeroShadow(myatomdriver->atom, myatomdriver->atomChannel, period);
    IfxGtm_Atom_Agc_enableChannelUpdate(myatomdriver->agc,myatomdriver->atomChannel,1);
}

void AtomPwm_SetClock(IfxGtm_Atom_Pwm_Driver* myatomdriver,uint16 clock)
{
    IfxGtm_Atom_Agc_enableChannelUpdate(myatomdriver->agc,myatomdriver->atomChannel,0);
    IfxGtm_Atom_Ch_setClockSource(myatomdriver->atom, myatomdriver->atomChannel, clock);
    IfxGtm_Atom_Agc_enableChannelUpdate(myatomdriver->agc,myatomdriver->atomChannel,1);
}
