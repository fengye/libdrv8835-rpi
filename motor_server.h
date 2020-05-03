#pragma once
#include "types.h"
#include <wiringPi.h>

const int MOTOR0 = 0;
const int MOTOR1 = 1;
const int MAX_SPEED = 480;
// 16ms sleep ~= 60 fps
const int WAIT_INTERVAL = 1000*16;
const float INTERP_SEGS = 30.0f;
const int PIN_MOTOR_PWM[] = {12, 13};
const int PIN_MOTOR_DIR[] = {5, 6};


extern result_t motor_server_init();
extern result_t motor_server_start();
extern result_t motor_server_stop();
extern result_t motor_server_set_speed(int motor, int speed);

