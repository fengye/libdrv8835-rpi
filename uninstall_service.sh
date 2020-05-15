#!/bin/bash

sudo systemctl stop drv8835-server.service
sudo systemctl disable drv8835-server.service
sudo rm /lib/systemd/system/drv8835-server.service
