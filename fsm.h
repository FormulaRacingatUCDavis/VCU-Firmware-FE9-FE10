/* 
 * File:   fsm.h
 *
 */

#ifndef FSM_H
#define	FSM_H

#ifdef	__cplusplus
extern "C" {
#endif




#ifdef	__cplusplus
}
#endif

#endif	/* FSM_H */

/************ States ************/ 

typedef enum {
    LV,
    PRECHARGING,
    HV_ENABLED,
    DRIVE,
    FAULT
} state_t;

typedef enum {
    NONE,
    DRIVE_REQUEST_FROM_LV,
    CONSERVATIVE_TIMER_MAXED,
    BRAKE_NOT_PRESSED,
    HV_DISABLED_WHILE_DRIVING,
    SENSOR_DISCREPANCY,
    BRAKE_IMPLAUSIBLE,
    ESTOP,
    SHUTDOWN_CIRCUIT_OPEN
} error_t;

// Initial FSM state
volatile state_t state = LV;
volatile error_t error = NONE;
// 8-bit encoding of state to be put on CAN
volatile uint8_t state_msg_byte = LV;

// true if state/error should be requesting HV
volatile uint8_t hv_requested = 0;