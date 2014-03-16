#Designed by Wang Bin(Lucas Wang). 2013 <wbsecg1@gmail.com>

##TODO: Why use function to add makefile target failed
#defineTest(packageSet) {
#	isEmpty(1): warning("packageSet(version [, name]")
#	PACKAGE_VERSION = $$1
#	isEmpty(2): PACKAGE_NAME = $$basename(_PRO_FILE_PWD_)
#	else: PACKAGE_NAME = $$2


symbian {
    TARGET.UID3 = 0xea6f847b
    # TARGET.CAPABILITY += 
    TARGET.EPOCSTACKSIZE = 0x14000
    TARGET.EPOCHEAPSIZE = 0x020000 0x800000
}


OTHER_FILES += README.md TODO \
	qtc_packaging/common/README \
	qtc_packaging/common/copyright \
	qtc_packaging/common/changelog \
	qtc_packaging/debian_harmattan/manifest.aegis \
	qtc_packaging/debian_harmattan/control \
	qtc_packaging/debian_harmattan/rules \
	qtc_packaging/debian_harmattan/compat \
	qtc_packaging/debian_harmattan/$${PACKAGE_NAME}.desktop \
	qtc_packaging/debian_fremantle/control \
	qtc_packaging/debian_fremantle/rules \
	qtc_packaging/debian_fremantle/compat \
	qtc_packaging/debian_fremantle/$${PACKAGE_NAME}.desktop \
	qtc_packaging/debian_generic/control \
	qtc_packaging/debian_generic/$${PACKAGE_NAME}.desktop

#export(OTHER_FILES)

unix {

# Add files and directories to ship with the application
# by adapting the examples below.
# file1.source = myfile
# dir1.source = mydir
#lang.files = i18n
#DEPLOYMENTFOLDERS = lang# file1 dir1
#include(deployment.pri)
#qtcAddDeployment()
#TODO: add other platform packaging

!isEmpty(MEEGO_VERSION_MAJOR): PLATFORM = harmattan #armMEEGO_VERSION_MAJORel
else:maemo5: PLATFORM = fremantle
else: PLATFORM = generic

ARCH = `dpkg --print-architecture`
*maemo*: ARCH = $$QT_ARCH
#ARCH = $$QT_ARCH  #what about harmattan?

fakeroot.target = fakeroot
fakeroot.depends = FORCE
fakeroot.commands = $$quote(rm -rf fakeroot && mkdir -p fakeroot/usr/share/doc/$$PACKAGE_NAME && mkdir -p fakeroot/DEBIAN)
fakeroot.commands += $$quote(chmod -R 755 fakeroot)  ##control dir must be 755
fakeroot.commands = $$join(fakeroot.commands,$$escape_expand(\\n\\t))

deb.target = deb
deb.depends += fakeroot
deb.commands += $$quote(make install INSTALL_ROOT=\$\$PWD/fakeroot)
deb.commands += $$quote(cd fakeroot; md5sum `find usr -type f |grep -v DEBIAN` > DEBIAN/md5sums; cd -)
deb.commands += $$quote(cp $$PWD/qtc_packaging/debian_$${PLATFORM}/control fakeroot/DEBIAN)
deb.commands += $$quote(sed -i \"s/%arch%/$${ARCH}/\" fakeroot/DEBIAN/control)
deb.commands += $$quote(sed -i \"s/%version%/$${PACKAGE_VERSION}/\" fakeroot/DEBIAN/control)
deb.commands += $$quote(gzip -9 fakeroot/usr/share/doc/$$PACKAGE_NAME/changelog)
deb.commands += $$quote(dpkg -b fakeroot $${PACKAGE_NAME}_$${PACKAGE_VERSION}_$${PLATFORM}_$${ARCH}.deb)
deb.commands = $$join(deb.commands,$$escape_expand(\\n\\t))

#message("deb.commands: $$deb.commands")
QMAKE_EXTRA_TARGETS += fakeroot deb
#export(QMAKE_EXTRA_TARGETS)
		#makeDeb($$PACKAGE_VERSION, $$PACKAGE_NAME)
}
#} #packageSet
