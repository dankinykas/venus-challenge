// TU/e Engineering challenge for venus
// This is the robot code main file
#include <libpynq.h>
#include <time.h>
#include <stepper.h>
#include <communications.h>
#include <edge_sensors.h>
#include <tcs3472.h> // color sensor library
#include <vl53l0x.h> // distance sensor library

// Code to power up color sensor, adapted from tcs3472 library example code
int color_sensor_init(tcs3472* color_sensor);

// Code to power up distance sensors, adapted from vl53l0x library example code
int distance_sensor_init();

int main()
{
  // Robot setup
  pynq_init();
  comms_init();
  edge_sensors_init();
  stepper_init();
  //connect IIC pins for the sensors that use them
  switchbox_set_pin(IO_AR_SCL, SWB_IIC0_SCL);
	switchbox_set_pin(IO_AR_SDA, SWB_IIC0_SDA);
	iic_init(IIC0);
  vl53x distance_sensor;
  if (distance_sensor_init(&distance_sensor) != 0) {;} // return EXIT_FAILURE;}
  tcs3472 color_sensor;
  if (color_sensor_init(&color_sensor) != 0) {;} // return EXIT_FAILURE;}
  stepper_enable();
  stepper_set_speed(30000, 30000);

  // Code for execution
  uint32_t iDistance;
  for (int i=0; i<1200; i++) // read values 20 times a second for 1 minute
	{
		iDistance = tofReadDistance(&distance_sensor);
		printf("Distance = %dmm\n", iDistance);
		sleep_msec(100);
	}

  if (get_edge(LEFT) == false) {
    printf("No edge here\n");
  } else {
    printf ("Edge here!\n");
  }

  // Robot destruction code
  iic_destroy(IIC0);
  uart_destroy(UART0);
  pynq_destroy();
  return EXIT_SUCCESS;
}

// Code to power up color sensor, adapted from tcs3472 library example code
int color_sensor_init(tcs3472* color_sensor)
{
  int integration_time_ms = 60;
  uint8_t id;
  int debug;
  // check connection
	debug = tcs_ping(IIC0, &id);
	if(debug != TCS3472_SUCCES)
	{
		printf("Color sensor connection failure\n");
		return 1;
	}
	printf("Color sensor connection success\n");
	printf("-- ID: %#X\n", id);
  // connect sensor
	tcs3472 sensor = TCS3472_EMPTY;
	tcs_set_integration(&sensor, tcs3472_integration_from_ms(integration_time_ms));
	tcs_set_gain(&sensor, x4);
  debug = tcs_init(IIC0, &sensor);
  if(debug != TCS3472_SUCCES)
	{
		printf("Color sensor initialization failure\n");
		return 1;
	}
	printf("Color sensor initialization success\n");
	fflush(NULL);
  // wait one integration cycle to start getting readings
  sleep_msec(integration_time_ms);
  *color_sensor = sensor;
  return 0;
}

// Code to power up distance sensors, adapted from vl53l0x library example code
int distance_sensor_init(vl53x* distance_sensor)
{
  int debug;
	uint8_t addr = 0x29;
	debug = tofPing(IIC0, addr);
	if(debug != 0)
	{
		printf("Distance sensor connection failure\n");
		return 1;
	}
	printf("Distance sensor connection success\n");
	//Create a sensor struct
	vl53x sensor;
	//Initialize the sensor
	debug = tofInit(&sensor, IIC0, addr, 0); // set default range mode (up to 800mm)
	if (debug != 0)
	{
    printf("Distance sensor initialization failure\n");
		return 1;
	}
  printf("Distance sensor initialization success\n");
	uint8_t model, revision;
	tofGetModel(&sensor, &model, &revision);
	printf("Model ID - %d\n", model);
	printf("Revision ID - %d\n", revision);
	fflush(NULL); //Get some output even is distance readings hang
  *distance_sensor = sensor;
  return 0;
}