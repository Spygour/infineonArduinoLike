
/**********************************************************************************************************************
 * \file phaseShiftPwm.c
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
#include "ThreeTimerPhase.h"
/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/
#define MAX_TOM_CHANNELS            3



/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/
/* Structure for three phase PWM output configuration and handling */
typedef struct
{
    IfxGtm_Tom_Timer    timer;                  /* Timer driver                                                      */
    IfxGtm_Tom_PwmHl    pwm;                    /* GTM TOM PWM driver object                                         */
    Ifx_TimerValue      pwmOnTimes[2];          /* PWM ON-time for 3 phases                                          */
    IfxGtm_Tom_Pwm3_Ch  update;
} Pwm3_Timers_Output;

Pwm3_Timers_Output GpwmThreePhaseOutput;
/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
/*********************************************************************************************************************/
/*This function sets default pwm signals for every phase*/
IFX_STATIC void IfxGtmTomPwmThreeTimersUpdateOff(IfxGtm_Tom_PwmHl *driver, Ifx_TimerValue *tOn)
{
    IFX_UNUSED_PARAMETER(tOn)
    uint8 ChannelIndex;
    Ifx_TimerValue period;

    period = driver->timer->base.period;

    for (ChannelIndex = 0; ChannelIndex < driver->base.channelCount; ChannelIndex++)
    {
        IfxGtm_Tom_Ch_setCompareShadow(driver->tom, driver->ccxTemp[ChannelIndex],
            2 /* 1 will keep the previous level */, 2 + 2);
    }
}


/*This function runs the function pointer of the GpwmThreePhaseOutput.update structure and sets the dutycycle of the pwm signals*/
void IfxGtm_TomPwmThreeTimersSetOnTime(IfxGtm_Tom_PwmHl *driver, Ifx_TimerValue *tOn)
{
    GpwmThreePhaseOutput.update.setMode(driver, tOn);
}


/*This function Starts the three phase pwm signals*/
IFX_STATIC void IfxGtmTomPwmThreeTimersUpdateCenterAligned(IfxGtm_Tom_PwmHl *driver, Ifx_TimerValue *tOn)
{
    uint8          ChannelIndex;
    Ifx_TimerValue Period;

    Period = driver->timer->base.period; /*Get period of the master timer*/

    for (ChannelIndex = 0; ChannelIndex < driver->base.channelCount; ChannelIndex++)
    {
        Ifx_TimerValue X;             /* x=period*dutyCycle, x=OnTime+deadTime */
        Ifx_TimerValue Cm0, Cm1;
        X = tOn[ChannelIndex];
        Cm1 = X;
        Cm0 = Period/2 + X;
        while (Cm1 >= Period)
        {
          Cm1 -= Period+1;
        }

        while (Cm0  >= Period)
        {
          Cm0 -= Period+1;
        }
        IfxGtm_Tom_Ch_setCompareShadow(driver->tom, driver->ccxTemp[ChannelIndex],(uint16)Cm0,(uint16)Cm1);
    }
}

/*This pointer of structures has the different modes of the three phase signals, at first we use the configuration of
 * IfxGtmTomPwm3chModes[1] structure and then the configuration of IfxGtmTomPwm3chModes[0] structure*/
IFX_STATIC IFX_CONST IfxGtm_Tom_Pwm3_Ch IfxGtmTomPwm3chModes[2] = {
    {Ifx_Pwm_Mode_centerAligned,         FALSE, &IfxGtmTomPwmThreeTimersUpdateCenterAligned  },
    {Ifx_Pwm_Mode_off,                   FALSE, &IfxGtmTomPwmThreeTimersUpdateOff   }
    /*To add two more modes*/
};


