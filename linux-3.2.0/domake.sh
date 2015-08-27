#!/bin/bash
#
# Copyright (c) 2013-2014, SeeCare.CVTE
# All rights reserved.
#
##############################################
# Author	: QiuWeibo
# Version	: V1.0.0
# Date		: 2014-6-10
# Comment	: script for make kernel for aPM-MCU
##############################################
# Author	: QiuWeibo
# Version	: V1.0.1
# Date		: 2014-6-10
# Comment	: add script for make install

echo "domake starting..."
case "$1" in
  a)
	echo "make alls"
	make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- mrproper
	make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- aPM_MPU_defconfig
	make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- menuconfig
	make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- uImage
#	make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- modules
#	make INSTALL_MOD_PATH=${HOME}/tftproot/modules modules_install
	;;
  u)
	echo "only uImage to make"
	make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- uImage
	;;
  c)
	echo "clean object file"
	make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- mrproper
	;;
  m)
	echo "only modules to make"
	make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- modules
	make INSTALL_MOD_PATH=${HOME}/tftproot/modules modules_install
	;;
  *)
	echo "do nothing..."
esac

if [ "$2" = "cp" ]; then
	echo "coping to ~/tftproot/"
	cp -f ./arch/arm/boot/uImage /home/qiuweibo/tftproot/
	ls -l /home/qiuweibo/tftproot/uImage
fi

echo "domake exit success!"
exit 0
