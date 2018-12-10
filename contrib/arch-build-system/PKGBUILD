# Maintainer: Claus Klingberg <cjk at pobox dot com>
# Contributor: vapourismo <ole at vprsm dot de>
_pkgname=knxd
pkgname='knxd-git'
pkgver=1647.r672ca39
pkgrel=1
pkgdesc='A server which provides an interface to a KNX/EIB installation'
license=('GPL')

arch=('i686' 'x86_64' 'arm' 'armv6h' 'armv7h')
conflicts=('knxd' 'eibd')
provides=('knxd')
replaces=('eibd')
depends=('gcc-libs')
optdepends=('libsystemd' 'libusb')
makedepends=('cmake' 'git' 'libtool' 'autoconf' 'automake')

source=("git://github.com/knxd/${_pkgname}.git")
sha512sums=('SKIP')
url="https://github.com/knxd/knxd"

pkgver() {
  cd "$srcdir/$_pkgname"
  printf "%s.r%s" "$(git rev-list --count HEAD)" "$(git rev-parse --short HEAD)"
}

build() {
  cd "$srcdir/$_pkgname"
  ./bootstrap.sh
  ./configure \
    --prefix="/usr" \
    --sysconfdir=/etc \
    --libdir=/usr/lib \
    --libexecdir=/usr/lib \
    --enable-usb
  make
}

package() {
  backup=(etc/knxd.conf)

  cd "$srcdir/$_pkgname"
  make DESTDIR="$pkgdir/" install
}
