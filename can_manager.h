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
    VEHICLE_STATE = 0x0c0,
    BSPD_FLAGS = 0x0c1,
    DRIVER_SWITCHES = 0x0d0,
    TORQUE_REQUEST_COMMAND = 0x766,
    BRAKE_COMMAND = 0x767,
    BMS_STATUS_MSG = 0x380,
    PEI_CURRENT_SHUTDOWN = 0x387,
    BMS_VOLTAGES = 0x388,
    BMS_TEMPERATURES = 0x389,
    MC_ESTOP = 0x366,
    MC_DEBUG = 0x466,
    MC_PDO_SEND = 0x566,
    MC_PDO_ACK = 0x666,
} CAN_ID;


void can_init();
void can_receive();
void can_tx_torque_request();


#endif	/* CAN_MANAGER_H */