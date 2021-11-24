include /usr/share/dpkg/pkg-info.mk
include /usr/share/dpkg/architecture.mk

PACKAGE := proxmox-mini-journalreader

GITVERSION:=$(shell git rev-parse HEAD)

BUILDDIR ?= ${PACKAGE}-${DEB_VERSION_UPSTREAM}

DEB=${PACKAGE}_${DEB_VERSION_UPSTREAM_REVISION}_${DEB_BUILD_ARCH}.deb
DBGDEB=${PACKAGE}-dbgsym_${DEB_VERSION_UPSTREAM_REVISION}_${DEB_BUILD_ARCH}.deb
DSC=${PACKAGE}_${DEB_VERSION_UPSTREAM_REVISION}.dsc

all: $(DEB)

$(BUILDDIR): src debian
	rm -rf $(BUILDDIR)
	rsync -a src/ debian $(BUILDDIR)
	echo "git clone git://git.proxmox.com/git/proxmox-mini-journal\\ngit checkout $(GITVERSION)" > $(BUILDDIR)/debian/SOURCE

.PHONY: deb
deb: $(DEB)
$(DEB): $(BUILDDIR)
	cd $(BUILDDIR); dpkg-buildpackage -b -us -uc
	lintian $(DEB)

.PHONY: dsc
dsc: ${DSC}
${DSC}: ${BUILDDIR}
	cd ${BUILDDIR}; dpkg-buildpackage -S -us -uc -d
	lintian ${DSC}

dinstall: $(DEB)
	dpkg -i $(DEB)

.PHONY: clean
clean:
	rm -rf $(BUILDDIR) *.deb *.buildinfo *.changes *.dsc *.tar.gz

.PHONY: upload
upload: ${DEB}
	tar cf - ${DEB}|ssh -X repoman@repo.proxmox.com -- upload --product pve,pmg --dist bullseye --arch ${DEB_BUILD_ARCH}
