# Vehicle Control Unit (VCU)

This is the code for the dsPIC33CH526MP505-E/M4 microcontroller on the VCU, a board that ensures correct vehicle state of operation. It houses the vehicle's state machine (shown below), Brake System Plausibility Device (BSPD) and sends torque requests to the motor controller via PCAN.

## Vehicle State Machine
![fsm-diagram](https://github.com/FormulaRacingatUCDavis/VCU-Firmware-FE9/blob/main/Vehicle%20State%20Machine.drawio.png?raw=true)

### State Machine Inputs:
 - vehicle drive switches (PCAN)
 - motor controller feedback (PCAN)
 - pedal positions (analog)

## Throttle mapping
Converting pedal inputs to torque requests. VCU uses throttle mapping that adapts to BMS values; throttle mapping attenuates with increasing pack temperature.

## Related Docs
### VCU-Firmware-FE9-Prototype: https://github.com/FormulaRacingatUCDavis/VCU-Firmware-FE9-Prototype
Prototype for this code. Tested on a PICduino board with PIC18F26K83 microcontroller.
### FE9 CAN Index: https://docs.google.com/spreadsheets/d/1NBGkYXU-0hWoHLGAnpFGG1UOc4kb2333ix3pwNxI9C4/edit#gid=0
Documentation of what vehicle CAN messages are in use.
