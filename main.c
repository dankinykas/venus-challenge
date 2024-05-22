// TU/e Engineering challenge for venus
// This is the robot code main file
#include <libpynq.h>
#include <time.h>
#include <stepper.h>
#include "communications.c"
#include "default_color_sensor.c"

int main()
{
  pynq_init();
  uart_init(UART0);
  switchbox_set_pin(IO_AR0, SWB_UART0_RX);
  switchbox_set_pin(IO_AR1, SWB_UART0_TX);
  uart_reset_fifos(UART0);
  printf("UARTs have been initialized\n");
  stepper_init();
  stepper_enable();
  stepper_set_speed(30000, 30000); //set to slow speed by default as a safety measure

  stepper_steps(3200, -3200);

  uart_destroy(UART0);
  pynq_destroy();
  return EXIT_SUCCESS;
}