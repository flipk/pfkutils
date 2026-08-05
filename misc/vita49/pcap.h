#ifndef __PCAP_H__
#define __PCAP_H__ 1

#include <sys/time.h>

#define PCAP_MAGIC 0xa1b2c3d4
#define PCAP_MAJOR_V 2
#define PCAP_MINOR_V 4
#define PCAP_LINKTYPE_ETH 1

struct pcap_file_header {
    uint32_t magic;           // a1b2c3d4
    uint16_t version_major;   // 2
    uint16_t version_minor;   // 4
    uint32_t thiszone;        // 0 
    uint32_t sigfigs;         // 0 
    uint32_t snaplen;         // 1500 
    uint32_t linktype;        // 1 -- DLT_EN10MB or DLT_RAW, in pcap/dlt.h
};

struct pcap_pkthdr {
    uint32_t time_sec;
    uint32_t time_usec;
    uint32_t caplen;     // length of portion in the file
    uint32_t len;        // actual length of packet
    uint8_t * data[0];
};

static inline bool build_pcap_header(
    uint8_t * output_pcap_buffer,
    uint32_t * output_pcap_len)
{
    pcap_file_header * ph = (pcap_file_header *) output_pcap_buffer;

    if (*output_pcap_len < sizeof(pcap_file_header))
        return false;
    *output_pcap_len = sizeof(pcap_file_header);

    ph->magic = PCAP_MAGIC;
    ph->version_major = PCAP_MAJOR_V;
    ph->version_minor = PCAP_MINOR_V;
    ph->thiszone = 0;
    ph->sigfigs = 0;
    ph->snaplen = 1500;
    ph->linktype = PCAP_LINKTYPE_ETH;

    return true;
}

static inline bool build_pcap_packet(
    const uint8_t * packet_contents,
    uint32_t packet_length,
    uint8_t * output_pcap_buffer,
    uint32_t * output_pcap_len)
{
    struct timeval tv;
    pcap_pkthdr * ph = (pcap_pkthdr *) output_pcap_buffer;
    uint32_t needed = packet_length + sizeof(pcap_pkthdr);

    if (*output_pcap_len < needed)
        return false;
    *output_pcap_len = needed;

    gettimeofday(&tv, NULL);

    printf("time: %u.%06u\n", (uint32_t) tv.tv_sec, (uint32_t) tv.tv_usec);

    ph->time_sec = tv.tv_sec;
    ph->time_usec = tv.tv_usec;
    ph->caplen = packet_length;
    ph->len = packet_length;
    memcpy(ph->data, packet_contents, packet_length);

    return true;
}

#endif // __PCAP_H__
