#!/usr/bin/env python3

from drv8835rpi import drv8835_client
import time
from argparse import *

if __name__ == '__main__':
    parser = ArgumentParser(description='Test drv8835 client')
    parser.add_argument('--host', default="localhost", required=False, help='Host machine runs drv8835 server')
    parser.add_argument('--port', type=int, default=drv8835_client.DEFAULT_PORT, required=False, help='Host socket port') 
    args = parser.parse_args()
    if drv8835_client.connect(args.host, args.port) == 0:
        drv8835_client.send_motor_params(0, 480, 1, 480)
        time.sleep(5)
        drv8835_client.send_motor_params(0, -480, 1, -480)
        time.sleep(5)
        drv8835_client.disconnect()
    else:
        print("Cannot connect to {}:{}".format(args.host, args.port))


