
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
#include "../InfineonArduinoLike/AtomAndAru/AtomPhasePwm.h"
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
    IfxGtm_Atom_Timer    timer;                  /* Timer driver                                                      */
    IfxGtm_Atom_PwmHl    pwm;                    /* GTM TOM PWM driver object                                         */
    Ifx_TimerValue      pwmOnTimes[2];          /* PWM ON-time for 3 phases                                          */
    IfxGtm_Atom_Pwm3_Ch  update;
} Pwm3_Timers_Output;

Pwm3_Timers_Output AtomPhasePwm_Output;
/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
/*********************************************************************************************************************/
/*This function sets default pwm signals for every phase*/
IFX_STATIC void IfxGtmTomPwmThreeTimersUpdateOff(IfxGtm_Atom_PwmHl *driver, Ifx_TimerValue *tOn)
{
    IFX_UNUSED_PARAMETER(tOn)
    uint8 ChannelIndex;

    for (ChannelIndex = 0; ChannelIndex < driver->base.channelCount; ChannelIndex++)
    {
        IfxGtm_Atom_Ch_setCompareShadow(driver->atom, driver->ccxTemp[ChannelIndex],
            2 /* 1 will keep the previous level */, 2 + 2);
    }
}


/*This function runs the function pointer of the AtomPhasePwm_Output.update structure and sets the dutycycle of the pwm signals*/
void IfxGtm_AtomPwmThreeTimersSetOnTime(IfxGtm_Atom_PwmHl *driver, Ifx_TimerValue *tOn)
{
    AtomPhasePwm_Output.update.setMode(driver, tOn);
}


/*This function Starts the three phase pwm signals*/
IFX_STATIC void IfxGtmTomPwmThreeTimersUpdateCenterAligned(IfxGtm_Atom_PwmHl *driver, Ifx_TimerValue *tOn)
{
  uint8          channelIndex;
  Ifx_TimerValue period;
  Ifx_TimerValue deadtime = driver->base.deadtime;

  period = driver->timer->base.period;

  for (channelIndex = 0; channelIndex < driver->base.channelCount; channelIndex++)
  {
      Ifx_TimerValue x; /* x=period*dutyCycle, x=OnTime+deadTime */
      Ifx_TimerValue cm0, cm1;
      x = tOn[channelIndex];

      if (driver->base.inverted != FALSE)
      {
          x = period - x;
      }
      else
      {}

      if ((x < driver->base.minPulse) || (x <= deadtime))
      {   /* For deadtime condition: avoid leading edge of top channel to occur after the trailing edge */
          x = 0;
      }
      else if (x > driver->base.maxPulse)
      {
          x = period;
      }
      else
      {}

      /* Special handling due to GTM issue */
      if (x == period)
      {   /* 100% duty cycle */
          IfxGtm_Atom_Ch_setCompareShadow(driver->atom, driver->ccxTemp[channelIndex],
              period + 1 /* No compare event */,
              2 /* 1st compare event (issue: expected to be 1)*/ + deadtime);
      }
      else if (x == 0)
      {
          cm0 = 1;
          cm1 = period + 2;
          IfxGtm_Atom_Ch_setCompareShadow(driver->atom, driver->ccxTemp[channelIndex], cm0, cm1);
      }
      else
      {                           /* x% duty cycle */
          cm1 = (period - x) / 2; // CM1
          cm0 = (period + x) / 2; // CM0
          IfxGtm_Atom_Ch_setCompareShadow(driver->atom, driver->ccxTemp[channelIndex], cm0, cm1 + deadtime);
      }
  }
}

/*This pointer of structures has the different modes of the three phase signals, at first we use the configuration of
 * IfxGtmTomPwm3chModes[1] structure and then the configuration of IfxGtmTomPwm3chModes[0] structure*/
IFX_STATIC IFX_CONST IfxGtm_Atom_Pwm3_Ch IfxGtmTomPwm3chModes[2] = {
    {Ifx_Pwm_Mode_centerAligned,         FALSE, &IfxGtmTomPwmThreeTimersUpdateCenterAligned  },
    {Ifx_Pwm_Mode_off,                   FALSE, &IfxGtmTomPwmThreeTimersUpdateOff   }
    /*To add two more modes*/
};


