#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include "drv8835.h"

int main(int argc, char *argv[]) {

	if (drv8835_server_init() != 0)
		exit(EXIT_FAILURE);

    printf("Set to 50, 80\n");
    drv8835_server_set_motor_params(MOTOR0, 50, MOTOR1, 80);
    usleep(1000*2000);
    printf("Set to 150, 180\n");
    drv8835_server_set_motor_param(MOTOR0, 150);
    drv8835_server_set_motor_param(MOTOR1, 180);
    usleep(1000*2000);
    printf("Set to -150, -180\n");
    drv8835_server_set_motor_params(MOTOR0, -150, MOTOR1, -180);
    usleep(1000*2000);
    printf("Set to 0, 0\n");
    drv8835_server_set_motor_params(MOTOR0, 0, MOTOR1, 0);
    usleep(1000*2000);
    printf("Set to MAX, MAX\n");
    drv8835_server_set_motor_params(MOTOR0, MAX_SPEED, MOTOR1, MAX_SPEED);
    usleep(1000*2000);
    printf("Set to 0, 0\n");
    drv8835_server_set_motor_param(MOTOR0, 0);
    drv8835_server_set_motor_param(MOTOR1, 0);
    usleep(1000*2000);
    printf("Set to -MAX, -MAX\n");
    drv8835_server_set_motor_param(MOTOR0, -MAX_SPEED);
    drv8835_server_set_motor_param(MOTOR1, -MAX_SPEED);
    usleep(1000*2000);
    if (drv8835_server_quit() != 0)
		exit(EXIT_FAILURE);

    usleep(1000*100);
    printf("Exit\n");
    exit(0);
}
