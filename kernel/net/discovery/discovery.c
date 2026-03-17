#include "discovery.h"
#include "../net.h"
#include "../../terminal/terminal.h"
#include "../../ai/knowledge/kb.h"
#include "../../ai/learning/learning.h"

peer_t peers[MAX_PEERS];
int peer_count = 0;

static int sstart(const char* s,const char* p){
    while(*p){if(*s!=*p)return 0;s++;p++;}return 1;
}
static int seq(const char* a,const char* b){
    while(*a&&*b){if(*a!=*b)return 0;a++;b++;}return *a==*b;
}
static int slen(const char* s){int n=0;while(*s++)n++;return n;}
static void scopy(char* d,const char* s,int m){
    int i=0;while(s[i]&&i<m-1){d[i]=s[i];i++;}d[i]=0;
}
static void sappend(char* d,const char* s){
    int i=slen(d),j=0;
    while(s[j]){d[i++]=s[j++];}d[i]=0;
}

// Broadcast address
static unsigned char bcast[4] = {10,0,2,255};

// AIOS discovery packet format:
// "AIOS|CMD|NAME|DATA"
// CMD: HELLO, BYE, MSG, SKILL, PING

static void build_packet(char* buf, const char* cmd,
                          const char* data){
    buf[0]=0;
    sappend(buf,"AIOS|");
    sappend(buf,cmd);
    sappend(buf,"|AIOS-NODE|");
    sappend(buf,data);
}

static peer_t* find_or_add_peer(unsigned char* ip){
    // Check existing
    for(int i=0;i<peer_count;i++){
        if(peers[i].ip[0]==ip[0] && peers[i].ip[1]==ip[1] &&
           peers[i].ip[2]==ip[2] && peers[i].ip[3]==ip[3]){
            return &peers[i];
        }
    }
    // Add new
    if(peer_count >= MAX_PEERS) return 0;
    peer_t* p = &peers[peer_count++];
    p->ip[0]=ip[0]; p->ip[1]=ip[1];
    p->ip[2]=ip[2]; p->ip[3]=ip[3];
    p->active=1;
    p->last_seen=0;
    scopy(p->name,"AIOS-NODE",PEER_NAME_LEN);
    return p;
}

