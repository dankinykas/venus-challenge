// TU/e Engineering challenge for venus
// This file contains functions for using the color sensor provided as part of the course

#include <libpynq.h>
#include <time.h>
#include <stepper.h>
#include <pulsecounter.h>
#include <stdint.h>

#define PULSECOUNTER_FREQUENCY 100000000
#define COLOR_SENSOR_OUT IO_AR4
#define COLOR_SENSOR_S2 IO_AR5
#define COLOR_SENSOR_S3 IO_AR6

struct pulsecount {
  uint32_t count;
  uint32_t time;
};

double get_color_frequency()
{
  struct pulsecount pc1, pc2;
  pc1.count = pulsecounter_get_count(PULSECOUNTER0, &pc1.time);
  sleep_msec(20);
  pc2.count = pulsecounter_get_count(PULSECOUNTER0, &pc2.time);
  return ((double)(pc2.count - pc1.count) / (double)(pc2.time - pc1.time)) * PULSECOUNTER_FREQUENCY / 1000;
}

/* example/testing main function:
int main()
{
  double f_red, f_green, f_blue;
  pynq_init();
  switchbox_set_pin(COLOR_SENSOR_OUT, SWB_TIMER_IC0);
  gpio_set_direction(COLOR_SENSOR_S2, GPIO_DIR_OUTPUT);
  gpio_set_direction(COLOR_SENSOR_S3, GPIO_DIR_OUTPUT);
  pulsecounter_init(PULSECOUNTER0);
  pulsecounter_set_edge(PULSECOUNTER0, GPIO_LEVEL_HIGH);
  pulsecounter_reset_count(PULSECOUNTER0);
  // gpio_set_level(COLOR_SENSOR_S2, GPIO_LEVEL_HIGH);
  // gpio_set_level(COLOR_SENSOR_S3, GPIO_LEVEL_LOW);

  while (1) {
    // red
    gpio_set_level(COLOR_SENSOR_S2, GPIO_LEVEL_LOW);
    gpio_set_level(COLOR_SENSOR_S3, GPIO_LEVEL_LOW);
    sleep_msec(1);
    f_red = get_color_frequency();
    // green
    gpio_set_level(COLOR_SENSOR_S2, GPIO_LEVEL_HIGH);
    gpio_set_level(COLOR_SENSOR_S3, GPIO_LEVEL_HIGH);
    sleep_msec(1);
    f_green = get_color_frequency();
    // blue
    gpio_set_level(COLOR_SENSOR_S2, GPIO_LEVEL_LOW);
    gpio_set_level(COLOR_SENSOR_S3, GPIO_LEVEL_HIGH);
    sleep_msec(1);
    f_blue = get_color_frequency();

    printf("R %4.4lf G %4.4lf B %4.4lf\n", f_red, f_green, f_blue);
    // sleep_msec(100);
  }

  pynq_destroy();
  return EXIT_SUCCESS;
}*/