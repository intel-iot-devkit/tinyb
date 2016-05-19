Solving common issues with TinyB                           {#troubleshooting}
======================

If you are having issues with TinyB, please follow these steps to resolve common issues:

1. Make sure bluetooth is not blocked. On most systems you can do this with ``` rfkill unblock bluetooth ```.

2. Make sure blueoothd daemon is started: ``` ps -eF | grep bluetoothd ``` or ``` ps | grep bluetoothd ``` if your ps command does not support parameters (such as BusyBox based systems). This command should a line containing bluetoothd.

  If the bluetooth daemon is not started, you should run ```systemctl start bluetooth.service``` on systems using systemd, or the equivalent for your distro.

3. Make sure you are running bluetoothd with the -E flag. It should be visible when running the commands in 1. If it is not present, you need to add it in ``` /lib/systemd/system/bluetooth.service ``` or the equivalent on your system.

3. Make sure that your DBus policy permits users to access BlueZ GATT interfaces. The following lines should be present in ``` /etc/dbus-1/system.d/bluetooth.conf ``` under ``` <policy context="default"> ```:
  ```
  <allow send_interface="org.bluez.GattService1"/>
  <allow send_interface="org.bluez.GattCharacteristic1"/>
  <allow send_interface="org.bluez.GattDescriptor1"/>
  ```

4. Make sure your kernel supports Bluetooth. This is sometimes hard to verify, here are some ways on how to do this:
  * ``` lsmod | grep bluetooth ``` should return a line containing bluetooth, if not, try ``` modprobe bluetooth ``` or ``` insmod bluetooth ```
  * ``` /proc/config ``` or ``` /proc/config.gz ``` or ``` /boot/config ``` should contain ``` CONFIG_BT=y ``` or ``` CONFIG_BT=m ``` and ``` CONFIG_BT_LE=y ```. If ``` CONFIG_BT=m ``` is enabled, make sure to load your module using ``` modprobe bluetooth ``` or ``` insmod bluetooth ```
  * ``` rfkill list ``` should show at least a line containing bluetooth, and it should not be blocked, if it is, see step 1.
