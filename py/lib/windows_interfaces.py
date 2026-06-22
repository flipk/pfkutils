
# requires 'pip install WMI'
import wmi

def get_windows_interfaces():
    c = wmi.WMI()
    interfaces = []

    # Query only adapters that currently have IP routing enabled
    for config in c.Win32_NetworkAdapterConfiguration(IPEnabled=True):
        
        # Correlate configuration with the physical/logical adapter 
        # to get the Windows Network Connection ID
        adapter = c.Win32_NetworkAdapter(Index=config.Index)[0]
        
        # Separate IPv4 and IPv6 addresses
        ipv4_addresses = [ip for ip in config.IPAddress if '.' in ip] if config.IPAddress else []
        ipv6_addresses = [ip for ip in config.IPAddress if ':' in ip] if config.IPAddress else []

        interface_data = {
            "name": adapter.NetConnectionID or config.Description,
            "hardware_description": config.Description,
            "mac_address": config.MACAddress,
            "ipv4_addresses": ipv4_addresses,
            "ipv6_addresses": ipv6_addresses,
            "netmasks": list(config.IPSubnet) if config.IPSubnet else [],
            "routers": list(config.DefaultIPGateway) if config.DefaultIPGateway else [],
            "dhcp_enabled": config.DHCPEnabled,
            "dns_servers": list(config.DNSServerSearchOrder) if config.DNSServerSearchOrder else []
        }
        
        interfaces.append(interface_data)

    return interfaces

if __name__ == "__main__":
    active_interfaces = get_windows_interfaces()
    desc_len = 1
    ip_len = 1
    rtr_len = 1
    for i in active_interfaces:
        desc = i['hardware_description']
        ip = i['ipv4_addresses'][0]
        if len(i['routers']) > 0:
            rtr = i['routers'][0]
        else:
            rtr = '<none>'
        if len(desc) > desc_len:
            desc_len = len(desc)
        if len(ip) > ip_len:
            ip_len = len(ip)
        if len(rtr) > rtr_len:
            rtr_len = len(rtr)
    print(f'\n{"Description":{desc_len}s}  {"IPADDR":{ip_len}s}  Router')
    print(f'{"-"*desc_len}  {"-"*ip_len}  {"-"*rtr_len}')
    for i in active_interfaces:
        desc = i['hardware_description']
        ip = i['ipv4_addresses'][0]
        if len(i['routers']) > 0:
            rtr = i['routers'][0]
        else:
            rtr = '<none>'
        print(f'{desc:{desc_len}s}  {ip:{ip_len}s}  {rtr:{rtr_len}s}')
    print('\n')
