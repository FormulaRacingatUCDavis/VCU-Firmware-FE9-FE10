/* 
 * File:   fsm.h
 *
 */

#ifndef FSM_H
#define	FSM_H

#include "stdint.h"

/************ States ************/ 

typedef enum {
    LV,
    PRECHARGING,
    HV_ENABLED,
    DRIVE,
    FAULT, 
    STARTUP
} state_t;

typedef enum {
    NONE,
    DRIVE_REQUEST_FROM_LV,
    CONSERVATIVE_TIMER_MAXED,
    BRAKE_NOT_PRESSED,
    HV_DISABLED_WHILE_DRIVING,
    SENSOR_DISCREPANCY,
    BRAKE_IMPLAUSIBLE,
    SHUTDOWN_CIRCUIT_OPEN,
    UNCALIBRATED,
    HARD_BSPD,
    MC_FAULT
} error_t;


void change_state(const state_t new_state);
void report_fault(error_t _error);
uint8_t hv_requested();
uint8_t inverter_enable();
uint8_t one_byte_state();

#endif	/* FSM_H */
