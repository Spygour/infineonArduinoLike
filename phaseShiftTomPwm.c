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
#include <ThreePhaseShift.h>
/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/
#define MAX_TOM_CHANNELS 5
#define PHASE_U_HS                &IfxGtm_TOM1_4_TOUT14_P00_5_OUT /*                */
#define PHASE_V_HS                &IfxGtm_TOM1_2_TOUT12_P00_3_OUT  /*                */
#define PHASE_W_HS                &IfxGtm_TOM1_5_TOUT15_P00_6_OUT  /* Pin driven by the PWM, P33.2                   */




/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/
/* Structure for three phase PWM output configuration and handling */
typedef struct
{
    IfxGtm_Tom_Timer    timer;                  /* Timer driver                                                      */
    IfxGtm_Tom_PwmHl    pwm;                    /* GTM TOM PWM driver object                                         */
    Ifx_TimerValue      pwmOnTimes[3];          /* PWM ON-time for 3 phases                                          */
} Pwm3PhaseOutput;

Pwm3PhaseOutput g_pwm3PhaseOutput;

/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
/*********************************************************************************************************************/

/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
/*********************************************************************************************************************/

IFX_STATIC void IfxGtm_Tom_PwmCCX_updateOff(IfxGtm_Tom_PwmHl *driver, Ifx_TimerValue *tOn)
{
    IFX_UNUSED_PARAMETER(tOn)
    uint8 channelIndex;
    Ifx_TimerValue period;

    period = driver->timer->base.period;

    for (channelIndex = 0; channelIndex < driver->base.channelCount; channelIndex++)
    {
        IfxGtm_Tom_Ch_setCompareShadow(driver->tom, driver->ccxTemp[channelIndex],
            2 /* 1 will keep the previous level */, period + 2);
    }
}


IFX_STATIC void IfxGtm_Tom_PwmCCX_updatePulseOff(IfxGtm_Tom_PwmHl *driver, float32 *tOn, float32 *offset)
{
    IFX_UNUSED_PARAMETER(tOn)
    IFX_UNUSED_PARAMETER(offset)
    IfxGtm_Tom_PwmCCX_updateOff(driver, NULL_PTR);
}
IFX_STATIC void IfxGtm_Tom_PwmCCX_updateAndShiftOff(IfxGtm_Tom_PwmHl *driver, Ifx_TimerValue *tOn, Ifx_TimerValue *shift)
{
    IFX_UNUSED_PARAMETER(tOn)
    IFX_UNUSED_PARAMETER(shift)

    IfxGtm_Tom_PwmCCX_updateOff(driver, NULL_PTR);
}

void IfxGtm_Tom_PwmCCX_setOnTime(IfxGtm_Tom_PwmHl *driver, Ifx_TimerValue *tOn)
{
    driver->update(driver, tOn);
}

IFX_STATIC void IfxGtm_Tom_PwmCCX_updatePulse(IfxGtm_Tom_PwmHl *driver, float32 *tOn, float32 *offset)
{
    uint8          channelIndex;
    Ifx_TimerValue period;

    period = driver->timer->base.period;

    /* Top channels */
    for (channelIndex = 0; channelIndex < driver->base.channelCount; channelIndex++)
    {
        Ifx_TimerValue x; /* x=period*dutyCycle, x=OnTime+deadTime */
        Ifx_TimerValue o;
        Ifx_TimerValue cm0, cm1;

        x = IfxStdIf_Timer_sToTick(driver->timer->base.clockFreq, tOn[channelIndex]);
        o = IfxStdIf_Timer_sToTick(driver->timer->base.clockFreq, offset[channelIndex]);

        if (driver->base.inverted != FALSE)
        {
            x = period - x;
        }
        else
        {}

        if ((x < driver->base.minPulse) || (o > period))
        {
            x = 0;
        }
        else if ((x > driver->base.maxPulse) || (o + x > period))
        {
            x = period;
        }
        else
        {}

        /* Special handling due to GTM issue */
        if (x == period)
        {   /* 100% duty cycle */
            IfxGtm_Tom_Ch_setCompareShadow(driver->tom, driver->ccxTemp[channelIndex],
                period + 1 /* No compare event */,
                2 /* 1st compare event (issue: expected to be 1)*/);
        }
        else if (x == 0)
        {
            cm0 = 1;
            cm1 = period + 2;
            IfxGtm_Tom_Ch_setCompareShadow(driver->tom, driver->ccxTemp[channelIndex], cm0, cm1);
        }
        else
        {                /* x% duty cycle */
            cm1 = 2 + o; // CM1, set to 2 due to a GTM issue. should be 1 according to spec
            cm0 = o + x; // CM0, set to x+2 due to a GTM issue. should be x+1 according to spec
            IfxGtm_Tom_Ch_setCompareShadow(driver->tom, driver->ccxTemp[channelIndex], cm0, cm1);
        }
    }


}

