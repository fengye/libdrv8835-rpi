#include "motor_server.h"
#include "types.h"
#include "drv8835_util.h"
#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>
#include <pthread.h>
#include "common.h"

// 16ms sleep ~= 60 fps
const int WAIT_INTERVAL = 1000*16;
const float INTERP_SEGS = 30.0f;

typedef struct _motor_param
{
    int left;
    int right;
} motor_param;

static pthread_t server_thread = 0;
static motor_param the_motor_param;
static float tar_param[2];
static float curr_param[2];
static float step_speeds[2];
static bool server_init = false;
static bool server_running = false;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static result_t _init_gpio()
{
	if (!drv8835_is_root())
	{
		drv8835_log_error("To run drv8835 server, you must be root.");
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

    drv8835_log_info("GPIO initialised.");
    return 0;
}

static void _calc_steps(int motor)
{
    if (motor < 0 || motor > 1)
        return;

    int diff = tar_param[motor] - curr_param[motor];
    step_speeds[motor] = (float)diff / INTERP_SEGS; 
}

static void _set_speed_direct(int motor, int speed)
{
    if (motor < 0 || motor > 1)
        return;

    if (speed > MAX_SPEED)
        speed = MAX_SPEED;
    if (speed < -MAX_SPEED)
        speed = -MAX_SPEED;

    int dir = 0;
    int newSpeed = speed;
    if (speed < 0)
    {
        dir = 1;
		newSpeed = -speed;
    }

    LOCK(&mutex);

    digitalWrite(PIN_MOTOR_DIR[motor], dir);
    pwmWrite(PIN_MOTOR_PWM[motor], newSpeed); 

    tar_param[motor] = speed;
    curr_param[motor] = speed;
    _calc_steps(motor);

    UNLOCK(&mutex);
}

static void _set_speed(int motor, int speed)
{
    if (motor < 0 || motor > 1)
        return;

    if (speed > MAX_SPEED)
        speed = MAX_SPEED;
    if (speed < -MAX_SPEED)
        speed = -MAX_SPEED;

    LOCK(&mutex);
    drv8835_log_debug("Set target speed: motor%d=%d", motor, speed);

    if ((speed > 0 && curr_param[motor] < 0) ||
        (speed < 0 && curr_param[motor] > 0))
    {
        // Change direction so we stop first
        curr_param[motor] = 0;
        pwmWrite(PIN_MOTOR_PWM[motor], 0);
    }

    tar_param[motor] = speed;
    _calc_steps(motor);

    UNLOCK(&mutex);
}

static void *_server_loop(void* ptr)
{
	LOCK(&mutex);
	server_running = true;
	UNLOCK(&mutex);

    while(1)
    {
        LOCK(&mutex);
        if (!server_running)
		{
			UNLOCK(&mutex);
			break;
		}

		for(int i = 0; i < 2; ++i)
        {
            if (tar_param[i] != curr_param[i])
            {
				if (fabs(tar_param[i] - curr_param[i]) <= fabs(step_speeds[i]))
				{
					curr_param[i] = tar_param[i];
				}
                else
                {
                    curr_param[i] += step_speeds[i];
                }
				int dir = 0;
				int speed = curr_param[i];
				if (curr_param[i] < 0)
				{
					dir = 1;
		     		speed = -curr_param[i];
		 		}
				digitalWrite(PIN_MOTOR_DIR[i], dir);
				pwmWrite(PIN_MOTOR_PWM[i], speed); 

				the_motor_param.left = (int)curr_param[0];
				the_motor_param.right = (int)curr_param[1];
            }
        }
        //drv8835_log_info("Left: %d, Right: %d", the_motor_param.left, the_motor_param.right);
        UNLOCK(&mutex);
        usleep (WAIT_INTERVAL);
    }
    drv8835_log_info_nocr("Stopping motor 0...");
    _set_speed_direct(MOTOR0, 0);
    drv8835_log_info(" Done.");
    drv8835_log_info_nocr("Stopping motor 1...");
    _set_speed_direct(MOTOR1, 0);
    drv8835_log_info(" Done.");

    return NULL;
}

static void _force_stop()
{
    digitalWrite(PIN_MOTOR_DIR[MOTOR0], 0);
    pwmWrite(PIN_MOTOR_PWM[MOTOR0], 0); 
    digitalWrite(PIN_MOTOR_DIR[MOTOR1], 0);
    pwmWrite(PIN_MOTOR_PWM[MOTOR1], 0);
}

static void _on_interrupt()
{
    _force_stop();
    exit(0);
}

result_t motor_server_init()
{
	result_t result = 0;
    LOCK(&mutex);

	if (_init_gpio() !=  0)
	{
		result = -1;
		goto END;
	}
    server_running = false;
    the_motor_param.left = 0;
    the_motor_param.right = 0;
    tar_param[0] = 0.0f;
    tar_param[1] = 0.0f;
    curr_param[0] = 0.0f;
    curr_param[1] = 0.0f;


	atexit(_force_stop);
	signal(SIGINT, _on_interrupt);

	server_init = true;
END:
	UNLOCK(&mutex);

	// force initial stop state
	if (result == 0)
	{
    	_set_speed_direct(MOTOR0, 0);
    	_set_speed_direct(MOTOR1, 0);
	}
	return result;
}

bool motor_server_is_initialised()
{
	bool result;
	LOCK(&mutex);
	result = server_init;
	UNLOCK(&mutex);
	return result;
}

result_t motor_server_start()
{
	result_t result = 0;
	LOCK(&mutex);
	if (server_running)
	{
		drv8835_log_error("Motor server is already running.");
		result = -1;
	}
	UNLOCK(&mutex);
	if (result < 0)
		return result;

    if (pthread_create(&server_thread, NULL, _server_loop, &the_motor_param) != 0)
    {
		drv8835_log_error("Create server thread failed, exiting...");
		return -1;
    }

	return 0;
}

result_t motor_server_stop()
{
	result_t result = 0;

    LOCK(&mutex);
	if (server_running)
	{
		drv8835_log_info("Signal motor server quit");
		server_running = false;
	}
	else
	{
		drv8835_log_error("Motor server is NOT running yet.");
		result = -1;
	}
    UNLOCK(&mutex);
	if (result < 0)
		return result;

    result = pthread_join(server_thread, NULL);

	if (result < 0)
		return result;
	
	drv8835_log_info("Motor server successfully stopped.");
	return 0;
}

result_t motor_server_force_stop()
{
	result_t result = 0;

    LOCK(&mutex);
	if (server_running)
	{
		drv8835_log_info("Signal motor server quit");
		server_running = false;
	}
	else
	{
		drv8835_log_error("Motor server is NOT running yet.");
		result = -1;
	}
    UNLOCK(&mutex);
	if (result < 0)
		return result;

    result = pthread_cancel(server_thread);

	if (result < 0)
		return result;
	
	drv8835_log_info("Motor server successfully terminated.");
	return 0;
}

result_t motor_server_wait_till_stop()
{
	result_t result = 0;

    LOCK(&mutex);
	bool is_running = server_running;
	UNLOCK(&mutex);
	if (is_running)
	{
		result = pthread_join(server_thread, NULL);

		if (result < 0)
			return result;
		
	}
	drv8835_log_info("Motor server successfully stopped.");
	return 0;
}

result_t motor_server_set_speed(int motor, int speed)
{
	result_t result = 0;
	LOCK(&mutex);
	if (!server_running)
	{
		drv8835_log_error("Motor server is NOT running, setting speed will have no effect");
		result = -1;
	}
	UNLOCK(&mutex);
	if (result < 0)
		return result;

	_set_speed(motor, speed);
	return 0;
}
