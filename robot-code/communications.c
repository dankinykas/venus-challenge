// TU/e Engineering challenge for venus
// This file contains communication functions for the robot

#include <libpynq.h>
#include <time.h>
#include <stdint.h>

#define UART_MQTT_RX IO_AR0
#define UART_MQTT_TX IO_AR1

// Sets up communications, needs to be called before any other function in this header file
void comms_init()
{
  uart_init(UART0);
  switchbox_set_pin(UART_MQTT_RX, SWB_UART0_RX);
  switchbox_set_pin(UART_MQTT_TX, SWB_UART0_TX);
  uart_reset_fifos(UART0);
}

// send message out to MQTT sever over UART
void transmit_message(uint8_t* msg)
{
  // create and send message length header
  uint32_t length = strlen((char*)msg);
  uint8_t bt;
  for (int i = 0; i < 4; i++) {
    bt = ((uint8_t*)&length)[i];
    uart_send(UART0, bt);
    sleep_msec(2);
  }
  sleep_msec(5);
  // send given message data
  for (int i = 0; msg[i] != '\0'; i++) {
    uart_send(UART0, msg[i]);
    sleep_msec(2);
  }
}

// reads message out from UART buffer
void receive_message()
{
  char cr;
  for (int i = 0; uart_has_data(UART0); i++) {
    cr = uart_recv(UART0);
    putchar(cr);
    sleep_msec(2);
  }
  putchar('\n');
}