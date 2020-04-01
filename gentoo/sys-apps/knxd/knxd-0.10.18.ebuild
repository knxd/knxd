# Copyright 1999-2015 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $
# Author: Michael Kefeder (m.kefeder@gmail.com)
# Author: Patrik Pfaffenbauer (patrik.pfaffenbauer@p3.co.at)
# Author: Marc Joliet (marcec@gmx.de)

EAPI="5"

inherit eutils autotools git-r3 user

DESCRIPTION="Provides an interface to the EIB / KNX bus"
HOMEPAGE="https://github.com/Makki1/knxd"

LICENSE="GPL-2"
SLOT="0"
KEYWORDS=""
IUSE="eibd ft12 pei16s tpuarts eibnetip eibnetiptunnel eibnetipserver usb groupcache java ncn5120 dummy"

DEPEND="dev-libs/pthsem
    usb? ( dev-libs/libusb )
    java? ( virtual/jdk )
	"

EGIT_REPO_URI="https://github.com/knxd/knxd.git"
EGIT_COMMIT="v${PV}"

src_prepare() {
    eautoreconf || die "eautotooling failed"
}

src_configure() {
#  works for me with the pth tests
#        --without-pth-test \
    econf \
        $(use_enable ft12) \
        $(use_enable pei16s) \
        $(use_enable tpuarts) \
        $(use_enable eibnetip) \
        $(use_enable eibnetiptunnel) \
        $(use_enable eibnetipserver) \
        $(use_enable usb) \
        $(use_enable java) \
        $(use_enable ncn5120) \
        $(use_enable groupcache) || die "econf failed"

}

src_compile() {
    emake || die "build of knxd failed"
}

src_install() {
    emake DESTDIR="${D}" install

    einfo "Installing init-script and config"

    sed -e "s|@SLOT@|${SLOT}|g" \
           "${FILESDIR}/${PN}.init" | newinitd - ${PN}-${SLOT}

    sed -e "s|@SLOT@|${SLOT}|g" \
           "${FILESDIR}/${PN}.confd" | newconfd - ${PN}-${SLOT}
}

pkg_setup() {
	enewgroup knxd
	enewuser knxd -1 -1 -1 "knxd"
}