/*This function sets the mode of the three phase pwm signals*/
boolean IfxGtm_TomPwmThreeTimersSetMode(IfxGtm_Tom_PwmHl *driver, Ifx_Pwm_Mode mode)
{
    boolean                Result = TRUE;
    IfxGtm_Tom_PwmHl_Base *Base   = &driver->base;

    if (Base->mode != mode)
    {
        if (mode > Ifx_Pwm_Mode_off)
        {
            mode = Ifx_Pwm_Mode_off;
            Result = FALSE;
        }

        IFX_ASSERT(IFX_VERBOSE_LEVEL_ERROR, mode == IfxGtm_Tom_Pwmccx_modes[mode].mode);

        Base->mode             = mode;
        GpwmThreePhaseOutput.update         = IfxGtmTomPwm3chModes[0];

            driver->ccxTemp   = driver->ccx;

        {   /* Workaround to enable the signal inversion required for center aligned inverted
             * and right aligned modes */
            /** \note that changing signal level may produce short circuit at the power stage,
             * in which case the inverter must be disable during this action. */

            /* Ifx_Pwm_Mode_centerAligned and Ifx_Pwm_Mode_LeftAligned use inverted=FALSE */
            /* Ifx_Pwm_Mode_centerAlignedInverted and Ifx_Pwm_Mode_RightAligned use inverted=TRUE */
            uint32 ChannelIndex;

            for (ChannelIndex = 0; ChannelIndex < driver->base.channelCount; ChannelIndex++)
            {
                IfxGtm_Tom_Ch Channel;

                Channel = driver->ccx[ChannelIndex];
                IfxGtm_Tom_Ch_setSignalLevel(driver->tom, Channel, Base->ccxActiveState); /*Every phase must have the same signal level, active state high*/

            }
        }
    }

    return Result;
}



/*This function returns true when the three timers are initialized correctly*/
boolean IfxGtm_TomPwmThreeTimersInit(IfxGtm_Tom_PwmHl *driver, IfxGtm_Tom_PwmHl_Config *config) {
    boolean           Result = TRUE;
    uint16            ChannelMask;
    uint16            ChannelsMask = 0;
    uint32            ChannelIndex;
    uint16            MaskShift    = 0;
    IfxGtm_Tom_Timer  *Timer        = config->timer;

    driver->base.mode             = Ifx_Pwm_Mode_init;
    driver->timer                 = Timer;
    driver->base.setMode          = 0;
    driver->base.inverted         = FALSE;
    driver->base.ccxActiveState   = config->base.ccxActiveState; /*Gets the active state of pwm signals from the configuration struct*/
    driver->base.channelCount     = config->base.channelCount; /*Gets the max channel count of controlled pwm signals by the master timer*/


    driver->tom = &(Timer->gtm->TOM[config->tom]); /*Gets the Tom used as master*/

    /* config->ccx[0] is used for the definition of the TGC */
    if (config->ccx[0]->channel <= 7)
    {
        driver->tgc = IfxGtm_Tom_Ch_getTgcPointer(driver->tom, 0); /*In case we have channels from 0-6 we enable global control register 0*/
    }
    else
    {
        driver->tgc = IfxGtm_Tom_Ch_getTgcPointer(driver->tom, 1); /*In case we have channels from 7-15 we enable global control register 1*/
    }

    MaskShift = (config->ccx[0]->channel <= 7) ? 0 : 8;

    IFX_ASSERT(IFX_VERBOSE_LEVEL_ERROR, config->base.channelCount <= IFXGTM_TOM_PWMHL_MAX_NUM_CHANNELS); /*In case we have less than 2 controlled pwm signals return 0*/

    IfxGtm_Tom_Ch_ClkSrc Clock = IfxGtm_Tom_Ch_getClockSource(Timer->tom, Timer->timerChannel); /*Set prescaler for every phase*/

    for (ChannelIndex = 0; ChannelIndex < config->base.channelCount; ChannelIndex++)
    {
        IfxGtm_Tom_Ch Channel;
        /* CCX */
        Channel                   = config->ccx[ChannelIndex]->channel;
        driver->ccx[ChannelIndex] = Channel;
        ChannelMask               = 1 << (Channel - MaskShift);
        ChannelsMask             |= ChannelMask;

        /* Initialize the timer part */
        IfxGtm_Tom_Ch_setClockSource(driver->tom, Channel, Clock);

        /* Initialize the SOUR reset value and enable the channel */
        IfxGtm_Tom_Ch_setSignalLevel(driver->tom, Channel,config->base.ccxActiveState);
        IfxGtm_Tom_Tgc_enableChannels(driver->tgc, ChannelMask, 0, TRUE); /* Write the SOUR outout with !SL */
        IfxGtm_Tom_Tgc_enableChannelsOutput(driver->tgc, ChannelMask, 0, TRUE);

        /* Set the SL to the required level for run time */
        IfxGtm_Tom_Ch_setSignalLevel(driver->tom, Channel, config->base.ccxActiveState);
        IfxGtm_Tom_Ch_setResetSource(driver->tom, Channel, IfxGtm_Tom_Ch_ResetEvent_onTrigger);
        IfxGtm_Tom_Ch_setTriggerOutput(driver->tom, Channel, IfxGtm_Tom_Ch_OutputTrigger_forward);
        IfxGtm_Tom_Ch_setCounterValue(driver->tom, Channel, IfxGtm_Tom_Timer_getOffset(driver->timer));

        /*Initialize the port in case it does not exists on pinmap*/
        if (config->initPins == TRUE)
        {
            IfxGtm_PinMap_setTomTout(config->ccx[ChannelIndex],
                config->base.outputMode, config->base.outputDriver);
            IfxPort_setPinState(config->ccx[ChannelIndex]->pin.port, config->ccx[ChannelIndex]->pin.pinIndex, config->base.ccxActiveState ? IfxPort_State_low : IfxPort_State_high);
        }
    }
    IfxGtm_TomPwmThreeTimersSetMode(driver, Ifx_Pwm_Mode_off); /*Enable the timers by having zero dutycycle*/

    Ifx_TimerValue tOn[MAX_TOM_CHANNELS] = {0};
    IfxGtmTomPwmThreeTimersUpdateOff(driver, tOn);     /* tOn do not need defined values */
    /* Transfer the shadow registers */
    IfxGtm_Tom_Tgc_setChannelsForceUpdate(driver->tgc, ChannelsMask, 0, 0, 0);
    IfxGtm_Tom_Tgc_trigger(driver->tgc);
    IfxGtm_Tom_Tgc_setChannelsForceUpdate(driver->tgc, 0, ChannelsMask, 0, 0);

    /* Enable timer to update the channels */
    for (ChannelIndex = 0; ChannelIndex < driver->base.channelCount; ChannelIndex++)
    {
        IfxGtm_Tom_Timer_addToChannelMask(Timer, driver->ccx[ChannelIndex]);
    }

    return Result;
}

