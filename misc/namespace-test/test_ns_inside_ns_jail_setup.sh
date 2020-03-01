
set -x

# assume ${newroot} is set by test_ns prior to entry

ifconfig veth1 inet 12.0.0.2

# umounts here

rm -rf              ${newroot}
mkdir               ${newroot}
mkdir               ${newroot}/etc
mkdir               ${newroot}/bin
mkdir               ${newroot}/lib64

cd /etc
cp -p host.conf hosts ld.so.cache ld.so.conf nsswitch.conf protocols services ${newroot}/etc

cd /bin
cp bash ls cat    ${newroot}/bin

cd /sbin
cp ifconfig       ${newroot}/bin

cd /lib64
cp libtinfo.so.6 libdl.so.2 libc.so.6 ld-linux-x86-64.so.2 libselinux.so.1 libcap.so.2 libpcre2-8.so.0 libpthread.so.0   ${newroot}/lib64

mkdir               ${newroot}/dev
mknod               ${newroot}/dev/zero c 1 5
mknod               ${newroot}/dev/null c 1 3
mknod               ${newroot}/dev/urandom c 1 9
mknod               ${newroot}/dev/random c 1 8
mknod               ${newroot}/dev/tty c 5 0
chmod 666           ${newroot}/dev/*

mkdir               ${newroot}/proc 
mount -t proc proc  ${newroot}/proc 

(

    echo root:x:0:0:root:/:/bin/false
    echo user:x:9001:9001::/home/user:/bin/false

)  > ${newroot}/etc/passwd

(

    echo root:x:0:
    echo user:x:9001:

) > ${newroot}/etc/group

mkdir -p            ${newroot}/home/user
chmod 700           ${newroot}/home/user
chown 9001:9001     ${newroot}/home/user

#ln -s /proc/mounts ${newroot}/etc/mtab

echo NONAME > ${newroot}/etc/hostname

chmod 444 ${newroot}/etc/passwd ${newroot}/etc/group ${newroot}/etc/hostname
