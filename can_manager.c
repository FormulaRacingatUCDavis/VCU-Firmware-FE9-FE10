#include "can_manager.h"

extern volatile state_t state;
extern volatile error_t error;

extern volatile uint16_t TC_torque_adjustment;

volatile uint8_t mc_lockout;
volatile uint8_t mc_enabled;
volatile uint16_t capacitor_volt = 0;
volatile uint8_t shutdown_flags = 0b00111000;  //start with shutdown flags OK
volatile uint8_t estop_flags = 0;
volatile uint8_t switches = 0xC0;   //start with switches on to stay in startup state
volatile uint8_t PACK_TEMP;

// From TCAN
volatile uint16_t front_right_wheel_speed = 0;
volatile uint16_t front_left_wheel_speed = 0;
volatile uint16_t back_right_wheel_speed = 0;
volatile uint16_t back_left_wheel_speed = 0;

// receive buffer message
CAN_MSG_OBJ msg_RX;

/********** OUTGOING CAN MESSAGES **********/

// mc command
CAN_MSG_FIELD field_TX_mc_command = {
    .idType = 0,
    .frameType = 0,
    .dlc = 6,
    .formatType = 0,
    .brs = 0
};
CAN_MSG_OBJ msg_TX_mc_command = {
    .msgId = MC_COMMAND,
    .field = {0}, // null
    .data = 0 // null pointer
};
// VCU state
CAN_MSG_FIELD field_TX_vcu_state = {
    .idType = 0,
    .frameType = 0,
    .dlc = 6,
    .formatType = 0,
    .brs = 0
};
CAN_MSG_OBJ msg_TX_vcu_state = {
    .msgId = VCU_STATE,
    .field = {0}, // null
    .data = 0 // null pointer
};

/************ CAN ************/

void can_receive() {
    // gets message and updates values
    if (CAN1_Receive(&msg_RX)) {
        switch (msg_RX.msgId) {
            case DRIVER_SWITCHES:
                switches = msg_RX.data[0]; 
                break;
            case BMS_STATUS_MSG:
                PACK_TEMP = msg_RX.data[0];
                temp_attenuate();
                break;
            case PEI_ESTOP:
                estop_flags = msg_RX.data[0];
            case MC_VOLTAGE_INFO: 
                capacitor_volt = (msg_RX.data[0] << 8); // upper bits
                capacitor_volt += msg_RX.data[1]; // lower bits
            case MC_INTERNAL_STATES:
                mc_lockout = msg_RX.data[6] & 0b1000000;
                mc_enabled = msg_RX.data[6] & 0b1;
            case PEI_CURRENT_SHUTDOWN: 
                shutdown_flags = msg_RX.data[2];
            default:
                // no valid input received
                break;
        }
    } 
    
    if (CAN2_Receive(&msg_RX)) {
        switch (msg_RX.msgId) {
            case FRONT_RIGHT_WHEEL_SPEED:	
                front_right_wheel_speed = (msg_RX.data[0] << 8) ; 
                front_right_wheel_speed += msg_RX.data[1]; 
                break;
            case FRONT_LEFT_WHEEL_SPEED:
                front_left_wheel_speed = (msg_RX.data[0] << 8) ; 
                front_left_wheel_speed += msg_RX.data[1]; 
                break;
            case BACK_RIGHT_WHEEL_SPEED:	
                back_right_wheel_speed = (msg_RX.data[0] << 8) ; 
                back_right_wheel_speed += msg_RX.data[1]; 
                break;
            case BACK_LEFT_WHEEL_SPEED:
                back_left_wheel_speed = (msg_RX.data[0] << 8) ; 
                back_left_wheel_speed += msg_RX.data[1]; 
                break;
            default:
                // no valid input received
                break;
        }
    } 
}

// CAN transmit VCU state and HV request 
void can_tx_vcu_state(){
    uint16_t state_msg_byte = state; 
    if (state == FAULT) { 
        state_msg_byte = 0b10000000 + error; // greatest bit = 1 if fault 
    }
     
    uint16_t throttle_msg_byte = 0;
    if (state == DRIVE) {
        throttle_msg_byte = requested_throttle();
    }
    
    uint8_t data_TX_torque[6] = {
        hv_requested(),
        (uint8_t)(throttle_msg_byte >> 8), // torque request upper
        (uint8_t)(throttle_msg_byte & 0xff), // torque request lower
        (error != SHUTDOWN_CIRCUIT_OPEN),
        state_msg_byte,
        braking()
    };

    msg_TX_mc_command.field = field_TX_vcu_state; 
    msg_TX_mc_command.data = data_TX_torque;
    CAN1_Transmit(CAN1_TX_TXQ, &msg_TX_mc_command);
}

void can_tx_torque_request(){
    
    uint16_t throttle_msg_byte = 0;
    if (state == DRIVE) {
        throttle_msg_byte = requested_throttle() + TC_torque_adjustment;
    }
    
    uint8_t data_TX_torque[8] = {
        (uint8_t)(throttle_msg_byte & 0xff), // 0 - torque command lower (Nm*10)
        (uint8_t)(throttle_msg_byte >> 8), // 1 - torque command upper (Nm*10)
        0, // 2 - speed command lower (not applicable)
        0, // 3 - speed command upper (not applicable)
        1, // 4 - direction (1 = forward, 0 = backward)
        0, // 5 - mode (0 = torque mode, 1 = speed mode)
        0, // 6 - torque limit lower (if 0, default EEPROM value used)
        0 // 7 - torque limit upper (if 0, default EEPROM value used)
    };

    msg_TX_mc_command.field = field_TX_mc_command; 
    msg_TX_mc_command.data = data_TX_torque;
    CAN1_Transmit(CAN1_TX_TXQ, &msg_TX_mc_command);
}

void can_init(){
    // Set up CAN
    CAN1_OperationModeSet(CAN_CONFIGURATION_MODE);
    if(CAN_CONFIGURATION_MODE == CAN1_OperationModeGet()) {
        if(CAN_OP_MODE_REQUEST_SUCCESS == CAN1_OperationModeSet(CAN_NORMAL_2_0_MODE)) {
            // CAN set up successful
        }
    }
}

void can_tx_disable_MC() {
    uint8_t data_TX_disable_mc[] = { 0, 0, 0, 0, 0, 0, 0 };

    msg_TX_mc_command.field = field_TX_mc_command; 
    msg_TX_mc_command.data = data_TX_disable_mc;
    CAN1_Transmit(CAN1_TX_TXQ, &msg_TX_mc_command);
}

