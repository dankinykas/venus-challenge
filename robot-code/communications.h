// TU/e Engineering challenge for venus
// This file contains communication function headers for the robot

#ifndef TEAM32_COMMUNICATIONS_H
#define TEAM32_COMMUNICATIONS_H

#include <libpynq.h>
#include <time.h>
#include <stdint.h>

#define UART_MQTT_RX IO_AR0
#define UART_MQTT_TX IO_AR1

// Sets up communications, needs to be called before any other function in this header file
void comms_init();

// Sends message out to MQTT sever over UART
void transmit_message(uint8_t* msg);

// Reads message out from UART buffer
void receive_message();

#endif