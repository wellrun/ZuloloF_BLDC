/******************************************************************************
 * @file     BLDCSensorLess.c
 * @version  V1.00
 * $Revision: 1 $
 * $Date: 14/12/10 21:13p $ 
 * @brief    BLDC Sensor Less controller for Mini51 series MCU
 *
 * @note
 * Copyright (C) 2014 Zulolo Technology Corp. All rights reserved.
*****************************************************************************/
#define __USED_BY_BLDC_SENSOR_LESS_C__
#include "BLDCSensorLess.h"

/* After change phase and wait a filter time,
   the zero cross detect window will be opened.
   1. In this window the comparator's interrupt will be enabled.
   2. The first ACMP INT will start TIM1 and enable its interrupt.
   3. TIM1 INT will be set as current time + filter time (100us).
   4. Each new ACMP INT will reset TIM1 INT as current time + filter time (100us).
   5. So when TIM1 INT really happened, it means ACMP was stable and really zero crossed.                                                            
   6. In the TIM1 INT we set TIM0's phase change time.*/

/* Some limitation:
   1. Because the phase duration variable is uint16_t, max one phase is 32ms,
	  so min rotation speed is about 180RMP (12 e-phase).*/

__INLINE void PhaseChangedRoutine(void)
{
	FLAG_PHASE_CHANGED = RESET;
	tMotor.structMotor.unPHASE_CHANGE_CNT++;
	
	if (TRUE == tMotor.structMotor.MSR.bZeroCrossDetecting)
	{
//		iPhaseChangeTime = TIMER_GetCounter(TIMER1);
		// Miss ZXD or ZXD success filter
		// If continuously detected more than MIN_SUCC_ZXD_THRESHOLD ZX, OK! GOOD!!
		if (TRUE == tMotor.structMotor.MSR.bThisPhaseDetectedZX)
		{
			tMotor.structMotor.MSR.unMissedZXD_CNT = 0;

			if (tMotor.structMotor.MSR.unSuccessZXD_CNT > MIN_SUCC_ZXD_THRESHOLD)
			{
				tMotor.structMotor.MSR.bLocked = TRUE;
//				BRG_DISABLE;
//				P50 = 1;
//				BLDC_stopMotor();

//				iTestZXContinueCNT++;
//				iTestZXDPeriod = tMotor.structMotor.ACT_PERIOD;
			}
			else
			{
				tMotor.structMotor.MSR.unSuccessZXD_CNT++;
			}
		}
		else	// If continuously missing detected more than MAX_MISS_ZXD_THRESHOLD ZX, loss lock
		{
			tMotor.structMotor.MSR.unSuccessZXD_CNT = 0;
			// If ZX was not detected in last phase, unLastZXDetectedTime was also not updated
			// Guess one value
			unLastZXDetectedTime = GET_TIMER_DIFF((tMotor.structMotor.unACT_PERIOD >> 2), TIMER_GetCounter(TIMER1));
			if (tMotor.structMotor.MSR.unMissedZXD_CNT > MAX_MISS_ZXD_THRESHOLD)
			{
				if (TRUE == tMotor.structMotor.MSR.bLocked)
				{	
					tMotor.structMotor.MSR.bLocked = FALSE;
					MOTOR_SHUT_DOWN;
					setError(ERR_INTERNAL);
				}
			}
			else
			{
				tMotor.structMotor.MSR.unMissedZXD_CNT++;
			}
		}

	}

	if (TRUE == tMotor.structMotor.MSR.bLocked)
	{
		// Set a rough next phase change time as the same with last phase
		// After detected ZX in TIM1 interrupt, next phase change time will be re-configured
		TIMER_SET_CMP_VALUE(TIMER0, tMotor.structMotor.unACT_PERIOD << 1);
	}

	tMotor.structMotor.MSR.bThisPhaseDetectedZX = FALSE;
	// For debug
	GPIO_TOGGLE(P50);
}

/* return STATUS_WORKING: still detecting
   0: no rotation detected
   1-65534: phase time */
