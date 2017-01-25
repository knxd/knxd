# This is a knxd portage overlay for Gentoo Linux

###To use it on your computer

1. Create the local repo
   ```$ mkdir -p /usr/portage/local/knxd```
2. checkout the overlay (using svn is easiest method I think)
   ```$ svn co https://github.com/Makki1/knxd/trunk/contrib/gentoo /usr/portage/local/knxd```
3. Edit ```/etc/portage/make.conf``` and add this line 
   ```source /usr/portage/local/knxd/make.conf```

###To help with debugging

1. remove -ggdb from your CFLAGS and splitdebug from your FEATURES in make.conf.
2. create a file: ```/etc/portage/env/splitdebug.conf``` 

        CFLAGS="${CFLAGS} -ggdb" 
        CXXFLAGS="${CXXFLAGS} -ggdb" 
        FEATURES="${FEATURES} splitdebug"

3. in ```/etc/portage/package.env```, you can "execute" splitdebug.conf for the packages you want.
   in our case it looks like this:

        sys-apps/knxd splitdebug.conf

4. emerge knxd
5. check if it worked, there should exist many files in ```/usr/lib/debug``` for example: ```/usr/lib/debug/usr/bin/knxd.debug```
