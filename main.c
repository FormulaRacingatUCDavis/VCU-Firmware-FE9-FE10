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

#ifndef _XTAL_FREQ
#define _XTAL_FREQ  8000000UL
#endif

#ifndef FCY
#define FCY 8000000UL
#endif

#include "mcc_generated_files/mcc.h"
#include "mcc_generated_files/system.h"
#include <libpic30.h>

// general includes
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdio.h>

// project includes
#include "can_manager.h"
#include "fsm.h"


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


/************ Timer ************/

// How long to wait for pre-charging to finish before timing out
#define PRECHARGE_TIMEOUT_MS 5000
// Keeps track of timer waiting for pre-charging
unsigned int precharge_timer_ms = 0;
// Delay between checking pre-charging state
#define TMR1_PERIOD_MS 20
// discrepancy timer
#define MAX_DISCREPANCY_MS 100 
unsigned int discrepancy_timer_ms = 0;


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
 * global variable for driver switch status
 * 
 * format: 0000 00[Hv][Dr]
 * bit is high if corresponding switch is on, low otherwise
 * 
 * use in conditions:
 *  - Hv = switches & 0b10
 *  - Dr = switches & 0b1
 */
volatile uint8_t switches = 0;

uint8_t hv_switch() {
    return switches & 0b10;
}

uint8_t drive_switch() {
    return switches & 0b1;
}

/************ Pedals ************/

// APPS

volatile double THROTTLE_MULTIPLIER = 1;
volatile uint8_t PACK_TEMP;

const double THROTTLE_MAP[8] = { 95, 71, 59, 47, 35, 23, 11, 5 };

void temp_attenuate() {
    int t = PACK_TEMP - 50;
    if (t < 0) {
        THROTTLE_MULTIPLIER = 1;   
    } else if (t < 8) {
        THROTTLE_MULTIPLIER = THROTTLE_MAP[t] / 100.0; 
    } else if (t >= 8) {
        THROTTLE_MULTIPLIER = THROTTLE_MAP[7] / 100.0; 
    }
}

volatile uint32_t throttle_sent = 0;
volatile uint16_t throttle1 = 0;
volatile uint16_t throttle2 = 0;
volatile uint16_t per_throttle1 = 0;
volatile uint16_t per_throttle2 = 0;
uint16_t throttle1_max = 0;
uint16_t throttle1_min = 0x7FFF;
uint16_t throttle2_max = 0;
uint16_t throttle2_min = 0x7FFF;
uint16_t throttle_range = 0; // set after max and min values are calibrated


// Brake

// There is some noise when reading from the brake pedal
// So give some room for error when driver presses on brake
#define BRAKE_ERROR_TOLERANCE 50

volatile uint16_t brake1 = 0;
uint16_t brake1_max = 0;
uint16_t brake1_min = 0x7FFF;
uint16_t brake1_range = 0;

volatile uint16_t brake2 = 0;
uint16_t brake2_max = 0;
uint16_t brake2_min = 0x7FFF;
uint16_t brake2_range = 0;


// check differential between the throttle sensors
// returns true only if the sensor discrepancy is > 10%
// Note: after verifying there's no discrepancy, can use either sensor(1 or 2) for remaining checks
bool has_discrepancy() {
    // calculate percentage of throttle 1
    uint16_t temp_throttle1 = throttle1;
    if (temp_throttle1 > throttle1_max) {
        temp_throttle1 = throttle1_max;
    } else if (temp_throttle1 < throttle1_min) {
        temp_throttle1 = throttle1_min;
    }
    per_throttle1 = (uint16_t)(((double)(temp_throttle1-throttle1_min) / (throttle1_max - throttle1_min)) * 100);
    
    // calculate percentage of throttle 2
    uint16_t temp_throttle2 = throttle2;
    if (temp_throttle2 > throttle2_max) {
        temp_throttle2 = throttle2_max;
    } else if( temp_throttle2 < throttle2_min) {
        temp_throttle2 = throttle2_min;
    }
    per_throttle2 = (uint16_t)(((double)(temp_throttle2-throttle2_min) / (throttle2_max - throttle2_min)) * 100);
    
    return abs((int)per_throttle1 - (int)per_throttle2) > 10;
}

