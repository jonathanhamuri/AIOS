#ifndef DISCOVERY_H
#define DISCOVERY_H

#define DISCOVERY_PORT  7770
#define AIOS_MSG_PORT   7771
#define MAX_PEERS       8
#define PEER_NAME_LEN   16

typedef struct {
    unsigned char ip[4];
    char name[PEER_NAME_LEN];
    int active;
    int last_seen;
} peer_t;

void discovery_init();
void discovery_broadcast();
void discovery_poll();
void discovery_list_peers();
void discovery_send_msg(const char* msg, int peer_idx);
void discovery_share_skill(const char* skill_name, int peer_idx);
int  discovery_handle(const char* input);

extern peer_t peers[MAX_PEERS];
extern int peer_count;

#endif
