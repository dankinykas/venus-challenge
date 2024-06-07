// TU/e Engineering challenge for venus
// This is the robot code main file
#include <libpynq.h>
#include <time.h>
#include <stepper.h>
#include <communications.h>
#include <edge_sensors.h>
#include <tcs3472.h> // color sensor library
#include <vl53l0x.h> // distance sensor library

#define TOF_ADDRESS_LOW 0x69
#define TOF_ADDRESS_HIGH 0x70
#define TOF_ADDRESS_MIDDLE 0x71

#define INTEGRATION_TIME_MS 60

// Code to power up color sensor, adapted from tcs3472 library example code
int color_sensor_init(tcs3472* color_sensor);

// Code to power up distance sensors, adapted from vl53l0x library example code
int distance_sensor_init(vl53x* distance_sensor_lower, vl53x* distance_sensor_upper, vl53x* distance_sensor_middle);

void print_colour(uint16_t red, uint16_t green, uint16_t blue)
{
    printf("\033[3F\033[0J"); // Move cursor back 3 lines
    printf("\033[48;2;%hhu;%hhu;%hhum      \033[0m", CLAMP_255((red >> 4)), CLAMP_255((green >> 4)), CLAMP_255((blue >> 4))); // print uint8_t's
    printf("R: %hhu\n", CLAMP_255((red >> 4))); // print uint16_t
    printf("\033[48;2;%hhu;%hhu;%hhum      \033[0m", CLAMP_255((red >> 4)), CLAMP_255((green >> 4)), CLAMP_255((blue >> 4))); // print uint8_t's
    printf("G: %hu\n", CLAMP_255((green >> 4))); // print uint16_t
    printf("\033[48;2;%hhu;%hhu;%hhum      \033[0m", CLAMP_255((red >> 4)), CLAMP_255((green >> 4)), CLAMP_255((blue >> 4))); // print uint8_t's
    printf("B: %hu\n",  CLAMP_255((blue >> 4))); // print uint16_t
    fflush(NULL);
}

int main()
{
  	// Robot setup
	pynq_init();
	comms_init();
	edge_sensors_init();
  	stepper_init();
  	// connect IIC pins for the sensors that use them
  	switchbox_set_pin(IO_AR_SCL, SWB_IIC0_SCL);
	switchbox_set_pin(IO_AR_SDA, SWB_IIC0_SDA);
	iic_init(IIC0);
	// connect distance sensors
	vl53x distance_sensor_lower;
	vl53x distance_sensor_upper;
	vl53x distance_sensor_middle;
  	if (distance_sensor_init(&distance_sensor_lower, &distance_sensor_upper, &distance_sensor_middle) != 0) {return EXIT_FAILURE;}
	//connect color sensor
	printf("Now for color sensor... Enter to continue\n");
	getchar();
  	tcs3472 color_sensor;
  	if (color_sensor_init(&color_sensor) != 0) {printf("Color sensor fucked\n");} // return EXIT_FAILURE;}
  	stepper_enable();
  	stepper_set_speed(30000, 30000);

  	// Code for execution
  	uint32_t iDistance;
  	for (int i=0; i<300; i++) // read values 20 times a second for 1 minute
	{
		iDistance = tofReadDistance(&distance_sensor_lower);
		printf("lower distance = %dmm   ", iDistance);
		iDistance = tofReadDistance(&distance_sensor_middle);
		printf("middle distance = %dmm   ", iDistance);
		iDistance = tofReadDistance(&distance_sensor_upper);
		printf("upper distance = %dmm\n", iDistance);
	}

  	tcsReading rgb;
	printf("        \n        \n        \n"); // Buffer some space
	for (int i = 0; i < 1200; i++)
	{
		i = tcs_get_reading(&color_sensor, &rgb);
		print_colour(rgb.red, rgb.green, rgb.blue); //Used to print colour to screen
		sleep_msec(INTEGRATION_TIME_MS + 20);
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
	tcs_set_integration(&sensor, tcs3472_integration_from_ms(INTEGRATION_TIME_MS));
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
	sleep_msec(INTEGRATION_TIME_MS);
  	*color_sensor = sensor;
  	return 0;
}

int distance_sensor_init(vl53x* distance_sensor_lower, vl53x* distance_sensor_upper, vl53x* distance_sensor_middle)
{
	int debug;
	// set up lower sensor
	printf("Initialising Sensor A:\n");
	debug = tofSetAddress(IIC0, 0x29, TOF_ADDRESS_LOW);
	if (debug != 0) {
		printf("Failed to change lower distance sensor address\n");
		return 1;
	}
	printf("Changed lower distance sensor address\n");
	debug = tofPing(IIC0, TOF_ADDRESS_LOW);
	if(debug != 0)
	{
		printf("Failed to ping lower distance sensor\n");
		return 1;
	}
	printf("Successfully pinged lower distance sensor\n");
	// Create a sensor struct
	vl53x sensorA;
	// Initialize the sensor
	debug = tofInit(&sensorA, IIC0, TOF_ADDRESS_LOW, 0);
	if (debug != 0)
	{
		printf("Failed to initialize lower distance sensor\n");
		return 1;
	}
	printf("Successfully initialized lower distance sensor\n");
	printf("\n\nNow Power Sensor B!!\nPress \"Enter\" to continue...\n");
	getchar();
	// Setup upper sensor
	printf("Initialising upper distance sensor\n");
	// Change address for upper sensor too
	debug = tofSetAddress(IIC0, 0x29, TOF_ADDRESS_HIGH);
	if (debug != 0) {
		printf("Failed to change lower distance sensor address\n");
		return 1;
	}
	// Ping upper sensor
	debug = tofPing(IIC0, TOF_ADDRESS_HIGH);
	if(debug != 0)
	{
		printf("Failed to ping upper distance sensor\n");
		return 1;
	}
	printf("Successfully pinged upper distance sensor\n");
	// Create a sensor struct
	vl53x sensorB;
	// Initialize the sensor
	debug = tofInit(&sensorB, IIC0, TOF_ADDRESS_HIGH, 0);
	if (debug != 0)
	{
		printf("Failed to initialize upper distance sensor\n");
		return 1;
	}
	printf("\n\nNow Power Sensor C!!\nPress \"Enter\" to continue...\n");
	getchar();
	// Setup upper sensor
	printf("Initialising middle distance sensor\n");
	// Change address for upper sensor too
	debug = tofSetAddress(IIC0, 0x29, TOF_ADDRESS_MIDDLE);
	if (debug != 0) {
		printf("Failed to change middle distance sensor address\n");
		return 1;
	}
	// Ping upper sensor
	debug = tofPing(IIC0, TOF_ADDRESS_MIDDLE);
	if(debug != 0)
	{
		printf("Failed to ping middle distance sensor\n");
		return 1;
	}
	printf("Successfully pinged middle distance sensor\n");
	// Create a sensor struct
	vl53x sensorC;
	// Initialize the sensor
	debug = tofInit(&sensorC, IIC0, TOF_ADDRESS_MIDDLE, 0);
	if (debug != 0)
	{
		printf("Failed to initialize middle distance sensor\n");
		return 1;
	}
	printf("All distance sensors are running\n");
	fflush(NULL);
	*distance_sensor_lower = sensorA;
	*distance_sensor_upper = sensorB;
	*distance_sensor_middle = sensorC;
	return 0;
}