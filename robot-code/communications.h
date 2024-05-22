// TU/e Engineering challenge for venus
// This file contains communication function headers for the robot

#include <libpynq.h>
#include <time.h>
#include <stdint.h>

// send message out to MQTT sever over UART
void transmit_message(uint8_t* msg);

// reads message out from UART buffer
void receive_message();