#pragma once
#include "types.h"
#include <wiringPi.h>



extern result_t motor_server_init();
extern result_t motor_server_start();
extern result_t motor_server_stop();
extern result_t motor_server_set_speed(int motor, int speed);

