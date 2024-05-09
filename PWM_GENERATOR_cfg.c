/**********************************************************************************************************************
 * \file TomAtom_cfg.c
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
#include <PWM_GENERATOR_cfg.h>
/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/
IfxGtm_Tom_Timer_Config tomConfig;
IfxGtm_Tom_Timer_Config tomPwmConfig;
IfxGtm_Atom_Timer_Config atomConfig;
/*********************************************************************************************************************/
/*--------------------------------------------Private Variables/Constants--------------------------------------------*/
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
/*********************************************************************************************************************/
void setTomConfig(float freq,uint16 priority,uint16 tom,uint16 channel, uint16 clock,Ifx_GTM *GTM)
{
    tomConfig.base.frequency       = freq;
    tomConfig.base.isrPriority     = priority;                        /* Set interrupt priority           */
    tomConfig.base.isrProvider     = IfxSrc_Tos_cpu0;                         /* Set interrupt provider           */
    tomConfig.base.minResolution              = 0;
    tomConfig.base.trigger.outputMode         = IfxPort_OutputMode_pushPull;
    tomConfig.base.trigger.outputDriver       = IfxPort_PadDriver_cmosAutomotiveSpeed1;
    tomConfig.base.trigger.risingEdgeAtPeriod = FALSE;
    tomConfig.base.trigger.outputEnabled      = FALSE;
    tomConfig.base.trigger.enabled            = FALSE;
    tomConfig.base.trigger.triggerPoint       = 0;
    tomConfig.base.trigger.isrPriority        = 0;
    tomConfig.base.trigger.isrProvider        = IfxSrc_Tos_cpu0;
    tomConfig.base.countDir                   = IfxStdIf_Timer_CountDir_up;
    tomConfig.base.startOffset                = 0.0;
    tomConfig.gtm            = GTM;
    tomConfig.tom            = tom;                            /* Define the timer used            */
    tomConfig.timerChannel   = channel;                         /* Define the channel used          */
    tomConfig.triggerOut     = NULL_PTR;
    tomConfig.clock          = clock;
    tomConfig.base.countDir  = IfxStdIf_Timer_CountDir_up;
    tomConfig.irqModeTimer   = IfxGtm_IrqMode_level;
    tomConfig.irqModeTrigger = IfxGtm_IrqMode_level;
    tomConfig.initPins       = TRUE;
}

void setAtomConfig(float32 frequency, uint16 priority,uint16 atom,uint16 channel, uint16 clock,Ifx_GTM *GTM)
{
    atomConfig.base.frequency                  = frequency;
    atomConfig.base.isrPriority                = priority;
    atomConfig.base.isrProvider                = IfxSrc_Tos_cpu0;
    atomConfig.base.minResolution              = 0;
    atomConfig.base.trigger.outputMode         = IfxPort_OutputMode_pushPull;
    atomConfig.base.trigger.outputDriver       = IfxPort_PadDriver_cmosAutomotiveSpeed1;
    atomConfig.base.trigger.risingEdgeAtPeriod = FALSE;
    atomConfig.base.trigger.outputEnabled      = FALSE;
    atomConfig.base.trigger.enabled            = FALSE;
    atomConfig.base.trigger.triggerPoint       = 0;
    atomConfig.base.trigger.isrPriority        = 0;
    atomConfig.base.trigger.isrProvider        = IfxSrc_Tos_cpu0;
    atomConfig.base.countDir                   = IfxStdIf_Timer_CountDir_up;
    atomConfig.base.startOffset                        = 0.0;
    atomConfig.gtm            = GTM;
    atomConfig.atom           = atom;
    atomConfig.timerChannel   = channel;
    atomConfig.triggerOut     = NULL_PTR;
    atomConfig.clock          = clock;
    atomConfig.base.countDir  = IfxStdIf_Timer_CountDir_up;
    atomConfig.irqModeTimer   = IfxGtm_IrqMode_level;
    atomConfig.irqModeTrigger = IfxGtm_IrqMode_level;
    atomConfig.initPins       = TRUE;
}

void setTomPwm(float freq,uint16 clock,IfxGtm_Tom_ToutMap* pin,Ifx_GTM *GTM)
{
    tomPwmConfig.base.frequency       = freq;
    tomPwmConfig.base.isrPriority     = 0;                        /* Set interrupt priority           */
    tomPwmConfig.base.isrProvider     = IfxSrc_Tos_cpu0;                         /* Set interrupt provider           */
    tomPwmConfig.base.minResolution              = 0;
    tomPwmConfig.base.trigger.outputMode         = IfxPort_OutputMode_pushPull;
    tomPwmConfig.base.trigger.outputDriver       = IfxPort_PadDriver_cmosAutomotiveSpeed1;
    tomPwmConfig.base.trigger.risingEdgeAtPeriod = Ifx_ActiveState_high;
    tomPwmConfig.base.trigger.outputEnabled      = TRUE;
    tomPwmConfig.base.trigger.enabled            = TRUE;
    tomPwmConfig.base.trigger.triggerPoint       = 0;
    tomPwmConfig.base.trigger.isrPriority        = 0;
    tomPwmConfig.base.trigger.isrProvider        = IfxSrc_Tos_cpu0;
    tomPwmConfig.base.startOffset                = 0.0;
    tomPwmConfig.gtm            = GTM;
    tomPwmConfig.tom            = pin->tom;                            /* Define the timer used            */
    tomPwmConfig.timerChannel   = pin->channel;                         /* Define the channel used          */
    tomPwmConfig.triggerOut     = pin;
    tomPwmConfig.clock          = clock;
    tomPwmConfig.base.countDir  = IfxStdIf_Timer_CountDir_up;
    tomPwmConfig.irqModeTimer   = IfxGtm_IrqMode_level;
    tomPwmConfig.irqModeTrigger = IfxGtm_IrqMode_pulseNotify;
    tomPwmConfig.initPins       = TRUE;
}

void setTomDriver(IfxGtm_Tom_Timer *mytomtimer)
{
    IfxGtm_Tom_Timer_init(mytomtimer, &tomConfig);                        /* Initialize the TOM               */
}

void setAtomDriver(IfxGtm_Atom_Timer *myatomtimer)
{
    IfxGtm_Atom_Timer_init(myatomtimer, &atomConfig);                        /* Initialize the ATOM               */
}

void setTomPwmDriver(IfxGtm_Tom_Timer *mytomtimer)
{
    IfxGtm_Tom_Timer_init(mytomtimer,&tomPwmConfig);
    Ifx_TimerValue triggerPoint = IfxGtm_Tom_Timer_getPeriod(mytomtimer);
    IfxGtm_Tom_Timer_disableUpdate(mytomtimer);
    IfxGtm_Tom_Timer_setTrigger(mytomtimer, triggerPoint/2);
    IfxGtm_Tom_Timer_applyUpdate(mytomtimer);
}
