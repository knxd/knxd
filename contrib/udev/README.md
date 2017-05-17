udev rule file for KNX devices
==============================

The common rules file for knxd is now located in debian/knxd.udev.
See the head of that file for instructions if your device is not
recognized, or if you have more than one hardware interface.

Devices for KNX access will have access rights for user *knxd* which give
the KNXd process (if started as user knxd) read and write access to the
devices. It therefore does not to need to run as root. This is compatible
with systemd files and the debian packages.
