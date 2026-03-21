#!/usr/bin/env python3

import subprocess
import re
import ipaddress
from dataclasses import dataclass, field
from typing import List

@dataclass
class IPv4Config:
    """Stores a single IPv4 address and its corresponding netmask."""
    address: str
    netmask: str

@dataclass
class EthernetInterface:
    """Describes a network interface and its IPv4 configurations."""
    name: str
    is_up: bool
    is_running: bool
    # A list is used here to handle interfaces with multiple IPv4 addresses
    ipv4_configs: List[IPv4Config] = field(default_factory=list)

def get_ethernet_interfaces() -> List[EthernetInterface]:
    """
    Fetches IP, netmask, UP, and RUNNING states for all ethernet interfaces.
    Runs as non-root by parsing `ip addr show`.
    """
    interfaces = []
    current_iface_data = None
    
    # Run the standard `ip` command available on Ubuntu
    try:
        result = subprocess.run(
            ['ip', 'addr', 'show'], 
            capture_output=True, text=True, check=True
        )
    except subprocess.CalledProcessError as e:
        print(f"Failed to run 'ip' command: {e}")
        return []

    # Regex patterns to parse the output
    # Matches interface line, e.g., "2: eth0: <BROADCAST,MULTICAST,UP,LOWER_UP> ..."
    iface_re = re.compile(r'^\d+:\s+([^:@]+)(?:@[^:]+)?:\s+<([^>]+)>')
    # Matches MAC address line to verify it's a physical/virtual ethernet device (ignores loopback/tunnels)
    ether_re = re.compile(r'^\s+link/ether\s+')
    # Matches IPv4 line, e.g., "    inet 192.168.1.5/24 brd ..."
    inet_re = re.compile(r'^\s+inet\s+([0-9\.]+)/(\d+)')

    for line in result.stdout.splitlines():
        # 1. Look for a new interface definition
        iface_match = iface_re.match(line)
        if iface_match:
            # If we were tracking a previous interface and it was Ethernet, save it
            if current_iface_data and current_iface_data['is_ethernet']:
                interfaces.append(EthernetInterface(
                    name=current_iface_data['name'],
                    is_up=current_iface_data['is_up'],
                    is_running=current_iface_data['is_running'],
                    ipv4_configs=current_iface_data['ipv4_configs']
                ))
            
            # Parse flags for the new interface
            flags = iface_match.group(2).split(',')
            current_iface_data = {
                'name': iface_match.group(1).strip(),
                'is_up': 'UP' in flags,
                # 'LOWER_UP' is the modern Linux iproute2 indicator for a running physical link
                'is_running': 'RUNNING' in flags or 'LOWER_UP' in flags,
                'is_ethernet': False,
                'ipv4_configs': []
            }
            continue

        # 2. Gather data for the current interface
        if current_iface_data:
            # Check if it's an ethernet device
            if ether_re.match(line):
                current_iface_data['is_ethernet'] = True
            else:
                # Check for IPv4 addresses
                inet_match = inet_re.match(line)
                if inet_match:
                    ip = inet_match.group(1)
                    cidr_prefix = int(inet_match.group(2))
                    
                    # Convert CIDR (e.g., /24) to a standard dotted-decimal netmask
                    network = ipaddress.IPv4Network(f'0.0.0.0/{cidr_prefix}', strict=False)
                    netmask = str(network.netmask)
                    
                    current_iface_data['ipv4_configs'].append(IPv4Config(address=ip, netmask=netmask))

    # Don't forget to append the final interface in the loop
    if current_iface_data and current_iface_data['is_ethernet']:
        interfaces.append(EthernetInterface(
            name=current_iface_data['name'],
            is_up=current_iface_data['is_up'],
            is_running=current_iface_data['is_running'],
            ipv4_configs=current_iface_data['ipv4_configs']
        ))

    return interfaces

# --- Example Usage ---
if __name__ == "__main__":
    eth_interfaces = get_ethernet_interfaces()
    for iface in eth_interfaces:
        print(f"Interface: {iface.name}")
        print(f"  UP: {iface.is_up}, RUNNING: {iface.is_running}")
        for config in iface.ipv4_configs:
            print(f"  - IPv4: {config.address} (Netmask: {config.netmask})")


