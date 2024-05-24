// TU/e Engineering challenge for venus
// This file contains function headers for using the color sensor provided as part of the course

#ifndef TEAM32_DEFAULTCOLOR_H
#define TEAM32_DEFAULTCOLOR_H

#include <libpynq.h>
#include <time.h>
#include <stepper.h>
#include <pulsecounter.h>
#include <stdint.h>

#define PULSECOUNTER_FREQUENCY 100000000
#define COLOR_SENSOR_OUT IO_AR4

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
void edge_sensors_init();

// Returns whether a sensor is looking at an edge
bool get_edge(enum EdgeSensors sensor);

#endif