IFX_STATIC void IfxGtm_Tom_PwmCCX_updateCenterAligned(IfxGtm_Tom_PwmHl *driver, Ifx_TimerValue *tOn)
{
    uint8          channelIndex;
    Ifx_TimerValue period;
    Ifx_TimerValue deadtime = driver->base.deadtime;

    period = driver->timer->base.period;

    for (channelIndex = 0; channelIndex < driver->base.channelCount; channelIndex++)
    {
        Ifx_TimerValue x;             /* x=period*dutyCycle, x=OnTime+deadTime */
        Ifx_TimerValue cm0, cm1;
        x = tOn[channelIndex];

        if (driver->base.inverted != FALSE)
        {
            x = period - x;
        }
        else
        {}

        if ((x < driver->base.minPulse) || (x <= deadtime))
        {                       /* For deadtime condition: avoid leading edge of top channel to occur after the trailing edge */
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
        {                       /* 100% duty cycle */
            IfxGtm_Tom_Ch_setCompareShadow(driver->tom, driver->ccxTemp[channelIndex],
                period + 1 /* No compare event */,
                2 /* 1st compare event (issue: expected to be 1) */ + deadtime);
        }
        else if (x == 0)
        {
            cm0 = 1;
            cm1 = period + 2;
            IfxGtm_Tom_Ch_setCompareShadow(driver->tom, driver->ccxTemp[channelIndex], cm0, cm1);
        }
        else
        {                           /* x% duty cycle */
            cm1 = (period - x) / 2; // CM1
            cm0 = (period + x) / 2; // CM0
            IfxGtm_Tom_Ch_setCompareShadow(driver->tom, driver->ccxTemp[channelIndex], x, x+100);
        }
    }
}

IFX_STATIC void IfxGtm_Tom_PwmCCX_updateShiftCenterAligned(IfxGtm_Tom_PwmHl *driver, Ifx_TimerValue *tOn, Ifx_TimerValue *shift)
{
    uint8          channelIndex;
    Ifx_TimerValue period;
    Ifx_TimerValue deadtime = driver->base.deadtime;

    period = driver->timer->base.period;

    for (channelIndex = 0; channelIndex < driver->base.channelCount; channelIndex++)
    {
        Ifx_TimerValue x; /* x=period*dutyCycle, x=OnTime+deadTime */
        Ifx_TimerValue s; /* Shift value */
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
            IfxGtm_Tom_Ch_setCompareShadow(driver->tom, driver->ccxTemp[channelIndex],
                period + 1 /* No compare event */,
                2 /* 1st compare event (issue: expected to be 1)*/ + deadtime);
        }
        else if (x == 0)
        {
            cm0 = 1;
            cm1 = period + 2;
            IfxGtm_Tom_Ch_setCompareShadow(driver->tom, driver->ccxTemp[channelIndex], cm0, cm1);
        }
        else
        {                           /* x% duty cycle */
            s = shift[channelIndex];

            if (s > 0)
            {
                s = __minX(s, (period - x) / 2 - 1);
            }
            else
            {
                s = __maxX(s, (x - period) / 2 + 1);
            }

            cm1 = s + (period - x) / 2; // CM1
            cm0 = s + (period + x) / 2; // CM0
            IfxGtm_Tom_Ch_setCompareShadow(driver->tom, driver->ccxTemp[channelIndex], cm0, cm1 + deadtime);
        }
    }
}

