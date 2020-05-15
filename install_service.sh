#!/bin/bash

sudo cp drv8835-server.service /lib/systemd/system/
sudo systemctl enable drv8835-server.service
sudo systemctl start drv8835-server.service
