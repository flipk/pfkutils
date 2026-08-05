
#include <cstdint>
#include <cstring>

// Helper to compute standard internet checksum (RFC 1071)
// Returns the checksum in host byte order.
static inline uint16_t compute_ip_checksum(const uint8_t* buffer,
                                           size_t length,
                                           uint32_t sum = 0)
{
    // Sum 16-bit words (big-endian processing)
    for (size_t i = 0; i < length - 1; i += 2)
    {
        sum += (buffer[i] << 8) | buffer[i + 1];
    }
    
    // Add leftover byte if length is odd
    if (length & 1)
    {
        sum += (buffer[length - 1] << 8);
    }
    
    // Fold 32-bit sum to 16 bits
    while (sum >> 16)
    {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return static_cast<uint16_t>(~sum);
}

static inline bool build_ipv4_udp_packet(
    uint32_t source_ip,
    uint32_t dest_ip,
    uint16_t source_port,
    uint16_t dest_port,
    const uint8_t* udp_packet_contents,
    uint32_t udp_packet_length,
    uint8_t* output_ip_buffer,
    uint32_t* output_ip_buffer_len)
{
    if (!output_ip_buffer || !output_ip_buffer_len || !udp_packet_contents)
    {
        return false;
    }

    const uint32_t ip_header_len = 20;
    const uint32_t udp_header_len = 8;
    const uint32_t total_len =
        ip_header_len + udp_header_len + udp_packet_length;

    // A UDP packet payload cannot exceed
    // 65535 - 20 (IP) - 8 (UDP) = 65507 bytes
    if (udp_packet_length > 65507)
    {
        return false;
    }

    // Check if provided buffer is large enough
    if (*output_ip_buffer_len < total_len)
    {
        return false;
    }

    *output_ip_buffer_len = total_len;
    std::memset(output_ip_buffer, 0, total_len);

    // ==========================================
    // 1. Build IPv4 Header
    // ==========================================
    output_ip_buffer[0] = 0x45; // Version 4, IHL 5 (5 * 4 = 20 bytes)
    output_ip_buffer[1] = 0x00; // TOS / DSCP
    
    output_ip_buffer[2] = (total_len >> 8) & 0xFF; // Total Length
    output_ip_buffer[3] = total_len & 0xFF;
    
    output_ip_buffer[4] = 0x00; // Identification
    output_ip_buffer[5] = 0x00;
    
    output_ip_buffer[6] = 0x40; // Flags (0x40 = Don't Fragment)
    output_ip_buffer[7] = 0x00; // Fragment Offset
    
    output_ip_buffer[8] = 64;   // TTL
    output_ip_buffer[9] = 17;   // Protocol (UDP = 17)
    
    // Source IP
    output_ip_buffer[12] = (source_ip >> 24) & 0xFF;
    output_ip_buffer[13] = (source_ip >> 16) & 0xFF;
    output_ip_buffer[14] = (source_ip >> 8) & 0xFF;
    output_ip_buffer[15] = source_ip & 0xFF;
    
    // Destination IP
    output_ip_buffer[16] = (dest_ip >> 24) & 0xFF;
    output_ip_buffer[17] = (dest_ip >> 16) & 0xFF;
    output_ip_buffer[18] = (dest_ip >> 8) & 0xFF;
    output_ip_buffer[19] = dest_ip & 0xFF;

    // Calculate and write IP Checksum
    uint16_t ip_checksum = compute_ip_checksum(output_ip_buffer,
                                               ip_header_len);
    output_ip_buffer[10] = (ip_checksum >> 8) & 0xFF;
    output_ip_buffer[11] = ip_checksum & 0xFF;

    // ==========================================
    // 2. Build UDP Header
    // ==========================================
    uint8_t* udp_header = output_ip_buffer + ip_header_len;
    uint32_t udp_total_len = udp_header_len + udp_packet_length;
    
    udp_header[0] = (source_port >> 8) & 0xFF;
    udp_header[1] = source_port & 0xFF;
    
    udp_header[2] = (dest_port >> 8) & 0xFF;
    udp_header[3] = dest_port & 0xFF;
    
    udp_header[4] = (udp_total_len >> 8) & 0xFF;
    udp_header[5] = udp_total_len & 0xFF;
    
    // Copy Payload
    std::memcpy(udp_header + udp_header_len,
                udp_packet_contents, udp_packet_length);

    // ==========================================
    // 3. Calculate UDP Checksum (with pseudo-header)
    // ==========================================
    uint32_t pseudo_sum = 0;
    
    // Add Source IP to pseudo sum
    pseudo_sum += (source_ip >> 16) & 0xFFFF;
    pseudo_sum += source_ip & 0xFFFF;
    
    // Add Destination IP to pseudo sum
    pseudo_sum += (dest_ip >> 16) & 0xFFFF;
    pseudo_sum += dest_ip & 0xFFFF;
    
    // Add Protocol and UDP Length
    pseudo_sum += 17;
    pseudo_sum += udp_total_len;

    // Compute final UDP checksum
    uint16_t udp_checksum = compute_ip_checksum(udp_header,
                                                udp_total_len,
                                                pseudo_sum);
    
    // In UDP, a calculated checksum of 0 is transmitted as all ones (0xFFFF)
    if (udp_checksum == 0x0000)
    {
        udp_checksum = 0xFFFF;
    }
    
    udp_header[6] = (udp_checksum >> 8) & 0xFF;
    udp_header[7] = udp_checksum & 0xFF;

    return true;
}

static inline bool build_ethernet_packet(
    uint8_t   dest_eth[6],
    uint8_t   source_eth[6],
    uint16_t  ether_type,
    const uint8_t * packet_contents,
    uint32_t  packet_length,
    uint8_t*  output_eth_buffer,
    uint32_t* output_eth_buffer_len)
{
    uint32_t  needed = 14 + packet_length;
    if (*output_eth_buffer_len < needed)
        return false;
    *output_eth_buffer_len = needed;
    memcpy(output_eth_buffer, dest_eth, 6);
    output_eth_buffer += 6;
    memcpy(output_eth_buffer, source_eth, 6);
    output_eth_buffer += 6;
    *output_eth_buffer++ = (ether_type >> 8) & 0xFF;
    *output_eth_buffer++ = (ether_type >> 0) & 0xFF;
    memcpy(output_eth_buffer, packet_contents, packet_length);
    return true;
}

