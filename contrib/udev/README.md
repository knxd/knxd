udev rule file for KNX USB hid devices
======================================

This is a rule file to be put into `/etc/udev/rules.d`. Depending on your 
distribution you can choose a numeric prefix to control the priority of the 
already available rules in the directory (e.g. `90-knxd.rules`). USB devices for 
KNX access will have access rights for user *knxd* which give the KNXd process 
(if started as user knxd) read and write access to the devices and therefore has 
not to be run as root. This is compatible with systemd files and the debian 
packages.