/*Main function of the Three timers Phase shift PWM*/
void Init_ThreeTimersPwm(uint32 Period,uint32 phaseShift,IfxGtm_Tom_ToutMap* masterPin, IfxGtm_Tom_ToutMapP* slavePins)
{
  uint16 Prescaler[5] = {1, 16, 256, 4096, 32768};
  uint32 PeriodActl = Period*100;
  IfxGtm_Tom_Ch_ClkSrc ClckSrc= 0;
  float32 freq;
  while(PeriodActl > 0xFFFF)
  {
    PeriodActl = PeriodActl/Prescaler[ClckSrc];
    ClckSrc += 1;
  }
  freq = 1000000/(float32)Period;
  IfxGtm_enable(&MODULE_GTM);
  /* Set the GTM global clock frequency in Hz */
  IfxGtm_Cmu_setGclkFrequency(&MODULE_GTM, IfxGtm_Cmu_getModuleFrequency(&MODULE_GTM));
  /* Set the GTM configurable clock frequency in Hz */
  IfxGtm_Cmu_setClkFrequency(&MODULE_GTM, IfxGtm_Cmu_Clk_0, IfxGtm_Cmu_getGclkFrequency(&MODULE_GTM));
  /* Enable the FXU clock                         */
  IfxGtm_Cmu_enableClocks(&MODULE_GTM, IFXGTM_CMU_CLKEN_FXCLK);
  IfxGtm_Tom_Timer_Config TimerConfig;                                        /* Timer configuration              */
  IfxGtm_Tom_Timer_initConfig(&TimerConfig, &MODULE_GTM);                     /* Initialize timer configuration   */
  /*init pwm using the IfxGtm_Tom_Timer_Config of the Tom timer library*/
  TimerConfig.triggerOut           = masterPin;                               /* Set the output pin used by the master timer*/
  TimerConfig.initPins             = TRUE;                                    /* Enable the initialization of the ouput pin*/
  TimerConfig.base.trigger.enabled = TRUE;                                    /* Enable the trigger of the output pin*/
  TimerConfig.base.trigger.risingEdgeAtPeriod = Ifx_ActiveState_high;         /* Active state high*/
  TimerConfig.base.countDir        = IfxStdIf_Timer_CountDir_up;              /* Count mode up*/
  TimerConfig.base.trigger.outputDriver = IfxPort_PadDriver_cmosAutomotiveSpeed1; /*pad driver speed grade 1*/
  TimerConfig.base.trigger.outputMode = IfxPort_OutputMode_pushPull;          /* output mode push pull*/
  TimerConfig.base.trigger.outputEnabled = TRUE;                              /* Enable output from trigger pin*/
  TimerConfig.irqModeTrigger       = IfxGtm_IrqMode_pulseNotify;              /* Enable the pulse of the Output signal as interrupt*/

  TimerConfig.base.frequency       = freq;                                /* Set timer frequency              */
  TimerConfig.base.isrPriority     = 0;                                       /* Disable interrupts */
  TimerConfig.base.isrProvider     = IfxSrc_Tos_cpu0;                         /* Isr on cpu 0*/
  TimerConfig.tom                  = masterPin->tom;                          /* Define the timer used            */
  TimerConfig.timerChannel         = masterPin->channel;                      /* Define the channel used          */
  TimerConfig.clock                = ClckSrc;
  /* Define the CMU clock used        */

  IfxGtm_Tom_Timer_init(&GpwmThreePhaseOutput.timer, &TimerConfig);                        /* Initialize the TOM               */
  IfxGtm_Tom_Tgc_enableChannelUpdate(GpwmThreePhaseOutput.timer.tgc[0],GpwmThreePhaseOutput.timer.triggerChannel,TRUE);

  IfxGtm_Tom_PwmHl_Config PwmHlConfig;

  IfxGtm_Tom_ToutMapP Ccx[] =
          {
              slavePins[0], /* PWM High-side 1 */
              slavePins[1], /* PWM High-side 2 */
              NULL_PTR  /* PWM High-side 3 is null since the third pwm phase is provide by the master timer*/
          };
  IfxGtm_Tom_PwmHl_initConfig(&PwmHlConfig);
  PwmHlConfig.base.channelCount = 2; /* Controlled channels are two */

  PwmHlConfig.base.outputMode = IfxPort_OutputMode_pushPull; /* Output mode is push pull */


  PwmHlConfig.base.outputDriver = IfxPort_PadDriver_cmosAutomotiveSpeed1; /* Pad driver speed grade 1 */

  /* Set top PWM active state */
  PwmHlConfig.base.ccxActiveState = Ifx_ActiveState_low;


  PwmHlConfig.ccx   = Ccx;                        /* Assign the channels for High-side switches   */

  PwmHlConfig.timer = &GpwmThreePhaseOutput.timer;  /* Use as master timer the timer we initialized */
  PwmHlConfig.tom   = TimerConfig.tom;

  IfxGtm_TomPwmThreeTimersInit(&GpwmThreePhaseOutput.pwm, &PwmHlConfig); /* Run the initialize of the three timers function */

  IfxGtm_TomPwmThreeTimersSetMode(&GpwmThreePhaseOutput.pwm, Ifx_Pwm_Mode_centerAligned); /* Start the actual PWM signals */
  /* Update the input frequency */
  IfxGtm_Tom_Timer_updateInputFrequency(&GpwmThreePhaseOutput.timer);
  IfxGtm_Tom_Timer_run(&GpwmThreePhaseOutput.timer); /* Start the count of master timer */
  /* Calculate initial values of PWM duty cycles */
  GpwmThreePhaseOutput.pwmOnTimes[0] = PeriodActl * phaseShift/100; /* First controlled signal is shifted to a percentage of master timer's period */
  GpwmThreePhaseOutput.pwmOnTimes[1] = PeriodActl * 2 * phaseShift/100; /* Second controlled signal is shifted to two times the percentage of master timer's period */
  /* Update PWM duty cycles */
  IfxGtm_Tom_Timer_disableUpdate(&GpwmThreePhaseOutput.timer); /* Disable the update of master timer */
  IfxGtm_Tom_Ch_setCompareShadow(GpwmThreePhaseOutput.timer.tom, GpwmThreePhaseOutput.timer.timerChannel,(uint16)PeriodActl,(uint16)PeriodActl/2);/* Set dutycycle 50 per cent */
  IfxGtm_TomPwmThreeTimersSetOnTime(&GpwmThreePhaseOutput.pwm, GpwmThreePhaseOutput.pwmOnTimes); /* Set the shadow registers of the controlled pwm signals */
  IfxGtm_Tom_Timer_applyUpdate(&GpwmThreePhaseOutput.timer);  /* Start every PWM signal at the same time */
}

