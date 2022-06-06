/* 
 * File:   can_manager.h
 * 
 */

#ifndef CAN_MANAGER_H
#define	CAN_MANAGER_H

#ifdef	__cplusplus
extern "C" {
#endif




#ifdef	__cplusplus
}
#endif

#endif	/* CAN_MANAGER_H */

/********** ENUM OF CAN IDS **********/
typedef enum {
    VEHICLE_STATE = 0x0c0,
    DRIVER_SWITCHES = 0x0d0,
    TORQUE_REQUEST_COMMAND = 0x766,
    BRAKE_COMMAND = 0x767,
    BMS_STATUS_MSG = 0x380,
    PEI_CURRENT = 0x387,
    BMS_VOLTAGES = 0x388,
    BMS_TEMPERATURES = 0x389,
    MC_ESTOP = 0x366,
    MC_DEBUG = 0x466,
    MC_PDO_SEND = 0x566,
    MC_PDO_ACK = 0x666
} CAN_ID;


// receive buffer message
CAN_MSG_OBJ msg_RX;


/********** OUTGOING CAN MESSAGES **********/

// vehicle state
uint8_t data_TX_state[1];
CAN_MSG_FIELD field_TX_state = {
    .idType = 0,
    .frameType = 0,
    .dlc = 1,
    .formatType = 0,
    .brs = 0
};
CAN_MSG_OBJ msg_TX_state = {
    .msgId = VEHICLE_STATE,
    .field = {0}, // null
    .data = data_TX_state // null pointer
};

// torque request command
//uint8_t data_TX_torque[5];
CAN_MSG_FIELD field_TX_torque = {
    .idType = 0,
    .frameType = 0,
    .dlc = 5,
    .formatType = 0,
    .brs = 0
};
CAN_MSG_OBJ msg_TX_torque = {
    .msgId = TORQUE_REQUEST_COMMAND,
    .field = {0}, // null
    .data = 0 // null pointer
};

// brake command
uint8_t data_TX_brake[1];
CAN_MSG_FIELD field_TX_brake = {
    .idType = 0,
    .frameType = 0,
    .dlc = 1,
    .formatType = 0,
    .brs = 0
};
CAN_MSG_OBJ msg_TX_brake = {
    .msgId = BRAKE_COMMAND,
    .field = {0}, // null
    .data = data_TX_brake // null pointer
};