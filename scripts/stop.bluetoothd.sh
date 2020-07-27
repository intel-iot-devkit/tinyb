#!/bin/sh

systemctl stop bluetooth
systemctl mask bluetooth
systemctl stop bluetooth
systemctl status bluetooth
hciconfig hci0 reset
