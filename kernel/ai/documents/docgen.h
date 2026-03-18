#ifndef DOCGEN_H
#define DOCGEN_H

#define DOC_MAX_LINES  64
#define DOC_LINE_LEN   60

typedef struct {
    char lines[DOC_MAX_LINES][DOC_LINE_LEN];
    int  line_count;
    char title[64];
    char type[32];
} document_t;

int  docgen_handle(const char* input);
void docgen_write_essay(const char* topic, const char* style);
void docgen_write_report(const char* topic);
void docgen_write_letter(const char* to, const char* subject);
void docgen_write_plan(const char* project);
void docgen_print_doc(document_t* doc);

#endif
