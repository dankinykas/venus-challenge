// TU/e Engineering challenge for venus
// This is the robot code main file
#include <libpynq.h>
#include <time.h>
#include <stepper.h>
#include <communications.h>
#include <edge_sensors.h>

int main()
{
  pynq_init();
  comms_init();
  edge_sensors_init();
  stepper_init();
  stepper_enable();
  stepper_set_speed(30000, 30000); // set to slow speed by default as a safety measure

  // do things

  uart_destroy(UART0);
  pynq_destroy();
  return EXIT_SUCCESS;
}