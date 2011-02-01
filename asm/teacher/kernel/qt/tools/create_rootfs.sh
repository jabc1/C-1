#!/bin/bash
#!/bin/sh
#root dir
mkdir bin sbin lib etc dev sys proc tmp var opt mnt usr home root media
#usr sub dir
cd usr
mkdir bin sbin lib local
#usr/local sub dir
cd local
mkdir bin sbin lib
cd ../..
#etc sub dir
cd etc
touch inittab
touch fstab
touch profile
touch passwd
touch group
touch shadow
touch resolv.conf
touch mdev.conf
touch inetd.conf
mkdir rc.d
mkdir init.d
touch init.d/rcS
chmod +x init.d/rcS
mkdir sysconfig
touch sysconfig/HOSTNAME
cd ..
#dev sub dir
cd dev
mknod console c 5 1
chmod 777 console
mknod null c 1 3
chmod 777 null
cd ..
#var sub dir
cd var
mkdir log
cd ..

#create lib if dynamic compile busybox
cp -f /usr/local/arm/4.3.2/arm-none-linux-gnueabi/libc/armv4t/lib/*so* lib -a
cp -f /usr/local/arm/4.3.2/arm-none-linux-gnueabi/libc/armv4t/usr/lib/*so* usr/lib -a


