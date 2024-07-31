/**********************************************************************************************************************
 * \file TimerTom.c
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

#include "INTERRUPTS.h"

/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/                    /* Define the GPT12 Timer interrupt priority            */
#define ISR_PROVIDER_GPT12_TIMER    IfxSrc_Tos_cpu0/* Interrupt provider                                   */
#define CMU_FREQ                    1000000.0f
#define LED                         &MODULE_P00,6
/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
IfxGtm_Tom_Timer myTestExample;
/*********************************************************************************************************************/
/*--------------------------------------------Private Variables/Constants--------------------------------------------*/
IfxGtm_Tom_Timer_Config tomConfig;
IfxGtm_Tom_Timer_Config tomPwmTimerConfig;
IfxGtm_Atom_Timer_Config atomConfig;
/*********************************************************************************************************************/
/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
/*********************************************************************************************************************/
/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
/*********************************************************************************************************************/
static inline void setTomConfig(float freq,uint16 priority,uint16 tom,uint16 channel, uint16 clock,Ifx_GTM *GTM)
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

static inline void setAtomConfig(float32 frequency, uint16 priority,uint16 atom,uint16 channel, uint16 clock,Ifx_GTM *GTM)
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

static inline void setTomPwm(IfxGtm_Tom_ToutMap* pin,Ifx_GTM *GTM)
{
    tomPwmTimerConfig.base.frequency       = 100;
    tomPwmTimerConfig.base.isrPriority     = 0;                        /* Set interrupt priority           */
    tomPwmTimerConfig.base.isrProvider     = IfxSrc_Tos_cpu0;                         /* Set interrupt provider           */
    tomPwmTimerConfig.base.minResolution              = 0;
    tomPwmTimerConfig.base.trigger.outputMode         = IfxPort_OutputMode_pushPull;
    tomPwmTimerConfig.base.trigger.outputDriver       = IfxPort_PadDriver_cmosAutomotiveSpeed1;
    tomPwmTimerConfig.base.trigger.risingEdgeAtPeriod = Ifx_ActiveState_high;
    tomPwmTimerConfig.base.trigger.outputEnabled      = TRUE;
    tomPwmTimerConfig.base.trigger.enabled            = TRUE;
    tomPwmTimerConfig.base.trigger.triggerPoint       = 10;
    tomPwmTimerConfig.base.trigger.isrPriority        = 0;
    tomPwmTimerConfig.base.trigger.isrProvider        = IfxSrc_Tos_cpu0;
    tomPwmTimerConfig.base.startOffset                = 0.0;
    tomPwmTimerConfig.gtm            = GTM;
    tomPwmTimerConfig.tom            = pin->tom;                            /* Define the timer used            */
    tomPwmTimerConfig.timerChannel   = pin->channel;                         /* Define the channel used          */
    tomPwmTimerConfig.triggerOut     = pin;
    tomPwmTimerConfig.clock          = 2;
    tomPwmTimerConfig.base.countDir  = IfxStdIf_Timer_CountDir_up;
    tomPwmTimerConfig.irqModeTimer   = IfxGtm_IrqMode_level;
    tomPwmTimerConfig.irqModeTrigger = IfxGtm_IrqMode_pulseNotify;
    tomPwmTimerConfig.initPins       = TRUE;
}

static inline void setTomDriver(IfxGtm_Tom_Timer *mytomtimer)
{
    IfxGtm_Tom_Timer_init(mytomtimer, &tomConfig);                        /* Initialize the TOM               */
}

static inline void setAtomDriver(IfxGtm_Atom_Timer *myatomtimer)
{
    IfxGtm_Atom_Timer_init(myatomtimer, &atomConfig);                        /* Initialize the ATOM               */
}

static inline void setTomPwmDriver(IfxGtm_Tom_Timer *mytomtimer)
{
    IfxGtm_Tom_Timer_init(mytomtimer,&tomPwmTimerConfig);
    Ifx_TimerValue triggerPoint = IfxGtm_Tom_Timer_getPeriod(mytomtimer);
    IfxGtm_Tom_Timer_disableUpdate(mytomtimer);
    IfxGtm_Tom_Timer_setTrigger(mytomtimer, triggerPoint/2);
    IfxGtm_Tom_Timer_applyUpdate(mytomtimer);
}


