#include "drv8835.h"
#include <stdint.h>
#include <stdbool.h>
#include <wiringPi.h>
#include "types.h"
#include "common.h"
#include "motor_server.h"

static bool gpio_init = false;

result_t drv8835_GPIO_init()
{
	if (!is_root())
	{
		log_error("To run drv8835 server, you must be root.");
		return -1;
	}

	int init = wiringPiSetupGpio();
    if (init != 0)
        return init;

    pinMode(PIN_MOTOR_PWM[MOTOR0], PWM_OUTPUT);
    pinMode(PIN_MOTOR_PWM[MOTOR1], PWM_OUTPUT);

    pwmSetMode(PWM_MODE_MS);
    pwmSetRange(MAX_SPEED);
    pwmSetClock(2);

    pinMode(PIN_MOTOR_DIR[MOTOR0], OUTPUT);
    pinMode(PIN_MOTOR_DIR[MOTOR1], OUTPUT);

	gpio_init = true;
    log_info("GPIO initialised.");
    return 0;
}

result_t drv8835_server_init()
{
	if (!gpio_init)
	{
		log_error("GPIO must be initialised first.");
		return -1;
	}
	if (motor_server_init() != 0)
	{
		log_error("Motor server initialisation failed.");
		return -1;
	}

	return 0;
}

