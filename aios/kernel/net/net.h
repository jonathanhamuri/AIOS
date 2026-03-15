#ifndef NET_H
#define NET_H

// Ethernet frame
#define ETH_ALEN 6
#define ETH_MTU  1500

typedef struct {
    unsigned char  dst[ETH_ALEN];
    unsigned char  src[ETH_ALEN];
    unsigned short type;
    unsigned char  data[ETH_MTU];
} __attribute__((packed)) eth_frame_t;

// IP packet
typedef struct {
    unsigned char  version_ihl;
    unsigned char  tos;
    unsigned short total_len;
    unsigned short id;
    unsigned short flags_frag;
    unsigned char  ttl;
    unsigned char  protocol;
    unsigned short checksum;
    unsigned char  src[4];
    unsigned char  dst[4];
} __attribute__((packed)) ip_hdr_t;

// ARP packet
typedef struct {
    unsigned short htype;
    unsigned short ptype;
    unsigned char  hlen;
    unsigned char  plen;
    unsigned short oper;
    unsigned char  sha[ETH_ALEN];
    unsigned char  spa[4];
    unsigned char  tha[ETH_ALEN];
    unsigned char  tpa[4];
} __attribute__((packed)) arp_pkt_t;

// UDP header
typedef struct {
    unsigned short src_port;
    unsigned short dst_port;
    unsigned short length;
    unsigned short checksum;
} __attribute__((packed)) udp_hdr_t;

// Network config
extern unsigned char mac_addr[ETH_ALEN];
extern unsigned char ip_addr[4];
extern unsigned char gateway[4];
extern unsigned char netmask[4];

// Driver functions
int  net_init();
void net_send(unsigned char* dst_mac, unsigned short type, 
              unsigned char* data, unsigned short len);
void net_poll();

// Protocol functions  
void arp_send_request(unsigned char* target_ip);
void arp_handle(arp_pkt_t* arp);
void ip_send_udp(unsigned char* dst_ip, unsigned short src_port,
                 unsigned short dst_port, unsigned char* data, unsigned short len);
void udp_handle(ip_hdr_t* ip, udp_hdr_t* udp);

// High level
void net_send_message(const char* msg, unsigned char* dst_ip, unsigned short port);

#endif
