#include "discovery.h"
#include "../net.h"
#include "../../terminal/terminal.h"
#include "../../ai/knowledge/kb.h"
#include "../../ai/learning/learning.h"

peer_t peers[MAX_PEERS];
int peer_count = 0;

static int sstart(const char*s,const char*p){while(*p){if(*s!=*p)return 0;s++;p++;}return 1;}
static int seq(const char*a,const char*b){while(*a&&*b){if(*a!=*b)return 0;a++;b++;}return *a==*b;}
static int slen(const char*s){int n=0;while(*s++)n++;return n;}
static void scopy(char*d,const char*s,int m){int i=0;while(s[i]&&i<m-1){d[i]=s[i];i++;}d[i]=0;}
static void sappend(char*d,const char*s){int i=slen(d),j=0;while(s[j]){d[i++]=s[j++];}d[i]=0;}

static unsigned char bcast[4]={10,0,2,255};

static void build_packet(char*buf,const char*cmd,const char*data){
    buf[0]=0;sappend(buf,"AIOS|");sappend(buf,cmd);
    sappend(buf,"|AIOS-NODE|");sappend(buf,data);
}

static peer_t* find_or_add_peer(unsigned char*ip){
    for(int i=0;i<peer_count;i++)
        if(peers[i].ip[0]==ip[0]&&peers[i].ip[1]==ip[1]&&
           peers[i].ip[2]==ip[2]&&peers[i].ip[3]==ip[3])
            return &peers[i];
    if(peer_count>=MAX_PEERS) return 0;
    peer_t*p=&peers[peer_count++];
    p->ip[0]=ip[0];p->ip[1]=ip[1];p->ip[2]=ip[2];p->ip[3]=ip[3];
    p->active=1;p->last_seen=0;
    scopy(p->name,"AIOS-NODE",PEER_NAME_LEN);
    return p;
}

void discovery_init(){
    peer_count=0;
    for(int i=0;i<MAX_PEERS;i++){peers[i].active=0;peers[i].name[0]=0;}
    terminal_print_color("Discovery        : OK (UDP:7770)\n",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
}

void discovery_broadcast(){
    char pkt[128]={0};
    build_packet(pkt,"HELLO","ready");
    net_send_message(pkt,bcast,DISCOVERY_PORT);
    terminal_print_color("[DISC] Broadcast sent\n",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
}

void discovery_poll(){ net_poll(); }

void discovery_list_peers(){
    terminal_print_color("[DISC] Peers:\n",MAKE_COLOR(COLOR_BYELLOW,COLOR_BLACK));
    if(peer_count==0){
        terminal_print("  No peers. Type 'discover' to scan.\n");
        return;
    }
    for(int i=0;i<peer_count;i++){
        terminal_print_color("  [",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
        terminal_print_int(i);
        terminal_print_color("] ",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
        terminal_print(peers[i].name);
        terminal_print_color(" @ ",MAKE_COLOR(COLOR_GRAY,COLOR_BLACK));
        terminal_print_int(peers[i].ip[0]);terminal_print(".");
        terminal_print_int(peers[i].ip[1]);terminal_print(".");
        terminal_print_int(peers[i].ip[2]);terminal_print(".");
        terminal_print_int(peers[i].ip[3]);terminal_newline();
    }
}

void discovery_send_msg(const char*msg,int idx){
    if(idx<0||idx>=peer_count) return;
    char pkt[256]={0};
    build_packet(pkt,"MSG",msg);
    net_send_message(pkt,peers[idx].ip,AIOS_MSG_PORT);
    terminal_print_color("[MSG] Sent\n",MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
}

void discovery_share_skill(const char*skill_name,int idx){
    if(idx<0||idx>=peer_count) return;
    extern skill_t skill_table[];
    extern int learning_count;
    for(int i=0;i<learning_count;i++){
        if(seq(skill_table[i].name,skill_name)){
            char pkt[512]={0},data[300]={0};
            scopy(data,skill_name,64);sappend(data,"|");
            sappend(data,skill_table[i].code);
            build_packet(pkt,"SKILL",data);
            net_send_message(pkt,peers[idx].ip,DISCOVERY_PORT);
            terminal_print_color("[DISC] Skill shared\n",MAKE_COLOR(COLOR_BGREEN,COLOR_BLACK));
            return;
        }
    }
    terminal_print_color("[DISC] Skill not found\n",MAKE_COLOR(COLOR_BRED,COLOR_BLACK));
}

int discovery_handle(const char*input){
    if(seq(input,"discover")||seq(input,"find peers")||
       seq(input,"scan network")){
        discovery_broadcast();return 1;
    }
    if(seq(input,"peers")||seq(input,"list peers")||
       seq(input,"show peers")){
        discovery_list_peers();return 1;
    }
    if(sstart(input,"msg peer ")){
        int idx=input[9]-'0';
        discovery_send_msg(input+11,idx);return 1;
    }
    if(sstart(input,"share skill ")){
        const char*p=input+12;
        char skill[64]={0};int i=0;
        while(p[i]&&!sstart(p+i," with peer")&&i<63){skill[i]=p[i];i++;}
        int pidx=sstart(p+i," with peer ")?p[i+11]-'0':0;
        discovery_share_skill(skill,pidx);return 1;
    }
    if(sstart(input,"ping peer ")){
        int idx=input[10]-'0';
        if(idx>=0&&idx<peer_count){
            char pkt[64]={0};
            build_packet(pkt,"PING","");
            net_send_message(pkt,peers[idx].ip,DISCOVERY_PORT);
            terminal_print_color("[DISC] Ping sent\n",MAKE_COLOR(COLOR_BCYAN,COLOR_BLACK));
        }
        return 1;
    }
    return 0;
}
