
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
    struct timeval ts;   // uint32 seconds, uint32 microseconds
    uint32_t caplen;     // length of portion in the file
    uint32_t len;        // actual length of packet
};
