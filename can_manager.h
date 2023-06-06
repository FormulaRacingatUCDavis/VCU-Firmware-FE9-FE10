/* 
 * File:   can_manager.h
 * 
 */

#ifndef CAN_MANAGER_H
#define	CAN_MANAGER_H

#include "mcc_generated_files/mcc.h"
#include "mcc_generated_files/system.h"
#include "stdint.h"
#include "libpic30.h"

#include "fsm.h"
#include "sensors.h"


/********** ENUM OF CAN IDS **********/
typedef enum {
    // PCAN
    BSPD_FLAGS = 0x0c1,
    DRIVER_SWITCHES = 0x0d0,
    VCU_STATE = 0x766,
    MC_COMMAND = 0x0C0, 
    BRAKE_COMMAND = 0x767,
    BMS_STATUS_MSG = 0x380,
    PEI_ESTOP = 0x366,
    PEI_CURRENT_SHUTDOWN = 0x387,
    BMS_VOLTAGES = 0x388,
    BMS_TEMPERATURES = 0x389,
    MC_VOLTAGE_INFO = 0x0A7,
    MC_INTERNAL_STATES = 0xAA,
    // TCAN
    FRONT_LEFT_WHEEL_SPEED = 0x470,
    FRONT_RIGHT_WHEEL_SPEED = 0x471,
    BACK_LEFT_WHEEL_SPEED = 0x472,
    BACK_RIGHT_WHEEL_SPEED = 0x473
} CAN_ID;


void can_init();
void can_receive();
void can_tx_vcu_state();
void can_tx_torque_request();
void can_tx_disable_MC();


#endif	/* CAN_MANAGER_H */