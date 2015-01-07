include $(TOPDIR)/rules.mk

PKG_NAME:=eibd
PKG_VERSION:=0.0.4.16
PKG_RELEASE:=4

PKG_SOURCE:=bcusdk_0.0.4+nmu16.tar.gz
# $(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://repo.wiregate.de/wiregate/pool/main/b/
PKG_MD5SUM:=d92f34a703d79421d90fa4e1258ca235

PKG_FIXUP:=libtool
PKG_INSTALL:=1

#PKG_BUILD_DEPENDS:=pthsem argp-standalone
#PKG_BUILD_DEPENDS:=pthsem uClibc++ libxml2 libusb argp-standalone
PKG_BUILD_DEPENDS:=pthsem uClibc++ argp-standalone

#PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
PKG_BUILD_DIR:=$(BUILD_DIR)/bcusdk

include $(INCLUDE_DIR)/package.mk

define Package/eibd/Default
  SECTION:=net
  CATEGORY:=Network
  TITLE:=eibd EIB/KNX 
  URL:=http://www.auto.tuwien.ac.at/~mkoegler/index.php/eibd
endef

define Package/libeibclient
  $(call Package/eibd/Default)
  DEPENDS:=+pthsem +uclibcxx
  TITLE+= libeibclient
endef
define Package/eibd-server
  $(call Package/eibd/Default)
  DEPENDS:=+pthsem uclibcxx
  TITLE+= server daemon
endef
define Package/eibd-bcuaddrtab
  $(call Package/eibd/Default)
  DEPENDS:=+pthsem uclibcxx
  TITLE+= bcuaddrtab
endef
define Package/eibd-knxtool
  $(call Package/eibd/Default)
  DEPENDS:=+pthsem uclibcxx libeibclient
  TITLE+= knxtool
endef
define Package/eibd-cgi
  $(call Package/eibd/Default)
  DEPENDS:=+pthsem uclibcxx libeibclient lighttpd lighttpd-mod-setenv lighttpd-mod-cgi
  TITLE+= cgi-clients
endef

define Build/Prepare
#markstaller@v1520-mm:~/openwrt/trunk$ cp build_dir/target-mips_r2_uClibc-0.9.32/pthsem-2.0.8/pthsem.m4 /home/markstaller/openwrt/trunk/staging_dir/#target-mips_r2_uClibc-0.9.32/usr/share/aclocal/
# dirty hack for useless libtool:
	$(CP) $(PKG_BUILD_DIR)/../pthsem-2.0.8/pthsem.m4 $(STAGING_DIR)/usr/share/aclocal/
	$(call Build/Prepare/Default)
endef

CONFIGURE_ARGS += \
	--enable-onlyeibd \
	--enable-eibnetiptunnel \
	--enable-eibnetip \
	--enable-eibnetipserver \
	--enable-tpuarts \
	--enable-ft12 \
	--disable-tpuart \
	--disable-usb \
	--disable-pei16 \
	--disable-pei16s \
	--enable-groupcache \
	--with-pth=yes \
	--without-pth-test \
	--without-libstdc 

#define Build/Compile
#	$(MAKE) -C $(PKG_BUILD_DIR) \
#		CC=$(TARGET_CC) \
#		LIBS="-L$(STAGING_DIR)/usr/lib -fno-builtin -nostdinc++ -nodefaultlibs -luClibc++ -lc -lm -lgcc -largp -lpthsem" \
#		CPPFLAGS="-I$(STAGING_DIR)/include -I$(STAGING_DIR)/usr/include"
#		# -I$(LINUX_DIR)/include"
#endef
MAKE_FLAGS += \
	CFLAGS="$(TARGET_CFLAGS) -Wall -D_REENTRANT" \
	LIBS="-L$(STAGING_DIR)/usr/lib -fno-builtin -nostdinc++ -nodefaultlibs -luClibc++ -lc -lm -lgcc -largp -lpthsem" \
	CPPFLAGS="-I$(STAGING_DIR)/include -I$(STAGING_DIR)/usr/include" \
	all

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/eibclient.h $(1)/usr/include/
	$(CP) $(PKG_INSTALL_DIR)/usr/include/eibloadresult.h $(1)/usr/include/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libeibclient.{a,so*} $(1)/usr/lib/
endef

define Package/libeibclient/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libeibclient.so.* $(1)/usr/lib/
endef

# FIXME: put/read eibd-config into/from UCI

define Package/eibd-server/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/eibd/server/eibd $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/eibd.init $(1)/etc/init.d/eibd
#	$(INSTALL_DIR) $(1)/etc/default
#	$(INSTALL_BIN) ./files/eibd.default $(1)/etc/default/eibd
endef
define Package/eibd-server/postinst
#!/bin/sh
# check if we are on real system
if [ -z "$${IPKG_INSTROOT}" ]; then
        echo "Enabling rc.d symlink for eibd"
        /etc/init.d/eibd enable
        /etc/init.d/eibd start
fi
exit 0
endef

define Package/eibd-server/prerm
#!/bin/sh
# check if we are on real system
if [ -z "$${IPKG_INSTROOT}" ]; then
        echo "Removing rc.d symlink for eibd"
        /etc/init.d/eibd disable
        /etc/init.d/eibd stop
fi
exit 0
endef

define Package/eibd-bcuaddrtab/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/eibd/bcu/bcuaddrtab $(1)/usr/bin/
endef
define Package/eibd-cgi/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/eib*cgi $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/www/cgi-bin
	ln -s /usr/bin/eibread-cgi $(1)/www/cgi-bin/r
	ln -s /usr/bin/eibwrite-cgi $(1)/www/cgi-bin/w
	$(INSTALL_BIN) ./files/cgi.l.sh $(1)/www/cgi-bin/l
endef
define Package/eibd-knxtool/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/knxtool $(1)/usr/bin/
	ln -s knxtool $(1)/usr/bin/knxread
	ln -s knxtool $(1)/usr/bin/knxreadtemp
	ln -s knxtool $(1)/usr/bin/knxwrite
	ln -s knxtool $(1)/usr/bin/knxlog
	ln -s knxtool $(1)/usr/bin/knxon
	ln -s knxtool $(1)/usr/bin/knxoff
	ln -s knxtool $(1)/usr/bin/knxif
	ln -s knxtool $(1)/usr/bin/knxswrite
	ln -s knxtool $(1)/usr/bin/knxbool
	ln -s knxtool $(1)/usr/bin/knxdimup
endef

$(eval $(call BuildPackage,libeibclient))
$(eval $(call BuildPackage,eibd-server))
$(eval $(call BuildPackage,eibd-bcuaddrtab))
$(eval $(call BuildPackage,eibd-knxtool))
$(eval $(call BuildPackage,eibd-cgi))

