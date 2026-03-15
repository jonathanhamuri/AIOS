#include "net.h"
#include "../terminal/terminal.h"
#include "../mm/heap.h"

// RTL8139 registers
#define RTL_MAC0      0x00
#define RTL_MAR0      0x08
#define RTL_RBSTART   0x30
#define RTL_CMD       0x37
#define RTL_CAPR      0x38
#define RTL_IMR       0x3C
#define RTL_ISR       0x3E
#define RTL_TCR       0x40
#define RTL_RCR       0x44
#define RTL_CONFIG1   0x52
#define RTL_TSAD0     0x20
#define RTL_TSD0      0x10

// TX buffers (4 slots)
#define TX_BUF_SIZE   1536
#define RX_BUF_SIZE   (8192 + 16 + 1500)

static unsigned short nic_port = 0;
static unsigned char  rx_buf[RX_BUF_SIZE];
static unsigned char  tx_buf[4][TX_BUF_SIZE];
static int tx_slot = 0;
static unsigned int rx_pos = 0;

unsigned char mac_addr[ETH_ALEN] = {0};
unsigned char ip_addr[4]  = {10,0,2,15};
unsigned char gateway[4]  = {10,0,2,2};
unsigned char netmask[4]  = {255,255,255,0};

static void outb(unsigned short p, unsigned char v){
    __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));
}
static void outw(unsigned short p, unsigned short v){
    __asm__ volatile("outw %0,%1"::"a"(v),"Nd"(p));
}
static void outl(unsigned short p, unsigned int v){
    __asm__ volatile("outl %0,%1"::"a"(v),"Nd"(p));
}
static unsigned char inb(unsigned short p){
    unsigned char v;
    __asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));
    return v;
}
static unsigned short inw(unsigned short p){
    unsigned short v;
    __asm__ volatile("inw %1,%0":"=a"(v):"Nd"(p));
    return v;
}
static unsigned int inl(unsigned short p){
    unsigned int v;
    __asm__ volatile("inl %1,%0":"=a"(v):"Nd"(p));
    return v;
}

// PCI scan for RTL8139 (vendor=0x10EC, device=0x8139)
static unsigned int pci_read(unsigned char bus, unsigned char dev,
                              unsigned char func, unsigned char reg){
    unsigned int addr = 0x80000000 | ((unsigned int)bus<<16) |
                        ((unsigned int)dev<<11) | ((unsigned int)func<<8) |
                        (reg & 0xFC);
    outl(0xCF8, addr);
    return inl(0xCFC);
}
static void pci_write(unsigned char bus, unsigned char dev,
                      unsigned char func, unsigned char reg, unsigned int val){
    unsigned int addr = 0x80000000 | ((unsigned int)bus<<16) |
                        ((unsigned int)dev<<11) | ((unsigned int)func<<8) |
                        (reg & 0xFC);
    outl(0xCF8, addr);
    outl(0xCFC, val);
}

static unsigned short find_rtl8139(){
    for(int bus=0;bus<256;bus++)
        for(int dev=0;dev<32;dev++){
            unsigned int vid_did = pci_read(bus,dev,0,0);
            if(vid_did == 0xFFFFFFFF) continue;
            if(vid_did == 0x813910EC){
                // Enable bus mastering + IO space
                unsigned int cmd = pci_read(bus,dev,0,4);
                pci_write(bus,dev,0,4,cmd|0x05);
                // Get IO base (BAR0)
                unsigned int bar0 = pci_read(bus,dev,0,0x10);
                return (unsigned short)(bar0 & 0xFFFC);
            }
        }
    return 0;
}

static unsigned short htons(unsigned short v){
    return (v>>8)|(v<<8);
}
static unsigned int htonl(unsigned int v){
    return ((v&0xFF)<<24)|((v&0xFF00)<<8)|((v>>8)&0xFF00)|((v>>24)&0xFF);
}