void discovery_init(){
    peer_count=0;
    for(int i=0;i<MAX_PEERS;i++){
        peers[i].active=0;
        peers[i].name[0]=0;
    }
    terminal_print_color("Discovery        : OK (UDP:7770)\n",
        MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
}

void discovery_broadcast(){
    char pkt[128]={0};
    build_packet(pkt,"HELLO","ready");
    net_send_message(pkt, bcast, DISCOVERY_PORT);
}

void discovery_poll(){
    // Poll network for incoming packets
    net_poll();
}

// Parse incoming AIOS packet
void discovery_receive(unsigned char* src_ip,
                        const char* data, int len){
    if(len < 5) return;
    if(!sstart(data,"AIOS|")) return;

    // Parse: AIOS|CMD|NAME|DATA
    const char* p = data+5;
    char cmd[16]={0};
    int i=0;
    while(p[i]&&p[i]!='|'&&i<15){cmd[i]=p[i];i++;}
    cmd[i]=0;
    p+=i+1;

    char name[PEER_NAME_LEN]={0};
    i=0;
    while(p[i]&&p[i]!='|'&&i<15){name[i]=p[i];i++;}
    name[i]=0;
    p+=i+1;

    // Find/add peer
    peer_t* peer = find_or_add_peer(src_ip);
    if(peer && name[0]) scopy(peer->name,name,PEER_NAME_LEN);

    if(seq(cmd,"HELLO")){
        terminal_print_color("[DISC] New peer: ",
            MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
        terminal_print(name);
        terminal_print_color(" @ ",
            MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
        // Print IP
        char ipbuf[20]={0};
        ipbuf[0]='0'+src_ip[0]/100;
        ipbuf[1]='0'+(src_ip[0]/10)%10;
        ipbuf[2]='0'+src_ip[0]%10;
        ipbuf[3]='.';
        // simplified
        terminal_print(ipbuf);
        terminal_newline();

        // Reply with our hello
        char reply[128]={0};
        build_packet(reply,"HELLO","reply");
        net_send_message(reply,src_ip,DISCOVERY_PORT);
    }
    else if(seq(cmd,"MSG")){
        terminal_print_color("[MSG from ",
            MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
        terminal_print(name);
        terminal_print_color("] ",
            MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
        terminal_print(p);
        terminal_newline();
    }
    else if(seq(cmd,"SKILL")){
        // Receive a shared skill: name|code
        char skill_name[64]={0};
        char skill_code[256]={0};
        int j=0;
        while(p[j]&&p[j]!='|'&&j<63){skill_name[j]=p[j];j++;}
        skill_name[j]=0;
        scopy(skill_code,p+j+1,256);

        extern int learning_add(const char* name, const char* code);
        learning_add(skill_name,skill_code);

        terminal_print_color("[DISC] Received skill: ",
            MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
        terminal_print(skill_name);
        terminal_print_color(" from ",
            MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
        terminal_print(name);
        terminal_newline();
    }
    else if(seq(cmd,"PING")){
        char pong[64]={0};
        build_packet(pong,"PONG","alive");
        net_send_message(pong,src_ip,DISCOVERY_PORT);
    }
}

void discovery_list_peers(){
    terminal_print_color("[DISC] Connected peers:\n",
        MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    if(peer_count==0){
        terminal_print("  No peers found yet.\n");
        terminal_print("  Type 'discover' to broadcast.\n");
        return;
    }
    for(int i=0;i<peer_count;i++){
        if(!peers[i].active) continue;
        terminal_print_color("  [",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
        terminal_print_int(i);
        terminal_print_color("] ",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
        terminal_print_color(peers[i].name,
            MAKE_COLOR(COLOR_BWHITE,COLOR_BLACK));
        terminal_print_color(" @ ",
            MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
        terminal_print_int(peers[i].ip[0]);
        terminal_print_color(".",MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
        terminal_print_int(peers[i].ip[1]);
        terminal_print_color(".",MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
        terminal_print_int(peers[i].ip[2]);
        terminal_print_color(".",MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
        terminal_print_int(peers[i].ip[3]);
        terminal_newline();
    }
}

void discovery_send_msg(const char* msg, int peer_idx){
    if(peer_idx<0||peer_idx>=peer_count) return;
    char pkt[256]={0};
    build_packet(pkt,"MSG",msg);
    net_send_message(pkt,peers[peer_idx].ip,AIOS_MSG_PORT);
    terminal_print_color("[MSG] Sent to ",
        MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
    terminal_print(peers[peer_idx].name);
    terminal_newline();
}

void discovery_share_skill(const char* skill_name, int peer_idx){
    if(peer_idx<0||peer_idx>=peer_count) return;

    // Find skill code
    extern skill_t skill_table[];
    extern int learning_count;
    char code[256]={0};
    int found=0;
    for(int i=0;i<learning_count;i++){
        extern int seq(const char*,const char*);
        if(seq(skill_table[i].name,skill_name)){
            scopy(code,skill_table[i].code,256);
            found=1;
            break;
        }
    }

    if(!found){
        terminal_print_color("[DISC] Skill not found\n",
            MAKE_COLOR(COLOR_BRED,COLOR_BLACK));
        return;
    }

    char pkt[512]={0};
    char data[300]={0};
    scopy(data,skill_name,64);
    sappend(data,"|");
    sappend(data,code);
    build_packet(pkt,"SKILL",data);
    net_send_message(pkt,peers[peer_idx].ip,DISCOVERY_PORT);

    terminal_print_color("[DISC] Shared skill: ",
        MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
    terminal_print(skill_name);
    terminal_newline();
}

int discovery_handle(const char* input){
    // "discover" / "find peers" / "scan network"
    if(seq(input,"discover")||
       seq(input,"find peers")||
       seq(input,"scan network")||
       sstart(input,"broadcast")){
        terminal_print_color("[DISC] Broadcasting presence...\n",
            MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
        discovery_broadcast();
        return 1;
    }

    // "peers" / "list peers" / "connected devices"
    if(seq(input,"peers")||
       seq(input,"list peers")||
       seq(input,"connected devices")||
       seq(input,"show peers")){
        discovery_list_peers();
        return 1;
    }

    // "msg peer 0 hello there"
    if(sstart(input,"msg peer ")){
        int idx = input[9]-'0';
        const char* msg = input+11;
        discovery_send_msg(msg,idx);
        return 1;
    }

    // "share skill X with peer 0"
    if(sstart(input,"share skill ")){
        const char* p = input+12;
        char skill[64]={0};
        int i=0;
        while(p[i]&&!sstart(p+i," with peer")&&i<63){
            skill[i]=p[i];i++;
        }
        skill[i]=0;
        int peer_idx=0;
        if(sstart(p+i," with peer ")){
            peer_idx=p[i+11]-'0';
        }
        discovery_share_skill(skill,peer_idx);
        return 1;
    }

    // "ping peer 0"
    if(sstart(input,"ping peer ")){
        int idx=input[10]-'0';
        if(idx>=0&&idx<peer_count){
            char pkt[64]={0};
            build_packet(pkt,"PING","");
            net_send_message(pkt,peers[idx].ip,DISCOVERY_PORT);
            terminal_print_color("[DISC] Ping sent\n",
                MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
        }
        return 1;
    }

    return 0;
}
