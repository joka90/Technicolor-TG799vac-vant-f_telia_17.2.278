ifeq ($(strip $(BRCM_USER_SSP)),y)
CFLAGS=$(SSP_MIN_COMPILER_OPTS)
LDFLAGS=-L$(INSTALL_DIR)/lib/public $(SSP_LIBS) 
export CFLAGS
export LDFLAGS
endif

all:
	if [ ! -e ./source/Makefile ];then \
	cd ./source; ./configure --host=mips-linux-uclibc --target=mips-linux-uclibc --disable-developer --enable-cups=no --enable-fam=no --enable-shared=yes --enable-iprint=no --enable-pie=no --without-ldap --without-cifsupcall --without-cifsmount --without-cluster-support --without-acl-support --with-privatedir=/var/samba --with-lockdir=/var/samba/locks --with-configdir=/var/samba --with-piddir=/var/samba --with-logfilebase=/var/samba SMB_BUILD_CC_NEGATIVE_ENUM_VALUES=no linux_getgrouplist_ok=no samba_cv_have_longlong=yes samba_cv_HAVE_IFACE_IFCONF=yes ; \
	fi

	echo $(CFLAGS)
	$(warning $(CFLAGS))
	$(warning $(LDFLAGS))
	$(MAKE) -C ./source srcdir=./ builddir=./
	install -m 755 ./source/bin/smbd $(INSTALL_DIR)/bin/smbd
	install -m 755 ./source/bin/smbpasswd $(INSTALL_DIR)/bin/smbpasswd
	install -m 444 ./source/conf/smb.conf $(INSTALL_DIR)/etc

clean:
	if [ -f ./source/Makefile ];then $(MAKE) -C ./source clean distclean srcdir=./ builddir=./;fi;

