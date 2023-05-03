/*
 * File:   uart.c
 * Author: leoja
 *
 * Created on March 6, 2023, 12:03 PM
 */

#include "uart.h"

extern volatile state_t state;
extern volatile error_t error;
extern CALIBRATED_SENSOR_t throttle1;
extern CALIBRATED_SENSOR_t throttle2;
extern CALIBRATED_SENSOR_t brake;
extern volatile uint16_t capacitor_volt;

void print_state(){
    printf("STATE: ");
    switch(state){
        case STARTUP: 
            printf("STARTUP");
            break;
        case LV: 
            printf("LV");
            break;
        case PRECHARGING: 
            printf("PRECHARGING");
            break;
        case HV_ENABLED: 
            printf("HV ENABLED");
            break;
        case DRIVE:
            printf("DRIVE"); 
            break;
        default: 
            printf("FAULT - ");
            switch(error){
                case DRIVE_REQUEST_FROM_LV: 
                    printf("DRIVE REQUEST FROM LV");
                    break;
                case CONSERVATIVE_TIMER_MAXED:
                    printf("PRECHARGE TIMEOUT");
                    break;
                case BRAKE_NOT_PRESSED: 
                    printf("BRAKE NOT PRESSED");
                    break;
                case HV_DISABLED_WHILE_DRIVING:
                    printf("HV DISABLED WHILE DRIVING");
                    break;
                case SENSOR_DISCREPANCY:
                    printf("SENSOR DISCREPANCY");
                    break;
                case BRAKE_IMPLAUSIBLE:
                    printf("SOFT BSPD TRIPPED");
                    break;
                case SHUTDOWN_CIRCUIT_OPEN: 
                    printf("SHUTDOWN CIRCUIT OPEN");
                    break;
                case UNCALIBRATED: 
                    printf("PEDALS NOT CALIBRATED");
                    break;
                case HARD_BSPD:
                    printf("HARD BSPD TRIPPED");
                    break;
                default:
                    printf("YIKES");
            }
    }
    printf("\n\r");
}

void print_pedal_vals(){
    printf("APPS1: %u%%\n\r", throttle1.percent);
    //printf("RAW: %u%%\n\r", throttle1.raw);
    //printf("MIN: %u%%\n\r", throttle1.min);
    //printf("MAX: %u%%\n\r", throttle1.max);
    //printf("RANGE: %u%%\n\r", throttle1.range);
    printf("APPS2: %u%%\n\r", throttle2.percent);
    printf("BRAKE: %u%%\n\r", brake.percent);
    //printf("STATE: %u", state);
}

void clear_screen(){
    printf("\e[1;1H\e[2J");
}

void gui_dump(){
    uart_write(ESCAPE_CHAR);
    uart_write(FRAME_START);
    
    uint16_t bms_volt = 504;
    
    send_byte_with_escape(state);
    send_byte_with_escape(throttle1.percent);
    send_byte_with_escape(throttle2.percent);
    send_byte_with_escape(brake.percent);
    send_byte_with_escape(0xFF&(bms_volt>>8));
    send_byte_with_escape(0xFF&bms_volt);
    send_byte_with_escape(0xFF&(capacitor_volt>>8));
    send_byte_with_escape(0xFF&capacitor_volt);
    
    uart_write(ESCAPE_CHAR);
    uart_write(FRAME_END);
}

//sends extra escape byte if byte is escape byte
void send_byte_with_escape(uint8_t byte){
    if(byte == ESCAPE_CHAR){
        uart_write(ESCAPE_CHAR);
    }
    uart_write(byte);
}

void uart_write(uint8_t byte){
    uint8_t counter = 0;
    
    while(!UART1_IsTxReady()){
        counter++;
        if(counter>255) return;
    }
    
    UART1_Write(byte); 
}