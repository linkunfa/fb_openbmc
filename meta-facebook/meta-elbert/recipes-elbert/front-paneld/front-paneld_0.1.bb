# Copyright 2021-present Facebook. All Rights Reserved.

inherit systemd

SUMMARY = "Front Panel Control Daemon"
DESCRIPTION = "Daemon to monitor and control the front panel"
SECTION = "base"
PR = "r1"
LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://front-paneld.c;beginline=5;endline=17;md5=da35978751a9d71b73679307c4d296ec"

DEPENDS:append = "libpal libkv libgpio-ctrl libsdr libobmc-i2c update-rc.d-native"

LOCAL_URI = " \
    file://Makefile \
    file://setup-front-paneld.sh \
    file://front-paneld.c \
    file://run-front-paneld.sh \
    file://front-paneld.service \
    "

binfiles = "front-paneld"

pkgdir = "front-paneld"

install_sysv() {
  install -d ${D}${sysconfdir}/init.d
  install -d ${D}${sysconfdir}/rcS.d
  install -d ${D}${sysconfdir}/sv
  install -d ${D}${sysconfdir}/sv/front-paneld
  install -d ${D}${sysconfdir}/front-paneld
  install -m 755 setup-front-paneld.sh ${D}${sysconfdir}/init.d/setup-front-paneld.sh
  install -m 755 run-front-paneld.sh ${D}${sysconfdir}/sv/front-paneld/run
  update-rc.d -r ${D} setup-front-paneld.sh start 67 5 .
}

install_systemd() {
  install -d ${D}${systemd_system_unitdir}
  install -m 644 front-paneld.service ${D}${systemd_system_unitdir}
}

do_install() {
  dst="${D}/usr/local/fbpackages/${pkgdir}"
  bin="${D}/usr/local/bin"
  install -d $dst
  install -d $bin
  install -m 755 front-paneld ${dst}/front-paneld
  ln -snf ../fbpackages/${pkgdir}/front-paneld ${bin}/front-paneld
  if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false', d)}; then
    install_systemd
  else
    install_sysv
  fi
}

FBPACKAGEDIR = "${prefix}/local/fbpackages"

FILES:${PN} = "${FBPACKAGEDIR}/front-paneld ${prefix}/local/bin ${sysconfdir} "
RDEPENDS:${PN} += "bash libpal libkv libgpio-ctrl libsdr libobmc-i2c libmisc-utils"
SYSTEMD_SERVICE:${PN} = "front-paneld.service"
LDFLAGS += "-lmisc-utils"