int net_init(){
    nic_port = find_rtl8139();
    if(!nic_port){
        terminal_print_color("Network          : No NIC found\n",
            MAKE_COLOR(COLOR_BRED,COLOR_BLACK));
        return -1;
    }

    // Power on
    outb(nic_port + RTL_CONFIG1, 0x00);
    // Software reset
    outb(nic_port + RTL_CMD, 0x10);
    int timeout = 1000;
    while((inb(nic_port + RTL_CMD) & 0x10) && timeout--);

    // Read MAC
    for(int i=0;i<6;i++)
        mac_addr[i] = inb(nic_port + RTL_MAC0 + i);

    // Set RX buffer
    outl(nic_port + RTL_RBSTART, (unsigned int)rx_buf);

    // Set IMR/ISR
    outw(nic_port + RTL_IMR, 0x0005); // ROK + TOK
    outw(nic_port + RTL_ISR, 0x0000);

    // RCR: accept all, no wrap
    outl(nic_port + RTL_RCR, 0x0000000F | (1<<7));

    // TCR
    outl(nic_port + RTL_TCR, 0x03000700);

    // Enable RX+TX
    outb(nic_port + RTL_CMD, 0x0C);

    rx_pos = 0;

    terminal_print_color("Network          : OK (RTL8139)\n",
        MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));

    // Print MAC
    terminal_print_color("MAC: ",MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    for(int i=0;i<6;i++){
        // print hex byte
        unsigned char b = mac_addr[i];
        char h[3];
        h[0]="0123456789ABCDEF"[b>>4];
        h[1]="0123456789ABCDEF"[b&0xF];
        h[2]=0;
        terminal_print(h);
        if(i<5) terminal_print(":");
    }
    terminal_print_color("\nIP:  10.0.2.15\n",MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));

    return 0;
}

void net_send(unsigned char* dst_mac, unsigned short type,
              unsigned char* data, unsigned short len){
    if(!nic_port) return;

    // Build ethernet frame in tx_buf
    unsigned char* buf = tx_buf[tx_slot];
    // dst mac
    for(int i=0;i<6;i++) buf[i]=dst_mac[i];
    // src mac
    for(int i=0;i<6;i++) buf[6+i]=mac_addr[i];
    // ethertype
    buf[12]=(type>>8)&0xFF;
    buf[13]=type&0xFF;
    // payload
    if(len>ETH_MTU) len=ETH_MTU;
    for(int i=0;i<len;i++) buf[14+i]=data[i];

    unsigned short total = 14 + len;
    if(total < 60) total = 60;

    // Send via RTL8139
    outl(nic_port + RTL_TSAD0 + tx_slot*4, (unsigned int)buf);
    outl(nic_port + RTL_TSD0  + tx_slot*4, total);

    tx_slot = (tx_slot+1)%4;
}

void net_poll(){
    if(!nic_port) return;
    unsigned short isr = inw(nic_port + RTL_ISR);
    if(!(isr & 0x01)) return; // no RX

    outw(nic_port + RTL_ISR, 0x01); // clear ROK

    while((inb(nic_port + RTL_CMD) & 0x01) == 0){
        unsigned char* pkt = rx_buf + rx_pos;
        unsigned short status = *(unsigned short*)pkt;
        unsigned short pkt_len = *(unsigned short*)(pkt+2);

        if(pkt_len > 1500 || pkt_len < 14){ rx_pos=0; break; }

        unsigned char* frame = pkt + 4;
        unsigned short etype = (frame[12]<<8)|frame[13];

        if(etype == 0x0806){
            arp_handle((arp_pkt_t*)(frame+14));
        } else if(etype == 0x0800){
            ip_hdr_t* ip = (ip_hdr_t*)(frame+14);
            if(ip->protocol == 17){
                udp_hdr_t* udp = (udp_hdr_t*)((unsigned char*)ip + ((ip->version_ihl&0xF)*4));
                udp_handle(ip, udp);
            }
        }

        rx_pos = (rx_pos + pkt_len + 4 + 3) & ~3;
        if(rx_pos > RX_BUF_SIZE) rx_pos=0;
        outw(nic_port + RTL_CAPR, rx_pos - 16);
    }
}

// ARP
static unsigned char arp_table_ip[8][4];
static unsigned char arp_table_mac[8][ETH_ALEN];
static int arp_count = 0;

void arp_send_request(unsigned char* target_ip){
    unsigned char bcast[6]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    arp_pkt_t arp;
    arp.htype=htons(1);
    arp.ptype=htons(0x0800);
    arp.hlen=6; arp.plen=4;
    arp.oper=htons(1);
    for(int i=0;i<6;i++) arp.sha[i]=mac_addr[i];
    for(int i=0;i<4;i++) arp.spa[i]=ip_addr[i];
    for(int i=0;i<6;i++) arp.tha[i]=0;
    for(int i=0;i<4;i++) arp.tpa[i]=target_ip[i];
    net_send(bcast,0x0806,(unsigned char*)&arp,sizeof(arp_pkt_t));
}