// check for soft BSPD
// see EV.5.7 of FSAE 2022 rulebook
bool brake_implausible() {
    uint16_t temp_throttle = throttle1 - throttle1_min; 
    
    // subtract dead zone 15%
    uint16_t temp_brake = brake1 - ((brake1_range)/6);
    if (temp_brake > brake1_max){
        temp_brake = brake1_max;
    }
    if (temp_brake < brake1_min){
        temp_brake = brake1_min;
    }
    temp_brake = (uint16_t)((temp_brake-brake1_min)*100.0 / brake1_range);
    
    
    if (error == BRAKE_IMPLAUSIBLE) {
        // once brake implausibility detected,
        // can only revert to normal if throttle unapplied
        return !(temp_throttle < throttle_range * 0.05);
    }
    else {
        // if both brake and throttle applied, brake implausible
        //return (temp_brake > 0 && temp_throttle > throttle_range * 0.25);
        return (brake1 >= 500 && temp_throttle > throttle_range * 0.25);
    }
    
}

uint16_t getConversion(ADC1_CHANNEL channel){
    uint16_t conversion;
    ADC1_Enable();
    ADC1_ChannelSelect(channel);
    ADC1_SoftwareTriggerEnable();
    //Provide Delay
    __delay_ms(1);
    ADC1_SoftwareTriggerDisable();
    while(!ADC1_IsConversionComplete(channel));
    conversion = ADC1_ConversionResultGet(channel);
    ADC1_Disable();
    return conversion;
}

// Update sensors

// On the breadboard, the range of values for the potentiometer is 0 to 4095
#define PEDAL_MAX 4095

void run_calibration() {
    // APPS1 = pin8 = RA0
    throttle1 = getConversion(APPS1);
    // APPS2 = pin9 = RA1
    throttle2 = getConversion(APPS2);
    // BSE1 = pin11 = RA3
    brake1 = getConversion(BSE1);
    // BSE2 = pin12 = RA4
    // = getConversion(BSE2);

    if (throttle1 > throttle1_max) {
        throttle1_max = throttle1;
    }
    if (throttle1 < throttle1_min) {
        throttle1_min = throttle1;
    }
    if (throttle2 > throttle2_max) {
        throttle2_max = throttle2;
    }
    if (throttle2 < throttle2_min) {
        throttle2_min = throttle2;    
    }

    if (brake1 > brake1_max) {
        brake1_max = brake1;
    }
    if (brake1 < brake1_min) {
        brake1_min = brake1;
    }
    throttle_range = throttle1_max - throttle1_min;
    brake1_range = brake1_max - brake1_min;
}

// storage variables used to return to previous state when discrepancy is resolved
volatile state_t temp_state = LV; // state before sensor discrepancy error
volatile error_t temp_error = NONE; // error state before sensor discrepancy error (only used when going from one fault to discrepancy fault)

void update_sensor_vals() {
    // APPS1 = pin8 = RA0
    throttle1 = getConversion(APPS1);
    // APPS2 = pin9 = RA1
    throttle2 = getConversion(APPS2);
    // BSE1 = pin11 = RA3
    brake1 = getConversion(BSE1);
    // BSE2 = pin12 = RA4
    //brake2 = getConversion(BSE2);

    /* 
     * T.4.2.5 in FSAE 2022 rulebook
     * If an implausibility occurs between the values of the APPSs and
     * persists for more than 100 msec, the power to the Motor(s) must
     * be immediately stopped completely.
     * 
     * It is not necessary to Open the Shutdown Circuit, the motor
     * controller(s) stopping the power to the Motor(s) is sufficient.
     */
    if (has_discrepancy()) {
        discrepancy_timer_ms += TMR1_PERIOD_MS;
        if (discrepancy_timer_ms > MAX_DISCREPANCY_MS && state == DRIVE) {
            temp_state = state;
            temp_error = error;
            report_fault(SENSOR_DISCREPANCY);
        }
    } else {
        discrepancy_timer_ms = 0;
    }
}


