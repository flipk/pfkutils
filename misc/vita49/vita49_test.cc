#if 0
set -e -ex
g++ -Wall -Werror vita49_test.cc -o vt
./vt
rm -f vt
exit 0
;
#endif

#include <stdio.h>
#include "vita49.h"
#include "ethernet_packet_builder.h"
#include "pcap.h"

void print_buffer(uint8_t * buf, int len)
{
    for (int pos = 0; pos < len; pos++)
    {
        printf("%02x ", buf[pos]);
        if ((pos & 15) == 15)
            printf("\n");
        else if ((pos & 3) == 3)
            printf("  ");
    }
    printf("\n");
}

int main()
{
    Vita49Header   h(/*signal*/false);
    uint8_t v49buffer[256];
    int len;

    h.stream_id = 0x2004;
    h.timestamp_seconds = 0x1234;
    h.timestamp_picoseconds = 0x12345678;
    h.packet_count = 13;

    h.changed = true;

    h.bandwidth_present = true;
    h.bandwidth = 4e6;

    h.rf_ref_freq_present = true;
    h.freq = 12e6;

    h.gain_present = true;
    h.gain2 = 0;
    h.gain1 = 35;

    h.samplerate_present = true;
    h.sample_rate = 6e6;

    h.payloadformat_present = true;
    h.complex = true;

    len = h.encode(v49buffer, sizeof(v49buffer));

    uint8_t   ip_buffer[1500];
    uint32_t  ip_buffer_len = sizeof(ip_buffer);

    if (!build_ipv4_udp_packet(0x0a000002, 0x0a000001,
                               25004, 25004,
                               v49buffer, len,
                               ip_buffer, &ip_buffer_len))
    {
        printf("FAIL building UDP packet\n");
        return 1;
    }

    uint8_t  eth_buffer[1500];
    uint32_t eth_buffer_len = sizeof(eth_buffer);
    uint8_t  dest[6] = { 0x1a, 0x01, 0x55, 0x15, 0x88, 0x0a };
    uint8_t  src[6] =  { 0x3a, 0x02, 0x55, 0x17, 0x22, 0x0b };
    uint16_t ethertype = 0x0800;

    if (!build_ethernet_packet(dest, src, ethertype,
                               ip_buffer, ip_buffer_len,
                               eth_buffer, &eth_buffer_len))
    {
        printf("FAIL building ETH packet\n");
        return 1;
    }

    printf("eth packet: %08x bytes\n", eth_buffer_len);
    print_buffer(eth_buffer, eth_buffer_len);


    // for a double-check, output to a PCAP file so you can
    // open with wireshark and validate. (configure VITA49
    // protocol on port 25004).

    uint8_t  pcap_pkt[1500];
    uint32_t pcap_len = sizeof(pcap_pkt);

    if (!build_pcap_header(pcap_pkt, &pcap_len))
    {
        printf("FAIL building pcap header\n");
        return 1;
    }
    FILE * f = fopen("log.pcap", "wb");
    printf("writing log.pcap\n");
    fwrite(pcap_pkt, pcap_len, 1, f);

    pcap_len = sizeof(pcap_pkt);
    if (!build_pcap_packet(eth_buffer, eth_buffer_len,
                           pcap_pkt, &pcap_len))
    {
        printf("FAIL building pcap packet\n");
        return 1;
    }
    fwrite(pcap_pkt, pcap_len, 1, f);
    fclose(f);

    return 0;
}