uint16_t canMotorContinueRunning(void)
{
	uint16_t unPhaseDuration = 0;
	static uint32_t unStateEnterTime;
// Later implement this when motor can rotate
// Then stop it while rotating to measure the waveform
// Manually rotate it is too slow 
	return 0;

	if ((uint32_t)(unSystemTick - unRotateDetectStartTime) > MAX_ALREADY_ROTATING_DETECT_TIME)
	{
		return 0;
	}
	switch (tRotateDetectState)
	{
	case DETECT_START: 
		unStateEnterTime = unSystemTick;
		tRotateDetectState = DETECT_PHASE_1_P;
		break;

	case DETECT_PHASE_1_P:
		if ((uint32_t)(unSystemTick - unStateEnterTime) > MAX_ROTATING_DETECT_PHASE_TIME)
		{
			return (uint16_t)0;
		}
		else
		{

		}
		break; 

	case DETECT_PHASE_1_A:

		break;

	case DETECT_PHASE_2_P:

		break;

	case DETECT_PHASE_2_A:

		break;

	case DETECT_PHASE_3_P:

		break;

	case DETECT_PHASE_3_A:

		break;

	default:
		break;
	}

	return unPhaseDuration;
}

// Mainly PWM duty increase/decrease
void BLDCSpeedManager(void)
{
	if (SET == FLAG_PHASE_CHANGED)
	{
		PhaseChangedRoutine();

		if (tMotor.structMotor.unACT_DUTY != tMotor.structMotor.unTGT_DUTY)
		{
//				tMotor.structMotor.ACT_DUTY = tMotor.structMotor.TGT_DUTY;
				// Change PWM duty after each x phase change
			if (unPhaseChangeCNT4Duty > CHANGE_DUTY_CNT_THR)
			{
				unPhaseChangeCNT4Duty = 0;
				if (tMotor.structMotor.unACT_DUTY < tMotor.structMotor.unTGT_DUTY)
				{
					tMotor.structMotor.unACT_DUTY++;
				}
				else
				{
					tMotor.structMotor.unACT_DUTY--;
				}
				MOTOR_SET_DUTY(tMotor.structMotor.unACT_DUTY);
			}
			unPhaseChangeCNT4Duty++;
		}
		
		PHASE_INCREASE(unCurrentPhase);
		// Modify PWM->PHCHGNXT at last because I don't know how long needed to reload PHCH with PHCHNEXT after TIM0 time-out
		PWM->PHCHGNXT = GET_PHASE_VALUE(unCurrentPhase);
	}
}
//
//void BLDCStarterManager(void)
//{
//
//}
//
//void BLDCPhaseChangeManager(void)
//{
//
//}
__INLINE void BLDC_stopMotor(void)
{
	MOTOR_SHUT_DOWN;
	tMotor.structMotor.MCR.bMotorNeedToRun = FALSE;
	tMotor.structMotor.MSR.bMotorPowerOn = FALSE;
	tMotorState = MOTOR_IDLE;
}

__INLINE void setPhaseManually(uint16_t iPWMDuty, uint8_t iPhase)
{
    MOTOR_SET_DUTY(iPWMDuty);
	PWM->PHCHG = GET_PHASE_VALUE(iPhase);
}

ENUM_STATUS BLDCLocatingManager(void)
{
	if ((uint32_t)(unSystemTick - unLastPhaseChangeTime) > tMotor.structMotor.unLCT_PERIOD)
	{
		if (unLocateIndex < (sizeof(unLocatePhaseSequencyTable)/sizeof(uint8_t)))
		{
			//iLastPhaseChangeTime = unSystemTick; 
			setPhaseManually(tMotor.structMotor.unLCT_DUTY, unLocatePhaseSequencyTable[unLocateIndex]);
			unLocateIndex++;
		}
		else
		{
			MOTOR_SHUT_DOWN;
			tMotor.structMotor.MSR.bMotorPowerOn = FALSE;
			unCurrentPhase = unLocatePhaseSequencyTable[unLocateIndex - 1];
			return STATUS_FINISHED;
		}
	}
	return STATUS_WORKING;
}

