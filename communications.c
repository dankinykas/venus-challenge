// TU/e Engineering challenge for venus
// This file contains communication functions for the robot

#include <libpynq.h>
#include <time.h>
#include <stdint.h>

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