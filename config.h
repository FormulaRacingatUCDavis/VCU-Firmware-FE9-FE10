/* Microchip Technology Inc. and its subsidiaries.  You may use this software 
 * and any derivatives exclusively with Microchip products. 
 * 
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS".  NO WARRANTIES, WHETHER 
 * EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED 
 * WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A 
 * PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP PRODUCTS, COMBINATION 
 * WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION. 
 *
 * IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
 * INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
 * WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS 
 * BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE 
 * FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS 
 * IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF 
 * ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE 
 * TERMS. 
 */

/* 
 * File:   
 * Author: 
 * Comments:
 * Revision history: 
 */

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef CONFIG_H
#define	CONFIG_H

#include <xc.h> // include processor files - each processor file is guarded.  

// How long to wait for pre-charging to finish before timing out
#define PRECHARGE_TIMEOUT_MS 5000
// Delay between checking pre-charging state

#define TMR1_PERIOD_MS 20

// discrepancy timer
#define MAX_DISCREPANCY_MS 100 

#define PRECHARGE_THRESHOLD 4976 // 77V = 90% of nominal accumulator voltage, scaled from range of 0-12800(0V-200V)

// On the breadboard, the range of values for the potentiometer is 0 to 4095
#define PEDAL_MAX 4095

#define APPS1_MIN_RANGE 0xD00   //will not enter HV until pedal is calibrated
#define BRAKE_MIN_RANGE 0x300

//in percent:
#define APPS1_BSPD_THRESHOLD 25  
#define APPS1_BSPD_RESET 5

//in raw ADC: 
#define APPS_SHORT_THRESH 3900   //~4.75V
#define APPS_OPEN_THRESH 200     //~0.25V

#define BRAKE_LIGHT_THRESHOLD 400     
#define RTD_BRAKE_THRESHOLD 500  //brake threshold to enter drive mode
#define BRAKE_BSPD_THRESHOLD 500

#endif	/* XC_HEADER_TEMPLATE_H */