IFX_STATIC IFX_CONST IfxGtm_Tom_PwmHl_Mode IfxGtm_Tom_Pwmccx_modes[2] = {
    {Ifx_Pwm_Mode_centerAligned,         FALSE, &IfxGtm_Tom_PwmCCX_updateCenterAligned, &IfxGtm_Tom_PwmCCX_updateShiftCenterAligned, &IfxGtm_Tom_PwmCCX_updatePulse   },
    {Ifx_Pwm_Mode_off,                   FALSE, &IfxGtm_Tom_PwmCCX_updateOff,           &IfxGtm_Tom_PwmCCX_updateAndShiftOff,        &IfxGtm_Tom_PwmCCX_updatePulseOff}
};


boolean IfxGtm_Tom_Pwmccx_setMode(IfxGtm_Tom_PwmHl *driver, Ifx_Pwm_Mode mode)
{
    boolean                result = TRUE;
    IfxGtm_Tom_PwmHl_Base *base   = &driver->base;

    if (base->mode != mode)
    {
        if (mode > Ifx_Pwm_Mode_off)
        {
            mode   = Ifx_Pwm_Mode_off;
            result = FALSE;
        }

        IFX_ASSERT(IFX_VERBOSE_LEVEL_ERROR, mode == IfxGtm_Tom_PwmHl_modes[mode].mode);

        base->mode             = mode;
        driver->update         = IfxGtm_Tom_Pwmccx_modes[0].update;
        driver->updateAndShift = IfxGtm_Tom_Pwmccx_modes[0].updateAndShift;
        driver->updatePulse    = IfxGtm_Tom_Pwmccx_modes[0].updatePulse;

            driver->ccxTemp   = driver->ccx;

        {   /* Workaround to enable the signal inversion required for center aligned inverted
             * and right aligned modes */
            /** \note that changing signal level may produce short circuit at the power stage,
             * in which case the inverter must be disable during this action. */

            /* Ifx_Pwm_Mode_centerAligned and Ifx_Pwm_Mode_LeftAligned use inverted=FALSE */
            /* Ifx_Pwm_Mode_centerAlignedInverted and Ifx_Pwm_Mode_RightAligned use inverted=TRUE */
            uint32 channelIndex;

            for (channelIndex = 0; channelIndex < driver->base.channelCount; channelIndex++)
            {
                IfxGtm_Tom_Ch channel;

                channel = driver->ccx[channelIndex];
                IfxGtm_Tom_Ch_setSignalLevel(driver->tom, channel, base->ccxActiveState);

            }
        }
    }

    return result;
}