__INLINE void BLDCRampUp_Manager(void)
{
	if (SET == FLAG_PHASE_CHANGED)
	{
		PhaseChangedRoutine();
		if (unPhaseChangeCNT4Period > CHANGE_DUTY_PERIOD_THR)
		{
			unPhaseChangeCNT4Period = 0;
			// Change duty and period 
//			MOTOR_RAMPUP_DT_INCR(tMotor.structMotor.ACT_DUTY);			
			MOTOR_RAMPUP_PR_DCR(tMotor.structMotor.unACT_PERIOD);	
			if (tMotor.structMotor.unACT_PERIOD <= MOTOR_RAMPUP_PR_MIN)
			{
				unRampUpPeriodMiniCNT++;
			}
		}
		unPhaseChangeCNT4Period++;
//		MOTOR_SET_DUTY(tMotor.structMotor.ACT_DUTY);
		TIMER_SET_CMP_VALUE(TIMER0, tMotor.structMotor.unACT_PERIOD);
		PHASE_INCREASE(unCurrentPhase);
		// Modify PWM->PHCHGNXT at last because I don't know how long needed to reload PHCH with PHCHNEXT after TIM0 time-out
		PWM->PHCHGNXT = GET_PHASE_VALUE(unCurrentPhase);
	}
}

