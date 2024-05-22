// TU/e Engineering challenge for venus
// This file contains functions for the robot's cliff / edge sensors
// They are implemented by using the color sensors provided to us by the course
// Set to brightness detection mode (S2 = ?, S3 = ?)

#include <libpynq.h>
#include <time.h>
#include <stepper.h>
#include <pulsecounter.h>
#include <stdint.h>

#define PULSECOUNTER_FREQUENCY 100000000
#define EDGE_THRESHOLD 1000
#define EDGE_SENSOR0_OUT IO_AR4

struct pulsecount {
  uint32_t count;
  uint32_t time;
};


// Sets up edge sensors, needs to be called before using get_edge()
void edge_sensors_init()
{
  switchbox_set_pin(EDGE_SENSOR0_OUT, SWB_TIMER_IC0);
  pulsecounter_init(PULSECOUNTER0);
}

// Returns whether a sensor is looking at an edge
bool get_edge()
{
  struct pulsecount pc1, pc2;
  double frequency;
  pc1.count = pulsecounter_get_count(PULSECOUNTER0, &pc1.time);
  sleep_msec(20);
  pc2.count = pulsecounter_get_count(PULSECOUNTER0, &pc2.time);
  frequency =  ((double)(pc2.count - pc1.count) / (double)(pc2.time - pc1.time)) * PULSECOUNTER_FREQUENCY / 1000;
  return frequency < EDGE_THRESHOLD; // There is an edge, if the sensor is looking at something darker than a chosen threshold
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