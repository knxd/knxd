# This is a knxd portage overlay for Gentoo Linux

###To use it on your computer

1. Create the local repo
   ```$ mkdir -p /usr/portage/local/knxd```
2. checkout the overlay (using svn is easiest method I think)
   ```$ svn co https://github.com/Makki1/knxd/trunk/contrib/gentoo /usr/portage/local/knxd```
3. Edit ```/etc/portage/make.conf``` and add this line 
   ```source /usr/portage/local/knxd/make.conf```