boolean IfxGtm_Tom_Pwmccx_init(IfxGtm_Tom_PwmHl *driver, IfxGtm_Tom_PwmHl_Config *config) {
    boolean           result = TRUE;
    uint16            channelMask;
    uint16            channelsMask = 0;
    uint32            channelIndex;
    uint16            maskShift    = 0;
    IfxGtm_Tom_Timer *timer        = config->timer;

    driver->base.mode             = Ifx_Pwm_Mode_init;
    driver->timer                 = timer;
    driver->base.setMode          = 0;
    driver->base.inverted         = FALSE;
    driver->base.ccxActiveState   = config->base.ccxActiveState;
    driver->base.coutxActiveState = config->base.coutxActiveState;
    driver->base.channelCount     = config->base.channelCount;

    IfxGtm_Tom_PwmHl_setDeadtime(driver, config->base.deadtime);
    IfxGtm_Tom_PwmHl_setMinPulse(driver, config->base.minPulse);

    driver->tom = &(timer->gtm->TOM[config->tom]);

    /* config->ccx[0] is used for the definition of the TGC */
    if (config->ccx[0]->channel <= 7)
    {
        driver->tgc = IfxGtm_Tom_Ch_getTgcPointer(driver->tom, 0);
    }
    else
    {
        driver->tgc = IfxGtm_Tom_Ch_getTgcPointer(driver->tom, 1);
    }

    maskShift = (config->ccx[0]->channel <= 7) ? 0 : 8;

    IFX_ASSERT(IFX_VERBOSE_LEVEL_ERROR, config->base.channelCount <= IFXGTM_TOM_PWMHL_MAX_NUM_CHANNELS);

    IfxGtm_Tom_Ch_ClkSrc clock = IfxGtm_Tom_Ch_getClockSource(timer->tom, timer->timerChannel);

    for (channelIndex = 0; channelIndex < config->base.channelCount; channelIndex++)
    {
        IfxGtm_Tom_Ch channel;
        /* CCX */
        channel                   = config->ccx[channelIndex]->channel;
        driver->ccx[channelIndex] = channel;
        channelMask               = 1 << (channel - maskShift);
        channelsMask             |= channelMask;

        /* Initialize the timer part */
        IfxGtm_Tom_Ch_setClockSource(driver->tom, channel, clock);

        /* Initialize the SOUR reset value and enable the channel */
        IfxGtm_Tom_Ch_setSignalLevel(driver->tom, channel,config->base.ccxActiveState);
        IfxGtm_Tom_Tgc_enableChannels(driver->tgc, channelMask, 0, TRUE); /* Write the SOUR outout with !SL */
        IfxGtm_Tom_Tgc_enableChannelsOutput(driver->tgc, channelMask, 0, TRUE);

        /* Set the SL to the required level for run time */
        IfxGtm_Tom_Ch_setSignalLevel(driver->tom, channel, config->base.ccxActiveState);
        IfxGtm_Tom_Ch_setResetSource(driver->tom, channel, IfxGtm_Tom_Ch_ResetEvent_onTrigger);
        IfxGtm_Tom_Ch_setTriggerOutput(driver->tom, channel, IfxGtm_Tom_Ch_OutputTrigger_forward);
        IfxGtm_Tom_Ch_setCounterValue(driver->tom, channel, IfxGtm_Tom_Timer_getOffset(driver->timer));

        /*Initialize the port */
        if (config->initPins == TRUE)
        {
            IfxGtm_PinMap_setTomTout(config->ccx[channelIndex],
                config->base.outputMode, config->base.outputDriver);
            IfxPort_setPinState(config->ccx[channelIndex]->pin.port, config->ccx[channelIndex]->pin.pinIndex, config->base.ccxActiveState ? IfxPort_State_low : IfxPort_State_high);
        }
    }
    IfxGtm_Tom_Pwmccx_setMode(driver, Ifx_Pwm_Mode_off);

    Ifx_TimerValue tOn[MAX_TOM_CHANNELS] = {0};
    IfxGtm_Tom_PwmCCX_updateOff(driver, tOn);     /* tOn do not need defined values */
    /* Transfer the shadow registers */
    IfxGtm_Tom_Tgc_setChannelsForceUpdate(driver->tgc, channelsMask, 0, 0, 0);
    IfxGtm_Tom_Tgc_trigger(driver->tgc);
    IfxGtm_Tom_Tgc_setChannelsForceUpdate(driver->tgc, 0, channelsMask, 0, 0);

    /* Enable timer to update the channels */
    for (channelIndex = 0; channelIndex < driver->base.channelCount; channelIndex++)
    {
        IfxGtm_Tom_Timer_addToChannelMask(timer, driver->ccx[channelIndex]);
    }

    return result;
}