/*Example regarding the interrupt using the tom timer*/
IFX_INTERRUPT(test_function,0,0);
void test_function(){
    IfxGtm_Tom_Timer_acknowledgeTimerIrq(&myTestExample); /*acknowledge the timer used. For the ATOM we use IfxGtm_Atom_Timer_acknowledgeTimerIrq(&myatomdriver)*/
    /*The code you should write*/
}



void initTomInterrupt(IfxGtm_Tom_Timer *mytomtimer,float freq,uint16 priority,uint16 tom,uint16 channel, uint16 clock)
{
    IfxGtm_enable(&MODULE_GTM); /* Enable GTM */

    setTomConfig(freq,priority,tom,channel, clock,&MODULE_GTM); /*Set the values to tom config*/

    IfxGtm_Cmu_enableClocks(&MODULE_GTM, IFXGTM_CMU_CLKEN_FXCLK);               /* Enable the CMU clock             */
    setTomDriver(mytomtimer); /*Use the const tom config of the cfg file to set the timer*/

    IfxGtm_Tom_Timer_run(mytomtimer); /* Start the TOM */
}


void TomPwmTimer_Init(IfxGtm_Tom_Timer *mytomtimer,uint32 Period,uint16 DutyCycle,IfxGtm_Tom_ToutMap* pin)
{

    IfxGtm_enable(&MODULE_GTM);
    /* Set the GTM global clock frequency in Hz */
    IfxGtm_Cmu_setGclkFrequency(&MODULE_GTM, IfxGtm_Cmu_getModuleFrequency(&MODULE_GTM));
    /* Set the GTM configurable clock frequency in Hz */
    IfxGtm_Cmu_setClkFrequency(&MODULE_GTM, IfxGtm_Cmu_Clk_0, IfxGtm_Cmu_getGclkFrequency(&MODULE_GTM));
    /* Enable the FXU clock                         */
    IfxGtm_Cmu_enableClocks(&MODULE_GTM, IFXGTM_CMU_CLKEN_FXCLK);
    setTomPwm(pin,&MODULE_GTM); /*Set pwm the values to tom config*/

    setTomPwmDriver(mytomtimer);                  /* Initialize the TOM               */
    IfxGtm_Tom_Timer_run(mytomtimer); /* Start the TOM */
    TomTimer_SetDutyAndPeriod(mytomtimer, DutyCycle, Period);

}

void initAtomInterrupt(IfxGtm_Atom_Timer* mytimer,float32 frequency, uint16 priority,uint16 atom,uint16 channel)
{
    IfxGtm_enable(&MODULE_GTM);                                             /* Enable GTM                       */

    setAtomConfig(frequency,priority,atom,channel, 0,&MODULE_GTM);            /*Set the values to the constant atom config on the cfg file*/


    IfxGtm_Cmu_setClkFrequency(&MODULE_GTM, 0, CMU_FREQ);    /* Set the clock frequency          */
    IfxGtm_Cmu_enableClocks(&MODULE_GTM, IFXGTM_CMU_CLKEN_CLK0);            /* Enable the CMU clock 0           */

    setAtomDriver(mytimer);                                           /*Change the Atom driver based on the atom config*/
    IfxGtm_Atom_Timer_run(mytimer);                                  /* Start the ATOM                   */
}


