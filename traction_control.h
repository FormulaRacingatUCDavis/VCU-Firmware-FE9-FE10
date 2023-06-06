/* 
 * File:   traction_control.h
 *
 */

#ifndef TRACTION_CONTROL_H
#define	TRACTION_CONTROL_H

extern volatile state_t state;
extern volatile uint16_t front_right_wheel_speed;
extern volatile uint16_t front_left_wheel_speed;
extern volatile uint16_t back_right_wheel_speed;
extern volatile uint16_t back_left_wheel_speed;

const float pi = 3.14;
const float wheel_radius = 0.5; // PLACEHOLDER VALUE
const uint16_t pulses_per_rev = 60; // from wheel speed sensor

const float target_slip_ratio = 0.1; 

const uint8_t kP = 0;
const uint8_t kI = 0;
const uint8_t kD = 0;

const uint16_t TC_torque_limit = 100;
volatile uint16_t TC_control_var = 0;
volatile uint16_t TC_torque_adjustment = 0;

volatile uint16_t pid_error = 0;
volatile uint16_t prev_pid_error = 0;

volatile uint16_t integral = 0;
volatile uint16_t derivative = 0;

void traction_control_PID() {
    if (state != DRIVE) return;
    
    // note: wheel speeds are in units of pulses/20ms
    const float avg_front_wheel_speed = (front_right_wheel_speed + front_left_wheel_speed)/2.0; 
    const float avg_back_wheel_speed = (back_right_wheel_speed + back_left_wheel_speed)/2.0; 
    const float conversion_factor = (2*pi*wheel_radius)/pulses_per_rev;
    const float current_slip_ratio = (avg_back_wheel_speed*conversion_factor) / (avg_front_wheel_speed*conversion_factor); 
    
    // if target slip ratio has been achieved
//    if (current_slip_ratio < target_slip_ratio + 0.001 || current_slip_ratio > target_slip_ratio - 0.001) return; 
    
    pid_error = target_slip_ratio - current_slip_ratio;
    integral = integral + pid_error;
    derivative = pid_error - prev_pid_error;
    
    TC_control_var = (kP * pid_error) + (kI * integral) + (kD * derivative);
    
    // limit PID torque request
    if (TC_control_var > TC_torque_limit) TC_control_var = TC_torque_limit;
    else if (TC_control_var < TC_torque_limit) TC_control_var = TC_torque_limit;
    
    TC_torque_adjustment = TC_control_var;
   
    prev_pid_error = pid_error;
}


#endif	/* TRACTION_CONTROL_H */

