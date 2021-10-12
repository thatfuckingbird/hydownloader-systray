pkgname=hydownloader-systray-git
pkgver=r85.7da1535
pkgrel=1
pkgdesc="Remote management GUI for hydownloader."
arch=('i686' 'x86_64')
url="https://github.com/thatfuckingbird/hydownloader-systray"
depends=('qt6-base')
makedepends=('cmake' 'qt6-tools' 'qt6-base')
provides=('hydownloader-systray')
source=("$pkgname"::"git+https://github.com/thatfuckingbird/hydownloader-systray.git")
sha512sums=('SKIP')

pkgver() {
  cd "$srcdir/$pkgname"
  printf "r%s.%s" "$(git rev-list --count HEAD)" "$(git rev-parse --short HEAD)"
}

build() {
  cd "$srcdir/$pkgname"
  git submodule update --init
  cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release .
  make
}

package() {
 cd "$srcdir/$pkgname"
 DESTDIR="$pkgdir" make install
 #install -Dm644 "icon.png" "${pkgdir}/usr/share/icons/hydownloader-systray.png"
 #install -Dm644 "hydownloader-systray.desktop" "${pkgdir}/usr/share/applications/hydownloader-systray.desktop"
}
