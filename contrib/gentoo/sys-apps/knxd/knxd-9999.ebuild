# Copyright 1999-2015 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $
# Author: Michael Kefeder (m.kefeder@gmail.com)
# Author: Patrik Pfaffenbauer (patrik.pfaffenbauer@p3.co.at)
# Author: Marc Joliet (marcec@gmx.de)

EAPI="5"

inherit eutils autotools git-2

DESCRIPTION="Provides an interface to the EIB / KNX bus (latest git)"
HOMEPAGE="https://github.com/Makki1/knxd"

LICENSE="GPL-2"
SLOT="9999"
KEYWORDS=""
IUSE="eibd ft12 pei16s tpuarts eibnetip eibnetiptunnel eibnetipserver usb groupcache java ncn5120 dummy"

DEPEND=""

EGIT_REPO_URI="https://github.com/Makki1/knxd.git"

src_prepare() {
	eautoreconf || die "eautotooling failed"
}

src_configure() {
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
