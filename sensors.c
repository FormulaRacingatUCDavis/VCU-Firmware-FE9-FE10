/*
 * File:   sensors.c
 * Author: leoja
 *
 * Created on March 4, 2023, 3:14 PM
 */

#include "sensors.h"

extern volatile state_t state;
extern volatile error_t error;

CALIBRATED_SENSOR_t throttle1;
CALIBRATED_SENSOR_t throttle2;
CALIBRATED_SENSOR_t brake;

// APPS
uint8_t THROTTLE_MULTIPLIER = 100;
const uint8_t THROTTLE_MAP[8] = { 95, 71, 59, 47, 35, 23, 11, 5 };

extern volatile uint8_t PACK_TEMP;

/************ Timer ************/
unsigned int discrepancy_timer_ms = 0;

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

void run_calibration() {
    update_minmax(&throttle1);
    update_minmax(&throttle2);
    update_minmax(&brake);
}

void update_sensor_vals() {
    // APPS1 = pin8 = RA0
    throttle1.raw = getConversion(APPS1);
    update_percent(&throttle1);
    // APPS2 = pin9 = RA1
    throttle2.raw = getConversion(APPS2);
    update_percent(&throttle2);
    // BSE1 = pin11 = RA3
    brake.raw = getConversion(BSE1);
    update_percent(&brake);
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
            report_fault(SENSOR_DISCREPANCY);
        }
    } else {
        discrepancy_timer_ms = 0;
    }
}

uint16_t requested_throttle(){
    temp_attenuate();
    
    uint32_t throttle = ((uint32_t)throttle1.percent * MAX_TORQUE) / 100;  //upscale for MC code
    throttle = (throttle * THROTTLE_MULTIPLIER) / 100;       //attenuate for temperature
    return (uint16_t)throttle;
}

void temp_attenuate() {
    int t = PACK_TEMP - 50;
    if (t < 0) {
        THROTTLE_MULTIPLIER = 100;   
    } else if (t < 8) {
        THROTTLE_MULTIPLIER = THROTTLE_MAP[t]; 
    } else if (t >= 8) {
        THROTTLE_MULTIPLIER = THROTTLE_MAP[7]; 
    }
}

bool sensors_calibrated(){
    if(throttle2.range < APPS1_MIN_RANGE) return 0;
    if(brake.range < BRAKE_MIN_RANGE) return 0;
    
    return 1;
}

bool braking(){
    return brake.raw > BRAKE_LIGHT_THRESHOLD;
}

bool brake_mashed(){
    return brake.raw > RTD_BRAKE_THRESHOLD;
}

// check differential between the throttle sensors
// returns true only if the sensor discrepancy is > 10%
// Note: after verifying there's no discrepancy, can use either sensor(1 or 2) for remaining checks
bool has_discrepancy() {
    if(abs((int)throttle1.percent - (int)throttle2.percent) > 10) return 1;  //percentage discrepancy
    
    return (throttle1.raw < APPS_OPEN_THRESH)
        || (throttle1.raw > APPS_SHORT_THRESH)
        || (throttle2.raw < APPS_OPEN_THRESH)
        || (throttle2.raw > APPS_SHORT_THRESH);   //wiring fault
            
}

// check for soft BSPD
// see EV.5.7 of FSAE 2022 rulebook
bool brake_implausible() {    
    if (error == BRAKE_IMPLAUSIBLE) {
        // once brake implausibility detected,
        // can only revert to normal if throttle unapplied
        return !(throttle1.percent <= APPS1_BSPD_RESET);
    }
    
    // if both brake and throttle applied, brake implausible
    //return (temp_brake > 0 && temp_throttle > throttle_range * 0.25);
    return (brake.raw >= BRAKE_BSPD_THRESHOLD && throttle1.percent > APPS1_BSPD_THRESHOLD);
}

void update_percent(CALIBRATED_SENSOR_t* sensor){
    uint32_t raw = (uint32_t)clamp(sensor->raw, sensor->min, sensor->max); 
    sensor->percent = (uint16_t)((100*(raw-sensor->min))/((sensor->range)));
}

void update_minmax(CALIBRATED_SENSOR_t* sensor){
    if (sensor->raw > sensor->max) sensor->max = sensor->raw;  
    else if (sensor->raw < sensor->min) sensor->min = sensor->raw;
    if(sensor->max > sensor->min) sensor->range = sensor->max - sensor->min;
}

uint16_t clamp(uint16_t in, uint16_t min, uint16_t max){
    if(in > max) return max;
    if(in < min) return min; 
    return in;
}

void init_sensors(){
    throttle1.min = 0x7FFF;
    throttle1.max = 0;
    throttle1.range = 1;
    throttle2.min = 0x7FFF;
    throttle2.max = 0;
    throttle2.range = 1;
    brake.min = 0x7FFF;
    brake.max = 0;
    brake.range = 1;
}