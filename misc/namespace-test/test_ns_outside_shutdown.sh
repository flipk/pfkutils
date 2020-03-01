
set -x

# assume ${newroot} is set by test_ns prior to entry

#iptables --flush
umount ${newroot}/proc
rm -rf ${newroot}