/************ Capacitor ************/

volatile uint16_t capacitor_volt = 0;
#define PRECHARGE_THRESHOLD 4976 // 77V = 90% of nominal accumulator voltage, scaled from range of 0-12800(0V-200V)


/************ Shutdown Circuit ************/
volatile uint8_t shutdown_flags = 0;


/************ CAN ************/

void can_receive() {
    // gets message and updates values
    if (CAN1_Receive(&msg_RX)) {
        switch (msg_RX.msgId) {
            case DRIVER_SWITCHES:
                switches = msg_RX.data[0]; 
                break;
            case BMS_TEMPERATURES:
                PACK_TEMP = msg_RX.data[7];
                temp_attenuate();
                break;
            case MC_ESTOP:
                if (msg_RX.data[0]) { // if estop detected in any state
                    report_fault(ESTOP);
                }
            case MC_PDO_SEND:
                capacitor_volt = (msg_RX.data[0] << 8); // upper bits
                capacitor_volt += msg_RX.data[1]; // lower bits
            case PEI_CURRENT_SHUTDOWN: 
                shutdown_flags = msg_RX.data[2];
            default:
                // no valid input received
                break;
        }
    } 
}


/*
            Main application
 */
    
int main(void)
{
    // initialize the device
    SYSTEM_Initialize();

    // Set up ADCC for reading analog signals
    //ADCC_DischargeSampleCapacitor();
    
    // Set up CAN
    CAN1_OperationModeSet(CAN_CONFIGURATION_MODE);
    if(CAN_CONFIGURATION_MODE == CAN1_OperationModeGet()) {
        if(CAN_OP_MODE_REQUEST_SUCCESS == CAN1_OperationModeSet(CAN_NORMAL_2_0_MODE)) {
            // CAN set up successful
        }
    }
    
    // Set up timer interrupt
    TMR1_SetInterruptHandler(tmr1_ISR);
    TMR1_Start();
    
    while (1) {
        // Main FSM
        // Source: https://docs.google.com/document/d/1q0RL4FmDfVuAp6xp9yW7O-vIvnkwoAXWssC3-vBmNGM/edit?usp=sharing
        
        // CAN transmit state
        state_msg_byte = state; 
        if (state == FAULT) { 
            state_msg_byte = 0b10000000 + error; // greatest bit = 1 if fault 
        }

        //  CAN transmit torque request command
        hv_requested = (state == PRECHARGING)
                    || (state == HV_ENABLED)
                    || (state == DRIVE)
                    || (error == BRAKE_NOT_PRESSED)
                    || (error == SENSOR_DISCREPANCY)
                    || (error == BRAKE_IMPLAUSIBLE);
        update_sensor_vals();
        if (state == DRIVE) {
            // check bounds
            if (throttle1 > throttle1_max) {
                throttle1 = throttle1_max;
            } else if (throttle1 < throttle1_min) {
                throttle1 = throttle1_min;
            }
            throttle_sent = (uint32_t)(throttle1 - throttle1_min);
            throttle_sent = (throttle_sent * 0x7fff) / throttle_range;
            throttle_sent = (uint32_t)(throttle_sent * THROTTLE_MULTIPLIER);
        } else {
            throttle_sent = 0;
        }
        uint8_t data_TX_torque[6] = {
            hv_requested,
            (uint8_t)(throttle_sent >> 8), // torque request upper
            (uint8_t)(throttle_sent & 0xff), // torque request lower
            (error != ESTOP),
            state_msg_byte,
            (brake1 >= 400)
        };

        msg_TX_torque.field = field_TX_torque; 
        msg_TX_torque.data = data_TX_torque;
        CAN1_Transmit(CAN1_TX_TXQ, &msg_TX_torque);
        
//        __delay_ms(50);
//        
//        // CAN transmit BSPD flags (DEBUG)
//        uint8_t data_TX_bspd_flags[6] = {
//            BSPD_LATCH_GetValue() << 5,
//            (uint8_t)(throttle1 >> 8), // throttle1 upper
//            (uint8_t)(throttle2 >> 8), // throttle2 upper
//            (uint8_t)(brake1 >> 8), // brake1 upper
//            0//(uint8_t)(brake2 >> 8), // brake2 upper
//        };
//        data_TX_bspd_flags[0] |= BSPD_TRIPPED_GetValue() << 4;
//        data_TX_bspd_flags[0] |= OPEN_SHORT_OK_GetValue() << 3;
//        data_TX_bspd_flags[0] |= B_OR_5K_TRIPPED_GetValue() << 2;
//        data_TX_bspd_flags[0] |= B_TRIPPED_GetValue() << 1;
//        data_TX_bspd_flags[0] |= K5_TRIPPED_GetValue();
//                
//        msg_TX_bspd_flags.field = field_TX_bspd_flags; 
//        msg_TX_bspd_flags.data = data_TX_bspd_flags;
//        CAN1_Transmit(CAN1_TX_TXQ, &msg_TX_bspd_flags);
//        
//        __delay_ms(50);
        
        // If shutdown circuit opens in any state
        if (shutdown_flags != 255) {
            change_state(SHUTDOWN_CIRCUIT_OPEN);
        }
        
        switch (state) {
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
                    if (throttle_range > 0xD00) {
                        // Start charging the car to high voltage state
                        change_state(PRECHARGING);
                    } else {
                        // ***** RENAME TO "UNCALIBRATED" *****
                        report_fault(DRIVE_REQUEST_FROM_LV);
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
                    
                    // ***** UNCOMMENT WHEN PEDALS WORK *****
                    if (brake1 >= 400) {//brake1 >= PEDAL_MAX - BRAKE_ERROR_TOLERANCE) {
                        change_state(DRIVE);                        
                    } else {
                        // Driver didn't press pedal
                        report_fault(BRAKE_NOT_PRESSED);
                    }
                }
                
                break;
            case DRIVE:
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
                    case DRIVE_REQUEST_FROM_LV:
                        if (!hv_switch() && !drive_switch()) {
                            // Drive switch was flipped off
                            // Revert to LV
                            change_state(LV);
                        }
                        break;
                    case CONSERVATIVE_TIMER_MAXED:
                        if (!hv_switch() && !drive_switch()) {
                            change_state(LV);
                        }
                        break;
                    case BRAKE_NOT_PRESSED:
                        if (!drive_switch()) {
                            // reset drive switch and try again
                            change_state(HV_ENABLED);
                        }
                        break;
                    case HV_DISABLED_WHILE_DRIVING:
                        if (!hv_switch() && !drive_switch()) {
                            change_state(LV);
                        }
                        break;
                    case SENSOR_DISCREPANCY:
                        // stop power to motors if discrepancy persists for >100ms
                        // see rule T.4.2.5 in FSAE 2022 rulebook
                        if (!drive_switch()) {
                            discrepancy_timer_ms = 0;
                            change_state(HV_ENABLED);
                        }
                        break;
                    case BRAKE_IMPLAUSIBLE:
                        if (!brake_implausible() && hv_switch() && drive_switch()) {
                            change_state(DRIVE);
                        }
                        if (!hv_switch() && !drive_switch()) {
                            change_state(LV);
                        }
                        
                        if (!drive_switch()) {
                            change_state(HV_ENABLED);
                        }
                        break;
                    case ESTOP:
                        if (!hv_switch() && !drive_switch()) {
                            change_state(LV);
                        }
                        break;
                    case SHUTDOWN_CIRCUIT_OPEN:
                        if (shutdown_flags == 0) {
                            change_state(LV);
                        } 
                }
                break;
        }
        
    }

}


/**
 End of File
*/

