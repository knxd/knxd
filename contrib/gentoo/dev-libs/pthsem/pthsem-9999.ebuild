EAPI=2

inherit autotools git-2 eutils flag-o-matic

EGIT_REPO_URI="http://www.auto.tuwien.ac.at/~mkoegler/git/pthsem.git"

DESCRIPTION="extended version of GNU pth (user mode multi threading library)"
HOMEPAGE="http://www.auto.tuwien.ac.at/~mkoegler/index.php/pth"

LICENSE="LGPL-2.1"
SLOT="0"
KEYWORDS=""
IUSE=""

DEPEND=""
RDEPEND=""

src_configure() {
    cd "${S}"
    ( use arm || use sh ) && append-flags -U_FORTIFY_SOURCE
    eautoreconf
    econf || die "econf failed"
}

src_compile() {
    cd "${S}"
    emake || die "make failed"
}

src_install() {
    emake DESTDIR="${D}" install || die "install failed"
    dodoc ANNOUNCE AUTHORS ChangeLog NEWS README THANKS USERS
}