void initGpt12interrupt(uint16 reload,IfxGpt12_Gpt1BlockPrescaler gtp1prescaler,IfxGpt12_TimerInputPrescaler timerPrescaler,uint16 priority)
{
    /* Initialize the GPT12 module */
    IfxGpt12_enableModule(&MODULE_GPT120);                                          /* Enable the GPT12 module      */
    IfxGpt12_setGpt1BlockPrescaler(&MODULE_GPT120, gtp1prescaler); /* Set GPT1 block prescaler     */

    /* Initialize the Timer T3 */
    IfxGpt12_T3_setMode(&MODULE_GPT120, IfxGpt12_Mode_timer);                       /* Set T3 to timer mode         */
    IfxGpt12_T3_setTimerDirection(&MODULE_GPT120, IfxGpt12_TimerDirection_down);    /* Set T3 count direction       */
    IfxGpt12_T3_setTimerPrescaler(&MODULE_GPT120, timerPrescaler); /* Set T3 input prescaler       */
    IfxGpt12_T3_setTimerValue(&MODULE_GPT120, reload);                        /* Set T3 start value           */

    /* Initialize the Timer T2 */
    IfxGpt12_T2_setMode(&MODULE_GPT120, IfxGpt12_Mode_reload);                      /* Set T2 to reload mode        */
    IfxGpt12_T2_setReloadInputMode(&MODULE_GPT120, IfxGpt12_ReloadInputMode_bothEdgesTxOTL); /* Set reload trigger  */
    IfxGpt12_T2_setTimerValue(&MODULE_GPT120, reload);                        /* Set T2 reload value          */

    /* Initialize the interrupt */
    volatile Ifx_SRC_SRCR *src = IfxGpt12_T3_getSrc(&MODULE_GPT120);                /* Get the interrupt address    */
    IfxSrc_init(src, ISR_PROVIDER_GPT12_TIMER, priority);/* Initialize service request   */
    IfxSrc_enable(src);                                                             /* Enable GPT12 interrupt       */


    IfxGpt12_T3_run(&MODULE_GPT120, IfxGpt12_TimerRun_start);                       /* Start the timer              */
}

void TomTimer_SetDutyCycle(IfxGtm_Tom_Timer *mytomtimer, uint16 DutyCycle)
{
  uint32 ActlPeriod = IfxGtm_Tom_Ch_getCompareZero(mytomtimer->tom, mytomtimer->timerChannel);
  uint32 ActlDutyCycle = (ActlPeriod*DutyCycle)/100;

  IfxGtm_Tom_Timer_disableUpdate(mytomtimer);
  IfxGtm_Tom_Ch_setCompareOneShadow(mytomtimer->tom, mytomtimer->timerChannel, (uint16)ActlDutyCycle);
  IfxGtm_Tom_Timer_applyUpdate(mytomtimer);
}


void TomTimer_SetPeriod(IfxGtm_Tom_Timer *mytomtimer,uint32 Period)
{
  IfxGtm_Tom_Ch_ClkSrc clock = 0;
  uint32 DutyCycle = (IfxGtm_Tom_Ch_getCompareOne(mytomtimer->tom, mytomtimer->timerChannel)*100)/IfxGtm_Tom_Ch_getCompareZero(mytomtimer->tom, mytomtimer->timerChannel);
  uint32 Prescaler[5] = {1, 16, 256, 4096, 32768};
  uint32 ActlPeriod = Period*100;
  uint32 ActlDutyCycle;

  while(ActlPeriod > 0xFFFF)
  {
    ActlPeriod = ActlPeriod/Prescaler[clock];
    clock++;
  }
  ActlDutyCycle = (DutyCycle*ActlPeriod)/100;

  IfxGtm_Tom_Timer_disableUpdate(mytomtimer);
  IfxGtm_Tom_Ch_setCompareShadow(mytomtimer->tom, mytomtimer->timerChannel, (uint16)ActlPeriod, (uint16)ActlDutyCycle);
  IfxGtm_Tom_Ch_setClockSource(mytomtimer->tom, mytomtimer->timerChannel, clock);
  IfxGtm_Tom_Timer_applyUpdate(mytomtimer);
}


void TomTimer_SetDutyAndPeriod(IfxGtm_Tom_Timer *mytomtimer,uint16 DutyCycle,uint32 Period)
{
  IfxGtm_Tom_Ch_ClkSrc clock = 0;
  uint32 Prescaler[5] = {1, 16, 256, 4096, 32768};
  uint32 ActlPeriod = Period*100;
  uint32 ActlDutyCycle;

  while(ActlPeriod > 0xFFFF)
  {
    ActlPeriod = ActlPeriod/Prescaler[clock];
    clock++;
  }
  ActlDutyCycle = (DutyCycle*ActlPeriod)/100;

  IfxGtm_Tom_Timer_disableUpdate(mytomtimer);
  IfxGtm_Tom_Ch_setCompareShadow(mytomtimer->tom, mytomtimer->timerChannel, (uint16)ActlPeriod, (uint16)ActlDutyCycle);
  IfxGtm_Tom_Ch_setClockSource(mytomtimer->tom, mytomtimer->timerChannel, clock);
  IfxGtm_Tom_Timer_applyUpdate(mytomtimer);
}

