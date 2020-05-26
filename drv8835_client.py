#!/usr/bin/python3

import ctypes
import pathlib

libname = str(pathlib.Path().absolute() / "libdrv8835-shared.so")
c_lib = ctypes.CDLL(libname)
c_lib.drv8835_client_connect.argtypes=[ctypes.c_char_p, ctypes.c_int]
c_lib.drv8835_client_send_motor_param.argtypes=[ctypes.c_ubyte, ctypes.c_short]
c_lib.drv8835_client_send_motor_params.argtypes=[ctypes.c_ubyte, ctypes.c_short, ctypes.c_ubyte, ctypes.c_short]

def connect(host, port):
    return c_lib.drv8835_client_connect(host.encode('utf-8'), port)

def disconnect():
    return c_lib.drv8835_client_disconnect()

def is_connected():
    return c_lib.drv8835_client_is_connected()

def send_motor_param(motor, param):
    return c_lib.drv8835_client_send_motor_param(motor, param)

def send_motor_params(motor0, param0, motor1, param1):
    return c_lib.drv8835_client_send_motor_params(motor0, param0, motor1, param1)

DEFAULT_PORT = ctypes.cast(c_lib.DEFAULT_PORT, ctypes.POINTER(ctypes.c_int))
DEFAULT_PORT = DEFAULT_PORT.contents.value

MOTOR0 = ctypes.cast(c_lib.MOTOR0, ctypes.POINTER(ctypes.c_int))
MOTOR0 = MOTOR0.contents.value

MOTOR1 = ctypes.cast(c_lib.MOTOR1, ctypes.POINTER(ctypes.c_int))
MOTOR1 = MOTOR1.contents.value

MAX_SPEED = ctypes.cast(c_lib.MAX_SPEED, ctypes.POINTER(ctypes.c_int))
MAX_SPEED = MAX_SPEED.contents.value

if __name__ == "__main__":
    print("Loading " + str(libname))
    print("Done")
