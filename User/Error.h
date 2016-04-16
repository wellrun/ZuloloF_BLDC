#ifndef __ERROR_H__
#define __ERROR_H__

#include "global.h"

#ifdef __USED_BY_ERROR_C__
	#define EXTERNAL_ERROR  

	#define LED_ON					(P54 = 0)
	#define LED_OFF 				(P54 = 1)
	#define LED_TOGGLE 				(GPIO_TOGGLE(P54))
	#define LED_PATTERN_INTERVAL	4000  //4s
	#define LED_BLINK_ON_TIME		200  //0.2s
	#define LED_BLINK_OFF_TIME		200  //0.2s, so 6 times is (0.4x6 = 2.4s)
	#define LED_BLINK_INTERVAL		(LED_BLINK_ON_TIME + LED_BLINK_OFF_TIME)

#else 
	#define EXTERNAL_ERROR extern
#endif

EXTERNAL_ERROR uint32_t iErrorMaster;
EXTERNAL_ERROR void clearError(ENUM_ERROR_LEVEL enumErrorType);
EXTERNAL_ERROR void setError(ENUM_ERROR_LEVEL enumErrorType);
#define GET_SPECIFIED_EEROR(x)		((iErrorMaster & (1L << (x)) == 0) ? FALSE : TRUE)
#define IS_ANY_EEROR				((iErrorMaster == 0) ? FALSE : TRUE)
/*
	ERR_NULL = 0,
	ERR_CURRENT_WARNING,	// Current > 5A
	ERR_LOCATE_FAIL,
	ERR_RAMPUP_FAIL,
	ERR_BATTERY_LOW,
	ERR_INTERNAL,	// Loss lock, single phase duration too long
	ERR_CURRENT_BURNING,	// Current > 15A
    ERR_BRD_FAULT
    */
#define MOTOR_ERROR_MSK				0xFFFFFFFEul
#define NO_MOTOR_EEROR				(((iErrorMaster & MOTOR_ERROR_MSK) == 0) ? TRUE : FALSE)
#endif