void TomTimer_SetPeriodActl(IfxGtm_Tom_Timer *mytomtimer, uint16 Period, IfxGtm_Tom_Ch_ClkSrc Prescaler)
{
  IfxGtm_Tom_Timer_disableUpdate(mytomtimer);
  IfxGtm_Tom_Ch_setCompareZeroShadow(mytomtimer->tom, mytomtimer->timerChannel, Period);
  IfxGtm_Tom_Ch_setClockSource(mytomtimer->tom, mytomtimer->timerChannel, Prescaler);
  IfxGtm_Tom_Timer_applyUpdate(mytomtimer);
}

void TomTimer_SetDutyCycleActl(IfxGtm_Tom_Timer *mytomtimer,uint16 DutyCycle)
{
  IfxGtm_Tom_Timer_disableUpdate(mytomtimer);
  IfxGtm_Tom_Ch_setCompareOneShadow(mytomtimer->tom, mytomtimer->timerChannel, (uint16)DutyCycle);
  IfxGtm_Tom_Timer_applyUpdate(mytomtimer);
}

void AtomTimer_SetDutyCycle(IfxGtm_Atom_Timer *myatomtimer, uint16 DutyCycle)
{
  uint32 ActlPeriod = IfxGtm_Atom_Timer_getPeriod(myatomtimer);
  uint32 ActlDutyCycle = DutyCycle*ActlPeriod/100;

  IfxGtm_Atom_Timer_disableUpdate(myatomtimer);
  IfxGtm_Atom_Ch_setCompareOneShadow(myatomtimer->atom, myatomtimer->timerChannel, ActlDutyCycle);
  IfxGtm_Atom_Timer_applyUpdate(myatomtimer);
}

void AtomTimer_SetPeriod(IfxGtm_Atom_Timer *myatomtimer,uint32 Period)
{
  IfxGtm_Cmu_Clk clock = IfxGtm_Atom_Ch_getClockSource(myatomtimer->atom, myatomtimer->timerChannel);
  uint32 DutyCycle = (IfxGtm_Atom_Ch_getCompareOne(myatomtimer->atom, myatomtimer->timerChannel)*100/IfxGtm_Atom_Timer_getPeriod(myatomtimer));
  uint32 ActlPeriod = Period*100;
  uint32 ActlDutyCycle;
  ActlDutyCycle = (DutyCycle*ActlPeriod/100);
  if (clock == IfxGtm_Cmu_Clk_7)
  {
    clock = IfxGtm_Cmu_Clk_0;
  }
  else
  {
    clock++;
  }

  IfxGtm_Atom_Timer_disableUpdate(myatomtimer);
  IfxGtm_Atom_Ch_setClockSource(myatomtimer->atom, myatomtimer->timerChannel, clock);
  IfxGtm_Atom_Ch_setCompareShadow(myatomtimer->atom, myatomtimer->timerChannel, ActlPeriod, ActlDutyCycle);
  IfxGtm_Atom_Timer_applyUpdate(myatomtimer);
}

void AtomTimer_SetPeriodActl(IfxGtm_Atom_Timer *myatomtimer,uint32 Period)
{
  IfxGtm_Atom_Timer_disableUpdate(myatomtimer);
  IfxGtm_Atom_Ch_setCompareZeroShadow(myatomtimer->atom, myatomtimer->timerChannel, Period);
  IfxGtm_Atom_Timer_applyUpdate(myatomtimer);
}

void AtomTimer_SetDutyCycleAct(IfxGtm_Atom_Timer *myatomtimer, uint32 DutyCycle)
{
  IfxGtm_Atom_Timer_disableUpdate(myatomtimer);
  IfxGtm_Atom_Ch_setCompareOneShadow(myatomtimer->atom, myatomtimer->timerChannel, DutyCycle);
  IfxGtm_Atom_Timer_applyUpdate(myatomtimer);
}