/*This function sets the mode of the three phase pwm signals*/
boolean IfxGtm_AtomPwmThreeTimersSetMode(IfxGtm_Atom_PwmHl *driver, Ifx_Pwm_Mode mode)
{
    boolean                Result = TRUE;
    IfxGtm_Atom_PwmHl_Base *Base   = &driver->base;

    if (Base->mode != mode)
    {
        if (mode > Ifx_Pwm_Mode_off)
        {
            mode = Ifx_Pwm_Mode_off;
            Result = FALSE;
        }

        IFX_ASSERT(IFX_VERBOSE_LEVEL_ERROR, mode == IfxGtm_Atom_Pwmccx_modes[mode].mode);

        Base->mode             = mode;
        AtomPhasePwm_Output.update         = IfxGtmTomPwm3chModes[0];

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
                IfxGtm_Atom_Ch Channel;

                Channel = driver->ccx[ChannelIndex];
                IfxGtm_Atom_Ch_setSignalLevel(driver->atom, Channel, Base->ccxActiveState); /*Every phase must have the same signal level, active state high*/

            }
        }
    }

    return Result;
}



/*This function returns true when the three timers are initialized correctly*/
boolean IfxGtm_AtomPwmThreeTimersInit(IfxGtm_Atom_PwmHl *driver, IfxGtm_Atom_PwmHl_Config *config) {
    boolean           Result = TRUE;
    uint16            ChannelMask;
    uint16            ChannelsMask = 0;
    uint32            ChannelIndex;
    IfxGtm_Atom_Timer  *Timer        = config->timer;

    driver->base.mode             = Ifx_Pwm_Mode_init;
    driver->timer                 = Timer;
    driver->base.setMode          = 0;
    driver->base.inverted         = FALSE;
    driver->base.ccxActiveState   = config->base.ccxActiveState; /*Gets the active state of pwm signals from the configuration struct*/
    driver->base.channelCount     = config->base.channelCount; /*Gets the max channel count of controlled pwm signals by the master timer*/


    driver->atom = &(Timer->gtm->ATOM[config->atom]); /*Gets the Tom used as master*/

    /* config->ccx[0] is used for the definition of the TGC */
    driver->agc = (Ifx_GTM_ATOM_AGC *)&driver->atom->AGC.GLB_CTRL; /*In case we have channels from 0-6 we enable global control register 0*/


    IFX_ASSERT(IFX_VERBOSE_LEVEL_ERROR, config->base.channelCount <= IfxGtm_Atom_PWMHL_MAX_NUM_CHANNELS); /*In case we have less than 2 controlled pwm signals return 0*/

    IfxGtm_Cmu_Clk Clock = IfxGtm_Atom_Ch_getClockSource(Timer->atom, Timer->timerChannel); /*Set prescaler for every phase*/

    for (ChannelIndex = 0; ChannelIndex < config->base.channelCount; ChannelIndex++)
    {
        IfxGtm_Atom_Ch Channel;
        /* CCX */
        Channel                   = config->ccx[ChannelIndex]->channel;
        driver->ccx[ChannelIndex] = Channel;
        ChannelMask               = 1 << (Channel);
        ChannelsMask             |= ChannelMask;

        /* Initialize the timer part */
        IfxGtm_Atom_Ch_configurePwmMode(driver->atom, Channel, Clock,
                    config->base.ccxActiveState,
                    IfxGtm_Atom_Ch_ResetEvent_onTrigger, IfxGtm_Atom_Ch_OutputTrigger_forward);

        /* Initialize the port */
        if (config->initPins == TRUE)
        {
            IfxGtm_PinMap_setAtomTout(config->ccx[ChannelIndex],
                config->base.outputMode, config->base.outputDriver);
        }

    }
    IfxGtm_Atom_Agc_enableChannelsOutput(driver->agc, ChannelsMask, 0, TRUE);
    IfxGtm_Atom_Agc_enableChannels(driver->agc, ChannelsMask, 0, TRUE);


    IfxGtm_AtomPwmThreeTimersSetMode(driver, Ifx_Pwm_Mode_off); /*Enable the timers by having zero dutycycle*/

    Ifx_TimerValue tOn[MAX_TOM_CHANNELS] = {0};
    IfxGtmTomPwmThreeTimersUpdateOff(driver, tOn);     /* tOn do not need defined values */
    /* Transfer the shadow registers */
    IfxGtm_Atom_Agc_setChannelsForceUpdate(driver->agc, ChannelsMask, 0, 0, 0);
    IfxGtm_Atom_Agc_trigger(driver->agc);
    IfxGtm_Atom_Agc_setChannelsForceUpdate(driver->agc, 0, ChannelsMask, 0, 0);

    /* Enable timer to update the channels */
    for (ChannelIndex = 0; ChannelIndex < driver->base.channelCount; ChannelIndex++)
    {
        IfxGtm_Atom_Timer_addToChannelMask(Timer, driver->ccx[ChannelIndex]);
    }

    return Result;
}

