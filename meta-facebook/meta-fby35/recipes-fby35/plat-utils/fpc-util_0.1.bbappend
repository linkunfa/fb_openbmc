# Copyright 2015-present Facebook. All Rights Reserved.
FILESEXTRAPATHS:prepend := "${THISDIR}/files/fpc-util:"
DEPENDS += " libbic libfby35-common"
RDEPENDS:${PN} += " libbic libfby35-common"
LDFLAGS += " -lbic -lfby35_common"
