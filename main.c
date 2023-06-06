/**
  main.c file for FE9 VCU

  @Company
    FRUCD

  @File Name
    main.c

  @Summary
    This is the generated main.c using dsPIC33 MCU.

  @Description
    This source file provides main entry point for system initialization and application code development.
    Generation Information :
        Product Revision  :  PIC24 / dsPIC33 / PIC32MM MCUs - 1.170.0
        Device            :  dsPIC33CH256MP505
    The generated drivers are tested against the following:
        Compiler          :  XC16 v1.61
        MPLAB 	          :  MPLAB X v5.45
*/

/**
  Section: Included Files
*/

#include "main.h"

extern volatile state_t state;
extern volatile error_t error;
extern volatile uint8_t mc_lockout;
extern volatile uint16_t capacitor_volt;
extern volatile uint8_t shutdown_flags;
extern volatile uint8_t switches;
extern volatile uint8_t estop_flags;
extern unsigned int discrepancy_timer_ms;


// Keeps track of timer waiting for pre-charging
unsigned int precharge_timer_ms = 0;

int main(void)
{
    // initialize the device
    SYSTEM_Initialize();

    // Set up ADCC for reading analog signals
    //ADCC_DischargeSampleCapacitor();
    
    can_init();
    init_sensors();
    
    // Set up timer interrupt
    TMR1_SetInterruptHandler(tmr1_ISR);
    TMR1_Start();
    
    while (1) {
        
        // Main FSM
        // Source: https://docs.google.com/document/d/1q0RL4FmDfVuAp6xp9yW7O-vIvnkwoAXWssC3-vBmNGM/edit?usp=sharing
        
        update_sensor_vals();
        
        can_tx_vcu_state();

        if (!mc_lockout) {
            can_tx_torque_request();
        }
        
        // Traction control
        if (traction_control_enable()) {
            traction_control_PID();
        }
        
        // If shutdown circuit opens in any state
        if (!shutdown_closed()) {
            report_fault(SHUTDOWN_CIRCUIT_OPEN);
        }
        
        //if hard BSPD trips in any state
        if (!BSPD_LATCH_GetValue()){
            report_fault(HARD_BSPD);
        }
        
        //clear_screen();
        //print_state();
        //print_pedal_vals();
        gui_dump();
        
        switch (state) {
            case STARTUP: 
                run_calibration();
                
                if (!hv_switch() && !drive_switch()) {
                    change_state(LV);
                }
                break;
            case LV:
                run_calibration();
                
                if (drive_switch()) {
                    // Drive switch should not be enabled during LV
                    report_fault(DRIVE_REQUEST_FROM_LV);
                    break;
                }

                if (hv_switch()) {
                    // HV switch was flipped
                    // check if APPS pedal was calibrated
                    if (sensors_calibrated()) {
                        // Start charging the car to high voltage state
                        change_state(PRECHARGING);
                    } else {
                        report_fault(UNCALIBRATED);
                    }
                } 
                
                break;
            case PRECHARGING:
                if (capacitor_volt > PRECHARGE_THRESHOLD) {
                    // Finished charging to HV on time
                    change_state(HV_ENABLED);
                    break;
                }
                if (!hv_switch()) {
                    // Driver flipped off HV switch
                    change_state(LV);
                    break;
                }
                if (drive_switch()) {
                    // Drive switch should not be enabled during PRECHARGING
                    report_fault(DRIVE_REQUEST_FROM_LV);
                    break;
                }
                break;
            case HV_ENABLED:
                if (!hv_switch()) {// || capacitor_volt < PRECHARGE_THRESHOLD) { // don't really need volt check by rules
                    // Driver flipped off HV switch
                    change_state(LV);
                    break;
                }
                
                if (drive_switch()) {
                    // Driver flipped on drive switch
                    // Need to press on pedal at the same time to go to drive
                    
                    if (brake_mashed()) {
                        change_state(DRIVE);                        
                    } else {
                        // Driver didn't press pedal
                        report_fault(BRAKE_NOT_PRESSED);
                    }
                }
                
                break;
            case DRIVE:
                // CM200 safety feature: starts in lockout mode, disable message must be sent before enable (torque requests)
                if (mc_lockout) {
                    can_tx_disable_MC();
                }
                
                if (!drive_switch()) {
                    // Drive switch was flipped off
                    // Revert to HV
                    change_state(HV_ENABLED);
                   break;
                }

                if (!hv_switch()) {// || capacitor_volt < PRECHARGE_THRESHOLD) { // don't really need volt check by rules || capacitor_volt < PRECHARGE_THRESHOLD) {
                    // HV switched flipped off, so can't drive
                    // or capacitor dropped below threshold
                    report_fault(HV_DISABLED_WHILE_DRIVING);
                    break;
                }
        
                // ***** UNCOMMENT WHEN PEDALS WORK *****
                if (brake_implausible()) {
                    report_fault(BRAKE_IMPLAUSIBLE);
                }
                
                break;
            case FAULT:
                switch (error) {
                    case BRAKE_NOT_PRESSED:    
                        if (!hv_switch())
                            change_state(LV);
                        
                        if (!drive_switch()) {
                            // reset drive switch and try again
                            change_state(HV_ENABLED);
                        }
                        break;
                    case SENSOR_DISCREPANCY:
                        // stop power to motors if discrepancy persists for >100ms
                        // see rule T.4.2.5 in FSAE 2022 rulebook
                        if (!drive_switch()) {
                            discrepancy_timer_ms = 0;
                            change_state(HV_ENABLED);
                        }
                        
                        if (!hv_switch())
                            report_fault(HV_DISABLED_WHILE_DRIVING);
                        
                        break;
                    case BRAKE_IMPLAUSIBLE:
                        if (!brake_implausible() && hv_switch() && drive_switch()) 
                            change_state(DRIVE);
                        
                        if (!hv_switch() && !drive_switch()) 
                            change_state(LV);
                        
                        if (!drive_switch()) 
                            change_state(HV_ENABLED);
                          
                        if (!hv_switch())
                            report_fault(HV_DISABLED_WHILE_DRIVING);
                     
                        break;
                    case SHUTDOWN_CIRCUIT_OPEN:
                        if (shutdown_closed()) {
                            change_state(LV);
                        } 
                        break;
                    case HARD_BSPD:
                        //should not be recoverable, but let hardware decide this
                        if (BSPD_LATCH_GetValue())
                            change_state(LV);
                        
                    default:  //UNCALIBRATED, DRIVE_REQUEST_FROM_LV, CONSERVATIVE_TIMER_MAXED, HV_DISABLED_WHILE_DRIVING, MC fault
                        if (!hv_switch() && !drive_switch()) {
                            change_state(LV);
                        }
                        break;
                }
                break;
        }
        
    }

}



// runs every 20ms; used for precharge timeout and sensor discrepancy
void tmr1_ISR() {
    // CAN receive done here to dodge __delay_ms() latency in main()
    can_receive();
    
    if (state == PRECHARGING) {
        precharge_timer_ms += TMR1_PERIOD_MS;
        if (precharge_timer_ms > PRECHARGE_TIMEOUT_MS) {
            report_fault(CONSERVATIVE_TIMER_MAXED);
        }
    } else {
        precharge_timer_ms = 0;
    }   
}


/************ Switches ************/

/*
 * global variable for driver switch and button status
 * 
 * format: 0000 0[TC][Hv][Dr]
 * bit is high if corresponding switch is on, low otherwise
 * 
 * use in conditions:
 *  - TC = switches & 0b100
 *  - Hv = switches & 0b10
 *  - Dr = switches & 0b1
 */

uint8_t traction_control_enable() {
    return switches & 0b100;
}

uint8_t hv_switch() {
    return switches & 0b10;
}

uint8_t drive_switch() {
    return switches & 0b1;
}


uint8_t shutdown_closed() {
    if (estop_flags) return 0;
    return (shutdown_flags & 0b00111000) == 0b00111000;
}



/**
 End of File
*/

