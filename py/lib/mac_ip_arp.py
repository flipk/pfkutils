#!/usr/bin/env python3

import os
import sys
import socket
import struct
import fcntl
import time

SIOCGIFADDR = 0x8915   # ioctl to get IP address, see linux C headers
ETH_P_ARP = 0x0806
ARP_OP_REPLY = 2

def get_mac_address(ifname):
    try:
        with open(f"/sys/class/net/{ifname}/address", 'r') as f:
            mac_str = f.read().strip()
        return mac_str, bytes.fromhex(mac_str.replace(':', ''))
    except FileNotFoundError:
        print(f"Error: interface {ifname} does not exist.")
        return None, None
    except Exception as e:
        print(f"Error reading MAC: {e}")
        return None, None

def get_ip_address(ifname):
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        ifreq = struct.pack('256s', ifname.encode('utf-8')[:15])
        res = fcntl.ioctl(s.fileno(), SIOCGIFADDR, ifreq)
        # the IPv4 address position in the struct
        ip_bytes = res[20:24]
        return socket.inet_ntoa(ip_bytes), ip_bytes
    except OSError:
        # if the interface doesn't have an address
        return None, None
    except Exception as e:
        print(f"Error reading IP: {e}")
        return None, None

def is_interface_running(ifname):
    try:
        with open(f"/sys/class/net/{ifname}/carrier", 'r') as f:
            state = f.read().strip()
        if state == '1':
            return True
    except Exception as e:
        print(f'exception: {e}')
    return False

def send_garp_raw(ifname, ip_bytes, mac_bytes):
    try:
        bcast_mac = b'\xff\xff\xff\xff\xff\xff'
        eth_hdr = struct.pack("!6s6sH", bcast_mac, mac_bytes, ETH_P_ARP)
        arp_hdr = struct.pack("!HHBBH6s4s6s4s",
                              1,  # hw type ethernet
                              0x0800, # protocol IPv4
                              6,  # hw addr len
                              4,  # protocol addr len
                              ARP_OP_REPLY,
                              mac_bytes,
                              ip_bytes,
                              bcast_mac,
                              ip_bytes)
        packet = eth_hdr + arp_hdr

        s = socket.socket(socket.AF_PACKET,
                          socket.SOCK_RAW,
                          socket.htons(ETH_P_ARP))
        s.bind((ifname, 0))
        s.send(packet)
        s.close()
    except PermissionError:
        print("permission denied, you need root do send garps")
        return False
    except Exception as e:
        print(f"failed to send ARP: {e}")
        return False
    return True
    
def _test_main():
    print('--scanning interfaces')
    ifnames = []
    with os.scandir("/sys/class/net/") as d:
        de: os.DirEntry
        for de in d:
            n = de.name
            if n == 'lo':
                print('skipping loopback')
                continue
            elif n.startswith('docker'):
                print(f'skipping {n}')
                continue
            ifnames.append(de.name)
    for ifname in ifnames:
        print(f'--doing interface: {ifname}')
        mac, mac_bytes = get_mac_address(ifname)
        if mac is None:
            print('no MAC on this interface, skipping')
            continue
        print(f'mac = {mac}')
        ip, ip_bytes = get_ip_address(ifname)
        if ip is None:
            print('no IP on this interface, skipping')
            continue
        print(f'ip = {ip}')
        up = is_interface_running(ifname)
        print(f'up = {up}')
        if not up:
            print('interface is down, skipping')
        print(f'doing garps on {ifname}')
        if not send_garp_raw(ifname, ip_bytes, mac_bytes):
            return 1
        time.sleep(0.1)
        if not send_garp_raw(ifname, ip_bytes, mac_bytes):
            return 1
        time.sleep(0.1)
        if not send_garp_raw(ifname, ip_bytes, mac_bytes):
            return 1

    return 0


if __name__ == '__main__':
    exit(_test_main())
