if HAVE_SYSTEMD
systemdsystemunit_DATA=knxd.service knxd.socket knxnetip.socket
sysconf_DATA=knxd.conf
systemdsysusers_DATA=sysusers.d/knxd.conf
all: knxd.service.exp
knxd.service.exp: knxd.service
	$(SED) -e 's,[$$]{prefix},$(prefix),g' -e 's,[$$]{exec_prefix},$(exec_prefix),g' < $< > $@
	cp -p $@ $<
endif

EXTRA_DIST=knxd.conf sysusers.d/knxd.conf knxd.udev

