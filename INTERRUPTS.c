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
#include "Ifx_Types.h"
#include "IfxGpt12.h"
#include "IfxPort.h"
#include "IfxGtm_Atom_Timer.h"
#include "IfxGtm_Tom_Timer.h"

#include "INTERRUPTS.h"

/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/                    /* Define the GPT12 Timer interrupt priority            */
#define ISR_PROVIDER_GPT12_TIMER    IfxSrc_Tos_cpu0/* Interrupt provider                                   */
#define RELOAD_VALUE                48828u                  /* Reload value to have an interrupt each 500ms         */
#define LED                         &MODULE_P00, 6          /* LED which toggles in the Interrupt Service Routine   */
#define CMU_FREQ            1000000.0f
/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/

/*********************************************************************************************************************/
/*--------------------------------------------Private Variables/Constants--------------------------------------------*/
/*********************************************************************************************************************/
/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
/*********************************************************************************************************************/
/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
/*********************************************************************************************************************/
void initTomInterrupt(IfxGtm_Tom_Timer *mytomtimer,float freq,uint16 priority,uint16 tom,uint16 channel, uint16 clock)
{
    IfxGtm_enable(&MODULE_GTM); /* Enable GTM */

    IfxGtm_Tom_Timer_Config timerConfig;                                        /* Timer configuration              */
    IfxGtm_Tom_Timer_initConfig(&timerConfig, &MODULE_GTM);                     /* Initialize timer configuration   */

    timerConfig.base.frequency       = freq;                                /* Set timer frequency              */
    timerConfig.base.isrPriority     = priority;                        /* Set interrupt priority           */
    timerConfig.base.isrProvider     = IfxSrc_Tos_cpu0;                         /* Set interrupt provider           */
    timerConfig.tom                  = tom;                            /* Define the timer used            */
    timerConfig.timerChannel         = channel;                         /* Define the channel used          */
    timerConfig.clock                = clock;
    /* Define the CMU clock used        */

    IfxGtm_Cmu_enableClocks(&MODULE_GTM, IFXGTM_CMU_CLKEN_FXCLK);               /* Enable the CMU clock             */
    IfxGtm_Tom_Timer_init(mytomtimer, &timerConfig);                        /* Initialize the TOM               */

    IfxPort_setPinModeOutput(LED, IfxPort_OutputMode_pushPull, IfxPort_OutputIdx_general);  /* Set pin mode         */

    IfxGtm_Tom_Timer_run(mytomtimer); /* Start the TOM */
}

void initAtomInterrupt(IfxGtm_Atom_Timer* mytimer,float32 frequency, uint16 priority,uint16 atom,uint16 channel, uint16 clock)
{
    IfxGtm_enable(&MODULE_GTM);                                             /* Enable GTM                       */

    IfxGtm_Atom_Timer_Config timerConfig;                                   /* Timer configuration structure    */
    IfxGtm_Atom_Timer_initConfig(&timerConfig, &MODULE_GTM);                /* Initialize default parameters    */

    timerConfig.atom = atom;                                       /* Select the ATOM_0                */
    timerConfig.timerChannel = channel;                            /* Select channel 0                 */
    timerConfig.clock = clock;                                   /* Select the CMU clock 0           */
    timerConfig.base.frequency = frequency;                                 /* Set timer frequency              */
    timerConfig.base.isrPriority = priority;                       /* Set interrupt priority           */
    timerConfig.base.isrProvider = IfxSrc_Tos_cpu0;                         /* Set interrupt provider           */

    IfxGtm_Cmu_setClkFrequency(&MODULE_GTM, clock, CMU_FREQ);    /* Set the clock frequency          */
    IfxGtm_Cmu_enableClocks(&MODULE_GTM, IFXGTM_CMU_CLKEN_CLK0);            /* Enable the CMU clock 0           */
    IfxGtm_Atom_Timer_init(mytimer, &timerConfig);                   /* Initialize the ATOM              */


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



/* Interrupt Service Routine of the GPT12 */
void interruptGpt12(void)
{
    IfxPort_togglePin(LED);                                                         /* Toggle LED state             */
}