/*Main function of the Three timers Phase shift PWM*/
void AtomPhasePwm_Init(float32 baseFrequency,uint32 phaseShift,IfxGtm_Atom_ToutMap* masterPin, IfxGtm_Atom_ToutMapP* slavePins)
{
  IfxGtm_Cmu_Clk ClckSrc= IfxGtm_Cmu_Clk_1;
  float32 frequency;
  /* Enable GTM, it is necessary if the GTM is not initialized earlier */
  IfxGtm_enable(&MODULE_GTM);

  frequency = IfxGtm_Cmu_getModuleFrequency(&MODULE_GTM);
  /* Set the global clock frequency to the max */
  IfxGtm_Cmu_setGclkFrequency(&MODULE_GTM, frequency);
  /* Set the CMU CLK0 */
  IfxGtm_Cmu_setClkFrequency(&MODULE_GTM, ClckSrc, frequency);
  /* Enable the FXU clock */
  IfxGtm_Cmu_enableClocks(&MODULE_GTM, IFXGTM_CMU_CLKEN_CLK1);
  IfxGtm_Atom_Timer_Config TimerConfig;                                        /* Timer configuration              */
  IfxGtm_Atom_Timer_initConfig(&TimerConfig, &MODULE_GTM);                     /* Initialize timer configuration   */

  /*init pwm using the IfxGtm_Atom_Timer_Config of the Tom timer library*/
  TimerConfig.triggerOut           = masterPin;                               /* Set the output pin used by the master timer*/
  TimerConfig.initPins             = TRUE;                                    /* Enable the initialization of the ouput pin*/
  TimerConfig.base.trigger.enabled = TRUE;                                    /* Enable the trigger of the output pin*/
  TimerConfig.base.trigger.risingEdgeAtPeriod = Ifx_ActiveState_high;         /* Active state high*/
  TimerConfig.base.countDir        = IfxStdIf_Timer_CountDir_up;              /* Count mode up*/
  TimerConfig.base.trigger.outputDriver = IfxPort_PadDriver_cmosAutomotiveSpeed1; /*pad driver speed grade 1*/
  TimerConfig.base.trigger.outputMode = IfxPort_OutputMode_pushPull;          /* output mode push pull*/
  TimerConfig.base.trigger.outputEnabled = TRUE;                              /* Enable output from trigger pin*/
  TimerConfig.irqModeTrigger       = IfxGtm_IrqMode_pulseNotify;              /* Enable the pulse of the Output signal as interrupt*/

  TimerConfig.base.frequency       = baseFrequency;                                /* Set timer frequency              */
  TimerConfig.base.isrPriority     = 0;                                       /* Disable interrupts */
  TimerConfig.base.isrProvider     = IfxSrc_Tos_cpu0;                         /* Isr on cpu 0*/
  TimerConfig.atom                  = masterPin->atom;                          /* Define the timer used            */
  TimerConfig.timerChannel         = masterPin->channel;                      /* Define the channel used          */
  TimerConfig.clock                = ClckSrc;
  TimerConfig.base.trigger.triggerPoint = 2;
  /* Define the CMU clock used        */

  IfxGtm_Atom_Timer_init(&AtomPhasePwm_Output.timer, &TimerConfig);                        /* Initialize the TOM               */
  IfxGtm_Atom_Agc_enableChannelUpdate(AtomPhasePwm_Output.timer.agc,AtomPhasePwm_Output.timer.triggerChannel,TRUE);

  IfxGtm_Atom_PwmHl_Config PwmHlConfig;

  IfxGtm_Atom_ToutMapP Ccx[] =
          {
              slavePins[0], /* PWM High-side 1 */
              slavePins[1], /* PWM High-side 2 */
              NULL_PTR  /* PWM High-side 3 is null since the third pwm phase is provide by the master timer*/
          };
  IfxGtm_Atom_PwmHl_initConfig(&PwmHlConfig);
  PwmHlConfig.base.channelCount = 2; /* Controlled channels are two */

  PwmHlConfig.base.outputMode = IfxPort_OutputMode_pushPull; /* Output mode is push pull */


  PwmHlConfig.base.outputDriver = IfxPort_PadDriver_cmosAutomotiveSpeed1; /* Pad driver speed grade 1 */

  /* Set top PWM active state */
  PwmHlConfig.base.ccxActiveState = Ifx_ActiveState_high;


  PwmHlConfig.ccx   = Ccx;                        /* Assign the channels for High-side switches   */

  PwmHlConfig.timer = &AtomPhasePwm_Output.timer;  /* Use as master timer the timer we initialized */
  PwmHlConfig.atom   = TimerConfig.atom;

  IfxGtm_AtomPwmThreeTimersInit(&AtomPhasePwm_Output.pwm, &PwmHlConfig); /* Run the initialize of the three timers function */
  AtomPhasePwm_Output.pwm.base.maxPulse = AtomPhasePwm_Output.timer.base.period - 5;

  IfxGtm_AtomPwmThreeTimersSetMode(&AtomPhasePwm_Output.pwm, Ifx_Pwm_Mode_centerAligned); /* Start the actual PWM signals */
  /* Update the input frequency */
  IfxGtm_Atom_Timer_updateInputFrequency(&AtomPhasePwm_Output.timer);
  IfxGtm_Atom_Timer_run(&AtomPhasePwm_Output.timer); /* Start the count of master timer */
  /* Calculate initial values of PWM duty cycles */
  AtomPhasePwm_Output.pwmOnTimes[0] = AtomPhasePwm_Output.timer.base.period/4; /* First controlled signal is shifted to a percentage of master timer's period */
  AtomPhasePwm_Output.pwmOnTimes[1] = AtomPhasePwm_Output.timer.base.period/2; /* Second controlled signal is shifted to two times the percentage of master timer's period */
  /* Update PWM duty cycles */
  IfxGtm_Atom_Timer_disableUpdate(&AtomPhasePwm_Output.timer); /* Disable the update of master timer */
  IfxGtm_Atom_Ch_setCompareShadow(AtomPhasePwm_Output.timer.atom, AtomPhasePwm_Output.timer.triggerChannel,AtomPhasePwm_Output.timer.base.period, AtomPhasePwm_Output.timer.base.period/2 );
  IfxGtm_AtomPwmThreeTimersSetOnTime(&AtomPhasePwm_Output.pwm, AtomPhasePwm_Output.pwmOnTimes); /* Set the shadow registers of the controlled pwm signals */
  IfxGtm_Atom_Timer_applyUpdate(&AtomPhasePwm_Output.timer);  /* Start every PWM signal at the same time */
}


void AtomPhasePwm_SetDutyCycle(uint32* pwmOnTime)
{
  AtomPhasePwm_Output.pwmOnTimes[0] = pwmOnTime[0]; /* First controlled signal is shifted to a percentage of master timer's period */
  AtomPhasePwm_Output.pwmOnTimes[1] = pwmOnTime[1]; /* Second controlled signal is shifted to two times the percentage of master timer's period */

  IfxGtm_Atom_Timer_disableUpdate(&AtomPhasePwm_Output.timer); /* Disable the update of master timer */
  IfxGtm_Atom_Ch_setCompareOneShadow(AtomPhasePwm_Output.timer.atom, AtomPhasePwm_Output.timer.triggerChannel, 50000);
  IfxGtm_AtomPwmThreeTimersSetOnTime(&AtomPhasePwm_Output.pwm, AtomPhasePwm_Output.pwmOnTimes); /* Set the shadow registers of the controlled pwm signals */
  IfxGtm_Atom_Timer_applyUpdate(&AtomPhasePwm_Output.timer);  /* Start every PWM signal at the same time */
}