// Take charge of all Motot control
void BLDC_SensorLessManager(void)
{
	uint16_t iMotorAlreadyRotatingPhaseTime;
	static uint32_t iEnterTimeBeforeWait;

	// Duty too big protection
	if ((tMotor.structMotor.unACT_DUTY > MAX_MOTOR_PWR_DUTY) || (PWM->CMR[1] > MAX_MOTOR_PWR_DUTY))
	{
		BLDC_stopMotor();
		setError(ERR_INTERNAL);
	}

	// Single phase duration too long protection 
	if (TRUE == tMotor.structMotor.MSR.bMotorPowerOn)
	{
		if (unCurrentPHCHG != PWM->PHCHG)
		{
			unCurrentPHCHG = PWM->PHCHG;
			unLastPhaseChangeTime = unSystemTick;
		}
		else
		{
			if ((uint32_t)(unSystemTick - unLastPhaseChangeTime) > MAX_SINGLE_PHASE_DURATION) 
			{
				BLDC_stopMotor();
				setError(ERR_INTERNAL);
			}
		}
	}

	switch (tMotorState)
	{
	case MOTOR_IDLE:
		if (tMotor.structMotor.MCR.bMotorNeedToRun && NO_MOTOR_EEROR)
		{
			unRotateDetectStartTime = unSystemTick;
			tRotateDetectState = DETECT_START;
			tMotorState = MOTOR_START;
		}
		break;
		
	case MOTOR_START:
		if (tMotor.structMotor.MCR.bMotorNeedToRun && NO_MOTOR_EEROR)
		{
			// Later implement this when motor can rotate
			// Then stop it while rotating to measure the waveform
			// Manually rotate it is too slow 
			iMotorAlreadyRotatingPhaseTime = canMotorContinueRunning();
			if (iMotorAlreadyRotatingPhaseTime != ALREADY_ROTATING_DETECTING)
			{
				if (iMotorAlreadyRotatingPhaseTime)
				{
					// 1 to 65534
					tMotorState = MOTOR_LOCKED;
				}
				else
				{
					// When back to Idle state the motor was already shut down
					// MOTOR_SHUT_DOWN;
					unCurrentPhase = 0;
					unLocateIndex = 0;
					tMotor.structMotor.MSR.unMissedZXD_CNT = 0;
					unLastPhaseChangeTime = unSystemTick;
					tMotor.structMotor.MSR.bMotorPowerOn = TRUE;
					// Clear start detect zero cross flag
					tMotor.structMotor.MSR.bZeroCrossDetecting = FALSE;
					tMotor.structMotor.MSR.bLocked = FALSE;
					//setPhaseManually(tMotor.structMotor.LCT_DUTY, unCurrentPhase);
					BRG_ENABLE;
					tMotorState = MOTOR_LOCATE;
				}
			}
		}
		else
		{
			BLDC_stopMotor();
		}
		break;

	case MOTOR_LOCATE:
		if (tMotor.structMotor.MCR.bMotorNeedToRun && NO_MOTOR_EEROR)
		{
			if (BLDCLocatingManager() == STATUS_FINISHED)
			{
				iEnterTimeBeforeWait = unSystemTick;
				tMotorState = MOTOR_WAIT_AFTER_LOCATE;
			}
		}
		else
		{
			BLDC_stopMotor();
		}
		break;

	case MOTOR_WAIT_AFTER_LOCATE:
		if (tMotor.structMotor.MCR.bMotorNeedToRun && NO_MOTOR_EEROR)
		{
			if ((uint32_t)(unSystemTick - iEnterTimeBeforeWait) >= WAIT_AFTER_LOCATE_TIME)
			{
				tMotor.structMotor.unACT_DUTY = tMotor.structMotor.unRU_DUTY;
				tMotor.structMotor.unACT_PERIOD = tMotor.structMotor.unRU_PERIOD;
				tMotor.structMotor.MSR.bMotorPowerOn = TRUE;
				PHASE_INCREASE(unCurrentPhase);
				setPhaseManually(tMotor.structMotor.unACT_DUTY, unCurrentPhase);
				BRG_ENABLE;
				// Set timer 0 valure, use timer 0 to change phase automatically
				// ************************************************************************
				// ----==== From here current unCurrentPhase is actually next phase ====----
				// What to get real current phase value? Read PWM->PHCHG.
				// ************************************************************************
				PHASE_INCREASE(unCurrentPhase);
				PWM->PHCHGNXT = GET_PHASE_VALUE(unCurrentPhase);
				// !!!! Need to make sure CPU run to here every min tMotor.structMotor.ACT_PERIOD time !!!
				// !!!! If not , timer counter may already passed tMotor.structMotor.ACT_PERIOD, !!!!
				// !!!! then need to count to 2^24, go back to 0 and triger interrupt when reach ACT_PERIOD !!!!
				TIMER_SET_CMP_VALUE(TIMER0, tMotor.structMotor.unACT_PERIOD);
				TIMER_Start(TIMER0);	// Once started, running and interrupting until Motor stop
				TIMER_EnableInt(TIMER0);
				unRampUpPeriodMiniCNT = 0;
				unPhaseChangeCNT4Duty = 0;
				unPhaseChangeCNT4Period = 0;
				tMotorState = MOTOR_RAMPUP_WO_ZXD;
			}
		}
		else
		{
			BLDC_stopMotor();
		}
		break;

	case MOTOR_RAMPUP_WO_ZXD:	// without zero cross detection
		if (tMotor.structMotor.MCR.bMotorNeedToRun && NO_MOTOR_EEROR)
		{
			BLDCRampUp_Manager();
			if (tMotor.structMotor.unACT_PERIOD <= MOTOR_START_ZXD_SPEED)	//(iRampUpPeriodMiniCNT > MOTOR_START_ZXD_MINROT_CNT)  //
			{
				tMotor.structMotor.MSR.bThisPhaseDetectedZX = FALSE;
				tMotor.structMotor.MSR.bZeroCrossDetecting = TRUE;
				// Speed is enough for zero cross detecting
				// Prepare everything
				// T0 used to change phase automatically -- already configured
				// T1 used to filter ZX
				//ACMP->CMPCR[0]

//				TIMER_SET_CMP_VALUE(TIMER1, GET_TIM1_CMP_VALUE(TIMER1->TDR + AVOID_ZXD_AFTER_PHCHG));
//				FLAG_TIM1_USEAGE = ENUM_TIM1_AVOID_ZXD;
				ACMP0_ENABLE;
				TIMER_Start(TIMER1);	// Once started, running until Motor stop
//				TIMER_EnableInt(TIMER1);
				// Suppose last ZX detected time 
//				unLastZXDetectedTime = MINI51_TIM_CNT_MAX - tMotor.structMotor.ACT_PERIOD / 2;
				tMotorState = MOTOR_RAMPUP_W_ZXD;
			}
		}
		else
		{
			BLDC_stopMotor();
		}
		break;

	case MOTOR_RAMPUP_W_ZXD:	// with zero cross detection
		if (tMotor.structMotor.MCR.bMotorNeedToRun && NO_MOTOR_EEROR)
		{
			if (TRUE == tMotor.structMotor.MSR.bLocked)
			{
				// Finally, everything was prepared:
				// T0 used to change phase automatically
				// T1 used to filter ZX
				tMotorState = MOTOR_LOCKED;
			}
			else
			{
				if (unRampUpPeriodMiniCNT < RAMP_UP_MIN_PERIOD_NUM_THRS)
				{
					BLDCRampUp_Manager(); 
				}
				else
				{
					setError(ERR_RAMPUP_FAIL);
				}
			}
		}
		else
		{
			BLDC_stopMotor();
		}
		break;

	case MOTOR_LOCKED:
		if (tMotor.structMotor.MCR.bMotorNeedToRun && NO_MOTOR_EEROR)
		{
			BLDCSpeedManager();	// Mainly PWM duty increase/decrease
		}
		else
		{
			BLDC_stopMotor();
		}
		break;

	default:
		break;
	}
}

