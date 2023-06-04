#include "can_manager.h"

extern volatile state_t state;
extern volatile error_t error;

volatile uint16_t capacitor_volt = 0;
volatile uint8_t shutdown_flags = 0b00111000;  //start with shutdown flags OK
volatile uint8_t estop_flags = 0;
volatile uint8_t switches = 0xC0;   //start with switches on to stay in startup state
volatile uint8_t PACK_TEMP;

// receive buffer message
CAN_MSG_OBJ msg_RX;

/********** OUTGOING CAN MESSAGES **********/

// torque request command
//uint8_t data_TX_torque[4];
CAN_MSG_FIELD field_TX_torque = {
    .idType = 0,
    .frameType = 0,
    .dlc = 6,
    .formatType = 0,
    .brs = 0
};
CAN_MSG_OBJ msg_TX_torque = {
    .msgId = TORQUE_REQUEST_COMMAND,
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
            case MC_ESTOP:
                estop_flags = msg_RX.data[0];
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

//  CAN transmit torque request command
void can_tx_torque_request(){
    uint8_t state_msg_byte = one_byte_state(); 
     
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

    msg_TX_torque.field = field_TX_torque; 
    msg_TX_torque.data = data_TX_torque;
    CAN1_Transmit(CAN1_TX_TXQ, &msg_TX_torque);
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

