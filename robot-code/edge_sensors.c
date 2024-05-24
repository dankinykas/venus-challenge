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
#define EDGE_SENSOR_LEFT_OUT IO_AR4
#define EDGE_SENSOR_RIGHT_OUT IO_AR5

enum EdgeSensors
{
  LEFT,
  RIGHT
};

struct pulsecount {
  uint32_t count;
  uint32_t time;
};

// Sets up edge sensors, needs to be called before using get_edge()
void edge_sensors_init()
{
  // set up left sensor
  switchbox_set_pin(EDGE_SENSOR_LEFT_OUT, SWB_TIMER_IC0);
  pulsecounter_init(PULSECOUNTER0);
  // set up right sensor
  switchbox_set_pin(EDGE_SENSOR_RIGHT_OUT, SWB_TIMER_IC1);
  pulsecounter_init(PULSECOUNTER1);
}

// Returns whether a sensor is looking at an edge
bool get_edge(enum EdgeSensors sensor)
{
  struct pulsecount pc1, pc2;
  double frequency;
  pulsecounter_index_t pulsecounter = (sensor == LEFT) ? PULSECOUNTER0 : PULSECOUNTER1;
  pc1.count = pulsecounter_get_count(pulsecounter, &pc1.time);
  sleep_msec(20);
  pc2.count = pulsecounter_get_count(pulsecounter, &pc2.time);
  frequency =  ((double)(pc2.count - pc1.count) / (double)(pc2.time - pc1.time)) * PULSECOUNTER_FREQUENCY / 1000;
  return frequency < EDGE_THRESHOLD; // There is an edge, if the sensor is looking at something darker than a chosen threshold
}