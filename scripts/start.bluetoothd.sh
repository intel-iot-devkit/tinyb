#!/bin/sh

systemctl stop bluetooth
systemctl unmask bluetooth
systemctl start bluetooth
systemctl status bluetooth
hciconfig hci0 reset