void arp_handle(arp_pkt_t* arp){
    // Check if it's a reply for us
    if(htons(arp->oper)==2){
        if(arp_count<8){
            for(int i=0;i<4;i++) arp_table_ip[arp_count][i]=arp->spa[i];
            for(int i=0;i<6;i++) arp_table_mac[arp_count][i]=arp->sha[i];
            arp_count++;
        }
    }
    // Reply to ARP requests for our IP
    if(htons(arp->oper)==1){
        int match=1;
        for(int i=0;i<4;i++) if(arp->tpa[i]!=ip_addr[i]){match=0;break;}
        if(match){
            arp_pkt_t rep;
            rep.htype=htons(1); rep.ptype=htons(0x0800);
            rep.hlen=6; rep.plen=4; rep.oper=htons(2);
            for(int i=0;i<6;i++) rep.sha[i]=mac_addr[i];
            for(int i=0;i<4;i++) rep.spa[i]=ip_addr[i];
            for(int i=0;i<6;i++) rep.tha[i]=arp->sha[i];
            for(int i=0;i<4;i++) rep.tpa[i]=arp->spa[i];
            net_send(arp->sha,0x0806,(unsigned char*)&rep,sizeof(arp_pkt_t));
        }
    }
}

static unsigned short ip_checksum(unsigned short* data, int len){
    unsigned int sum=0;
    while(len>1){sum+=*data++;len-=2;}
    if(len) sum+=*(unsigned char*)data;
    while(sum>>16) sum=(sum&0xFFFF)+(sum>>16);
    return ~sum;
}

void ip_send_udp(unsigned char* dst_ip, unsigned short src_port,
                 unsigned short dst_port, unsigned char* data, unsigned short len){
    static unsigned char pkt[1500];
    ip_hdr_t* ip = (ip_hdr_t*)pkt;
    udp_hdr_t* udp = (udp_hdr_t*)(pkt+20);
    unsigned char* payload = pkt+28;

    for(int i=0;i<len;i++) payload[i]=data[i];

    udp->src_port=htons(src_port);
    udp->dst_port=htons(dst_port);
    udp->length=htons(8+len);
    udp->checksum=0;

    ip->version_ihl=0x45;
    ip->tos=0;
    ip->total_len=htons(20+8+len);
    ip->id=htons(1);
    ip->flags_frag=0;
    ip->ttl=64;
    ip->protocol=17;
    ip->checksum=0;
    for(int i=0;i<4;i++) ip->src[i]=ip_addr[i];
    for(int i=0;i<4;i++) ip->dst[i]=dst_ip[i];
    ip->checksum=ip_checksum((unsigned short*)ip,20);

    // Find MAC for dst (use gateway if not local)
    unsigned char dst_mac[6]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    for(int i=0;i<arp_count;i++){
        int match=1;
        for(int j=0;j<4;j++) if(arp_table_ip[i][j]!=dst_ip[j]){match=0;break;}
        if(match){for(int j=0;j<6;j++) dst_mac[j]=arp_table_mac[i][j];break;}
    }
    net_send(dst_mac,0x0800,pkt,20+8+len);
}

void udp_handle(ip_hdr_t* ip, udp_hdr_t* udp){
    unsigned char* payload = (unsigned char*)udp + 8;
    unsigned short port = htons(udp->dst_port);
    terminal_print_color("[NET] UDP packet on port ",MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
    terminal_print_int(port);
    terminal_print_color(": ",MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
    // Print payload as string
    unsigned short plen = htons(udp->length)-8;
    if(plen>64) plen=64;
    char buf[65];
    for(int i=0;i<plen;i++) buf[i]=payload[i];
    buf[plen]=0;
    terminal_print(buf);
    terminal_newline();
}

void net_send_message(const char* msg, unsigned char* dst_ip, unsigned short port){
    unsigned short len=0;
    while(msg[len]) len++;
    ip_send_udp(dst_ip,(unsigned short)7777,port,(unsigned char*)msg,len);
    terminal_print_color("[NET] Message sent\n",MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
}
