#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>
//#include "libdrv8835.h"
#include <pthread.h>
#include "common.h"

typedef struct _motor_param
{
    int left;
    int right;
} motor_param;

static const int MOTOR0 = 0;
static const int MOTOR1 = 1;
static const int MAX_SPEED = 480;
// 16ms sleep ~= 60 fps
static const int WAIT_INTERVAL = 1000*16;
static const float INTERP_SEGS = 30.0f;
static const int PIN_MOTOR_PWM[] = {12, 13};
static const int PIN_MOTOR_DIR[] = {5, 6};

static pthread_t server_thread = 0;
static motor_param the_motor_param;
static float tar_param[2];
static float curr_param[2];
static float step_speeds[2];
static bool server_running;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static int drv8835_init() 
{
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

//    syslog(LOG_NOTICE, "GPIO initialised.");
    return 0;
}

static void calc_steps(int motor)
{
    if (motor < 0 || motor > 1)
        return;

    int diff = tar_param[motor] - curr_param[motor];
    step_speeds[motor] = (float)diff / INTERP_SEGS; 
}

static void set_speed_direct(int motor, int speed)
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

    pthread_mutex_lock(&mutex);

    digitalWrite(PIN_MOTOR_DIR[motor], dir);
    pwmWrite(PIN_MOTOR_PWM[motor], newSpeed); 

    tar_param[motor] = speed;
    curr_param[motor] = speed;
    calc_steps(motor);

    pthread_mutex_unlock(&mutex);
}

void set_speed(int motor, int speed)
{
    if (motor < 0 || motor > 1)
        return;

    if (speed > MAX_SPEED)
        speed = MAX_SPEED;
    if (speed < -MAX_SPEED)
        speed = -MAX_SPEED;

    pthread_mutex_lock(&mutex);
    printf("Set speed: %d - %d\n", motor, speed);

    if ((speed > 0 && curr_param[motor] < 0) ||
        (speed < 0 && curr_param[motor] > 0))
    {
        // Change direction so we stop first
        curr_param[motor] = 0;
        pwmWrite(PIN_MOTOR_PWM[motor], 0);
    }

    tar_param[motor] = speed;
    calc_steps(motor);

    pthread_mutex_unlock(&mutex);
}


static void server_quit()
{
    printf("Signal quit\n");
    pthread_mutex_lock(&mutex);
    server_running = false;
    pthread_mutex_unlock(&mutex);
}

static void *server_loop(void* ptr)
{
    while(1)
    {
        pthread_mutex_lock(&mutex);
        if (!server_running)
		{
			pthread_mutex_unlock(&mutex);
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

        printf("Left: %d, Right: %d\n", the_motor_param.left, the_motor_param.right);
        pthread_mutex_unlock(&mutex);
        usleep (WAIT_INTERVAL);
    }
    printf("Stopping motor 0...");
    set_speed_direct(MOTOR0, 0);
    printf(" Done.\n");
    printf("Stopping motor 1...");
    set_speed_direct(MOTOR1, 0);
    printf(" Done.\n");

    return NULL;
}

static void _init()
{
    drv8835_init();
    server_running = true;
    the_motor_param.left = 0;
    the_motor_param.right = 0;
    tar_param[0] = 0.0f;
    tar_param[1] = 0.0f;
    curr_param[0] = 0.0f;
    curr_param[1] = 0.0f;
    set_speed_direct(MOTOR0, 0);
    set_speed_direct(MOTOR1, 0);
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

int main(int argc, char *argv[]) {
    check_root();
    _init(); 

    atexit(_force_stop);
    signal(SIGINT, _on_interrupt);

    if (pthread_create(&server_thread, NULL, server_loop, &the_motor_param) != 0)
    {
		fprintf(stderr, "Create server thread failed, exiting...\n");
		exit(EXIT_FAILURE);
    }
    printf("Set to 50, 80\n");
    set_speed(MOTOR0, 50);
    set_speed(MOTOR1, 80);
    usleep(1000*2000);
    printf("Set to 150, 180\n");
    set_speed(MOTOR0, 150);
    set_speed(MOTOR1, 180);
    usleep(1000*2000);
    printf("Set to -150, -180\n");
    set_speed(MOTOR0, -150);
    set_speed(MOTOR1, -180);
    usleep(1000*2000);
    printf("Set to 0, 0\n");
    set_speed(MOTOR0, 0);
    set_speed(MOTOR1, 0);
    usleep(1000*2000);
    printf("Set to MAX, MAX\n");
    set_speed(MOTOR0, MAX_SPEED);
    set_speed(MOTOR1, MAX_SPEED);
    usleep(1000*2000);
    printf("Set to 0, 0\n");
    set_speed(MOTOR0, 0);
    set_speed(MOTOR1, 0);
    usleep(1000*2000);
    printf("Set to -MAX, -MAX\n");
    set_speed(MOTOR0, -MAX_SPEED);
    set_speed(MOTOR1, -MAX_SPEED);
    usleep(1000*2000);
    server_quit();
    pthread_join(server_thread, NULL);
    usleep(1000*100);
    printf("Exit\n");
    exit(0);
}
