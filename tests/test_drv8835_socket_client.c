#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include "drv8835.h"
#include "common.h"

static void _on_interrupt()
{
	drv8835_client_disconnect();
	sleep(1);
	exit(0);
}

int main(int argc, char *argv[]) {

	int time = -1;

	if (argc > 2 && strcmp(argv[1], "--time") == 0)
	{
		time = atoi(argv[2]);
	}


	if (drv8835_client_connect("localhost", DEFAULT_PORT) != 0)
		exit(EXIT_FAILURE);

    signal(SIGINT, _on_interrupt);

	if (time >= 0)
	{
		log_info("Client will run for %d seconds", time);
		sleep(time);
		if (drv8835_client_disconnect() != 0)
			exit(EXIT_FAILURE);
	}
	else
	{
		do 
		{
			drv8835_client_send_motor_params(MOTOR0, MAX_SPEED, MOTOR1, MAX_SPEED);
			usleep(1000*400);
			drv8835_client_send_motor_params(MOTOR0, -MAX_SPEED, MOTOR1, -MAX_SPEED);
			usleep(1000*400);

		} while(1);
	}

	log_info("Exit test.");
	exit(0);
}
