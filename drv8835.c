#include "drv8835.h"
#include <stdint.h>
#include <stdbool.h>
#include <wiringPi.h>
#include "types.h"
#include "common.h"
#include "motor_server.h"
#include "socket_server.h"

result_t drv8835_server_init()
{
	if (!motor_server_is_initialised())
	{
		if (motor_server_init() != 0)
		{
			log_error("Motor server initialisation failed.");
			return -1;
		}
	}

	if (motor_server_start() != 0)
	{
		log_error("Cannot start motor server thread.");
		return -1;
	}

	return 0;
}

result_t drv8835_server_quit()
{
	result_t result = 0;
	if (socket_server_stop() != 0)
	{
		result = -1;
	}

	if (motor_server_stop() != 0)
	{
		result = -1;
	}

	return result;
}

result_t drv8835_server_wait_till_quit()
{
	result_t result = 0;
	if (socket_server_wait_till_stop() != 0)
	{
		result = -1;
	}

	if (motor_server_stop() != 0)
	{
		result = -1;
	}

	return result;
}


result_t drv8835_server_set_motor_param(uint8_t motor, int16_t param)
{
	if (motor_server_set_speed((int)motor, (int)param) != 0)
		return -1;

	return 0;
}

result_t drv8835_server_set_motor_params(uint8_t motor1, int16_t param1, uint8_t motor2, int16_t param2)
{
	if (motor_server_set_speed((int)motor1, (int)param1) != 0)
		return -1;
	if (motor_server_set_speed((int)motor2, (int)param2) != 0)
		return -1;
	return 0;
}

result_t drv8835_server_listen(int port)
{
	if (!socket_server_is_initialised())
	{
		if (socket_server_init(port) != 0)
			return -1;
	}

	if (socket_server_start() != 0)
		return -1;

	return 0;
}


