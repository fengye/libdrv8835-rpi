// Requires root

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <math.h>
#include <wiringPi.h>

static const int MOTOR0 = 0;
static const int MOTOR1 = 1;
static const int MAX_SPEED = 480;
static const int PIN_MOTOR_PWM[] = {12, 13};
static const int PIN_MOTOR_DIR[] = {5, 6};
static int target_speeds[] = {0, 0};
static int current_speeds[] = {0, 0};
static float step_speeds[] = {0.0f, 0.0f};

static int GPIO_start() 
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

    syslog(LOG_NOTICE, "GPIO initialised.");
    return 0;
}

static void calc_steps(int motor)
{
    if (motor < 0 || motor > 1)
        return;

    int diff = target_speeds[motor] - current_speeds[motor];
    step_speeds[motor] = (float)diff / 15.0f; 
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
    if (speed < 0)
        dir = 1;

    digitalWrite(PIN_MOTOR_DIR[motor], dir);
    pwmWrite(PIN_MOTOR_PWM[motor], speed); 

    target_speeds[motor] = speed;
    current_speeds[motor] = speed;
    calc_steps(motor);
}

void set_speed(int motor, int speed)
{
    if (motor < 0 || motor > 1)
        return;

    if (speed > MAX_SPEED)
        speed = MAX_SPEED;
    if (speed < -MAX_SPEED)
        speed = -MAX_SPEED;

    if ((speed > 0 && current_speeds[motor] < 0) ||
        (speed < 0 && current_speeds[motor] > 0))
    {
        // Change direction so we stop first
        current_speeds[motor] = 0;
        pwmWrite(PIN_MOTOR_PWM[motor], 0);
    }

    target_speeds[motor] = speed;
    calc_steps(motor);

    // digitalWrite(PIN_MOTOR_DIR[motor], dir);
    // pwmWrite(PIN_MOTOR_PWM[motor], current_speeds[motor]); 
}

static void check_root()
{
    if(geteuid() != 0)
    {
        fprintf(stderr, "This daemon requires root.\n");
        exit(EXIT_FAILURE);
    }
}

static void skeleton_daemon()
{
    pid_t pid;

    /* Fork off the parent process */
    pid = fork();

    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* On success: The child process becomes session leader */
    if (setsid() < 0)
        exit(EXIT_FAILURE);

    /* Catch, ignore and handle signals */
    //TODO: Implement a working signal handler */
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    /* Fork off for the second time*/
    pid = fork();

    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* Set new file permissions */
    umask(0);

    /* Change the working directory to the root directory */
    /* or another appropriated directory */
    chdir("/");

    /* Close all open file descriptors */
    int x;
    for (x = sysconf(_SC_OPEN_MAX); x>=0; x--)
    {
        close (x);
    }

    /* Open the log file */
    openlog ("drv8835_daemon", LOG_PID, LOG_DAEMON);
}

int main()
{
    check_root(); 
    skeleton_daemon();

    if (GPIO_start() != 0)
    {
        fprintf(stderr, "GPIO initialisation failure, try sudo?");
        return EXIT_FAILURE;
    }
    // test run
    syslog(LOG_NOTICE, "drv8835_daemon self test started.");
    set_speed_direct(MOTOR0, MAX_SPEED/2);
    set_speed_direct(MOTOR1, -MAX_SPEED/2);
    usleep(200000);
    set_speed_direct(MOTOR0, -MAX_SPEED/2);
    set_speed_direct(MOTOR1, MAX_SPEED/2);
    usleep(200000);
    set_speed_direct(MOTOR0, 0);
    set_speed_direct(MOTOR1, 0);
    syslog(LOG_NOTICE, "drv8835_daemon self test ended.");

    syslog(LOG_NOTICE, "drv8835_daemon started.");
    while (1)
    {
        for(int i = 0; i < 2; ++i)
        {
            if (target_speeds[i] != current_speeds[i])
            {
                 if (fabs(target_speeds[i] - current_speeds[i]) <= fabs(step_speeds[i]))
                 {
                     current_speeds[i] = target_speeds[i];
                 }
                 else
                 {
                     current_speeds[i] += step_speeds[i];
                 }
                 int dir = 0;
                 if (current_speeds[i] < 0)
                     dir = 1;
                 digitalWrite(PIN_MOTOR_DIR[i], dir);
                 pwmWrite(PIN_MOTOR_PWM[i], current_speeds[i]); 
            }
        }

        // 32ms sleep ~= 30 fps
        usleep (32000);
    }

    syslog (LOG_NOTICE, "drv8835_daemon terminated.");
    closelog();

    return EXIT_SUCCESS;
}
