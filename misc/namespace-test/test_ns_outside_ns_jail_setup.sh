
set -x

namespace_pid="$1"

ip link add name veth0 type veth peer name veth1 netns $namespace_pid
ifconfig veth0 inet 12.0.0.1

#iptables -A INPUT -i veth0 -p tcp --dport 22 -j ACCEPT
#iptables -A INPUT -i veth0 -p icmp -j ACCEPT
#iptables -A INPUT -i veth0 -j DROP
