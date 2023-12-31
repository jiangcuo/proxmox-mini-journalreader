include /usr/share/dpkg/pkg-info.mk
include /usr/share/dpkg/architecture.mk

PACKAGE := proxmox-mini-journalreader

GITVERSION:=$(shell git rev-parse HEAD)

BUILDDIR ?= $(PACKAGE)-$(DEB_VERSION_UPSTREAM)

DEB=$(PACKAGE)_$(DEB_VERSION_UPSTREAM_REVISION)_$(DEB_BUILD_ARCH).deb
DBGDEB=$(PACKAGE)-dbgsym_$(DEB_VERSION_UPSTREAM_REVISION)_$(DEB_BUILD_ARCH).deb
DEBS=$(DEB) $(DBGDEB)

DSC=$(PACKAGE)_$(DEB_VERSION_UPSTREAM_REVISION).dsc

all: $(DEB)

$(BUILDDIR): src debian
	rm -rf $(BUILDDIR)
	rsync -a src/ debian $(BUILDDIR)
	echo "git clone git://git.proxmox.com/git/proxmox-mini-journal\\ngit checkout $(GITVERSION)" > $(BUILDDIR)/debian/SOURCE

.PHONY: deb
deb: $(DEB)
$(DBGDEB): $(DEB)
$(DEB): $(BUILDDIR)
	cd $(BUILDDIR); dpkg-buildpackage -b -us -uc
	lintian $(DEBS)

.PHONY: dsc
dsc: $(DSC)
$(DSC): $(BUILDDIR)
	cd $(BUILDDIR); dpkg-buildpackage -S -us -uc -d
	lintian $(DSC)

sbuild: $(DSC)
	sbuild $(DSC)

dinstall: $(DEB)
	dpkg -i $(DEB)

.PHONY: clean
clean:
	rm -rf $(PACKAGE)-[0-9]*/ *.deb *.buildinfo *.build *.changes *.dsc *.tar.*

.PHONY: upload
upload: UPLOAD_DIST ?= $(DEB_DISTRIBUTION)
upload: $(DEBS)
	tar cf - $(DEBS)|ssh -X repoman@repo.proxmox.com -- upload --product pve,pmg,pbs --dist $(UPLOAD_DIST) --arch $(DEB_BUILD_ARCH)