void InitChannelsPwm(float PWM_FREQ,IfxGtm_Tom tomMaster,IfxGtm_Tom_Ch tomMasterChannel,float32 phase_shift){
    /* Enable the GTM Module */
     IfxGtm_enable(&MODULE_GTM);
     /* Set the GTM global clock frequency in Hz */
     IfxGtm_Cmu_setGclkFrequency(&MODULE_GTM, IfxGtm_Cmu_getModuleFrequency(&MODULE_GTM));
     /* Set the GTM configurable clock frequency in Hz */
     IfxGtm_Cmu_setClkFrequency(&MODULE_GTM, IfxGtm_Cmu_Clk_0, IfxGtm_Cmu_getGclkFrequency(&MODULE_GTM));
     /* Enable the FXU clock                         */
     IfxGtm_Cmu_enableClocks(&MODULE_GTM, IFXGTM_CMU_CLKEN_FXCLK);

     IfxGtm_Tom_Timer_Config timerConfig;                                         /* Timer configuration              */
     {
         IfxGtm_Tom_Timer_initConfig(&timerConfig, &MODULE_GTM);                  /* Initialize default parameters    */

         timerConfig.base.frequency = PWM_FREQ;                                /* Set timer frequency              */
         timerConfig.clock= IfxGtm_Tom_Ch_ClkSrc_cmuFxclk0;                       /* Define the CMU clock used        */
         timerConfig.tom = tomMaster;                                        /* Define the timer used            */
         timerConfig.timerChannel = tomMasterChannel;                      /* Define the channel used          */
     }
         IfxGtm_Tom_Timer_init(&g_pwm3PhaseOutput.timer, &timerConfig);           /* Initialize the TOM               */

         IfxGtm_Tom_PwmHl_Config pwmHlConfig;

         IfxGtm_Tom_ToutMapP ccx[] =
                 {
                     PHASE_U_HS, /* PWM High-side 1 */
                     PHASE_V_HS, /* PWM High-side 2 */
                     PHASE_W_HS  /* PWM High-side 3 */
                 };
         IfxGtm_Tom_PwmHl_initConfig(&pwmHlConfig);
         pwmHlConfig.base.channelCount = sizeof(ccx) / sizeof(IfxGtm_Tom_ToutMapP);

         pwmHlConfig.base.deadtime = phase_shift;

         pwmHlConfig.base.outputMode = IfxPort_OutputMode_pushPull;
         /* Min pulse allowed as active state for the top and bottom PWM in seconds */


         pwmHlConfig.base.outputDriver = IfxPort_PadDriver_cmosAutomotiveSpeed1;

         /* Set top PWM active state */
         pwmHlConfig.base.ccxActiveState = Ifx_ActiveState_high;


         pwmHlConfig.ccx   = ccx;                        /* Assign the channels for High-side switches   */

         pwmHlConfig.timer = &g_pwm3PhaseOutput.timer;
         pwmHlConfig.tom   = timerConfig.tom;

         IfxGtm_Tom_Pwmccx_init(&g_pwm3PhaseOutput.pwm, &pwmHlConfig);

         IfxGtm_Tom_Pwmccx_setMode(&g_pwm3PhaseOutput.pwm, Ifx_Pwm_Mode_centerAligned);
         /* Update the input frequency */
         IfxGtm_Tom_Timer_updateInputFrequency(&g_pwm3PhaseOutput.timer);
         IfxGtm_Tom_Timer_run(&g_pwm3PhaseOutput.timer);
         /* Calculate initial values of PWM duty cycles */
         g_pwm3PhaseOutput.pwmOnTimes[0] = g_pwm3PhaseOutput.pwm.timer->base.period * 0.10;
         g_pwm3PhaseOutput.pwmOnTimes[1] = g_pwm3PhaseOutput.pwm.timer->base.period * 0.20;
         g_pwm3PhaseOutput.pwmOnTimes[2] = g_pwm3PhaseOutput.pwm.timer->base.period * 0.75;
         /* Update PWM duty cycles */
         IfxGtm_Tom_Timer_disableUpdate(&g_pwm3PhaseOutput.timer);
         IfxGtm_Tom_PwmCCX_setOnTime(&g_pwm3PhaseOutput.pwm, g_pwm3PhaseOutput.pwmOnTimes);
         IfxGtm_Tom_Timer_applyUpdate(&g_pwm3PhaseOutput.timer);
}




