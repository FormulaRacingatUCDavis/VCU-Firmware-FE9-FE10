#include "fsm.h"

// Initial FSM state
volatile state_t state = STARTUP;
volatile error_t error = NONE;

/************ States ************/

void change_state(const state_t new_state) {
    // Handle edge cases
    if (state == FAULT && new_state != FAULT) {
        // Reset the error cause when exiting fault state
        error = NONE;
    }
    
    state = new_state;
}

void report_fault(error_t _error) {
    change_state(FAULT);
    // Cause of error
    error = _error;
}

uint8_t inverter_enable() {
    return (state == DRIVE)
        || (error == SENSOR_DISCREPANCY)
        || (error == BRAKE_IMPLAUSIBLE);

}

uint8_t hv_requested(){
    return (state == PRECHARGING)
        || (state == HV_ENABLED)
        || (state == DRIVE)
        || (error == BRAKE_NOT_PRESSED)
        || (error == SENSOR_DISCREPANCY)
        || (error == BRAKE_IMPLAUSIBLE);
}

uint8_t one_byte_state(){
    uint8_t state_byte = state;
    if (state == FAULT) { 
        state_byte = 0b10000000 + error; // greatest bit = 1 if fault 
    }
    
    return state_byte;
}