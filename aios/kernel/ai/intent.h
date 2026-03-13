#ifndef INTENT_H
#define INTENT_H

// Intent types — what the user wants to DO
typedef enum {
    INTENT_UNKNOWN    = 0,
    INTENT_PRINT      = 1,   // show/print/display/affiche/montre
    INTENT_CLEAR      = 2,   // clear/efface/nettoie
    INTENT_MEMORY     = 3,   // memory/memoire/ram
    INTENT_HELP       = 4,   // help/aide/comment
    INTENT_ABOUT      = 5,   // about/apropos/what is aios
    INTENT_COMPILE    = 6,   // compile/run/execute
    INTENT_CALC       = 7,   // calculate/calcule/compute
    INTENT_GREET      = 8,   // hello/bonjour/salut/hi
    INTENT_LIST_PROC  = 9,   // processes/processus/ps
    INTENT_AI_QUERY   = 10,  // what/qui/pourquoi/why/how/comment
} intent_type_t;

// Extracted intent with arguments
typedef struct {
    intent_type_t type;
    char          arg1[128];   // primary argument (e.g. string to print)
    char          arg2[64];    // secondary argument
    int           num1;        // numeric argument
    float         num2;
    char          lang[4];     // "en" or "fr"
    int           confidence;  // 0-100
} intent_t;

void intent_parse(const char* input, intent_t* out);
const char* intent_name(intent_type_t t);

#endif
