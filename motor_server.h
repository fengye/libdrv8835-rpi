#pragma once
#include <stdbool.h>
#include "types.h"
#include <wiringPi.h>



extern result_t motor_server_init();
extern bool motor_server_is_initialised();
extern result_t motor_server_start();
extern result_t motor_server_stop();
extern result_t motor_server_force_stop();
extern result_t motor_server_wait_till_stop();
extern result_t motor_server_set_speed(int motor, int speed);

