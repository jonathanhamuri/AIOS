#ifndef DOC_PAGE_H
#define DOC_PAGE_H

#define DOC_STORE_MAX   32
#define DOC_TITLE_LEN   64
#define DOC_BODY_LEN    32768

typedef struct {
    char title[DOC_TITLE_LEN];
    char type[16];
    char body[DOC_BODY_LEN];
    int  used;
} doc_entry_t;

/* Page IDs */
#define PAGE_MAIN       0
#define PAGE_DOC_BROWSER 1
#define PAGE_DOC_EDITOR  2
#define PAGE_DOC_VIEWER  3

void doc_page_init(void);
void doc_page_open_browser(void);
void doc_page_open_editor(const char* title, const char* type);
void doc_page_open_viewer(int index);
void doc_page_close(void);
void doc_page_write(const char* text);
void doc_page_save(void);
void doc_page_tick(void);
int  doc_page_active(void);
int  doc_page_handle(const char* input);
extern int current_page;

void doc_page_write_long(const char* title, const char* type,
                            const char* topic, int pages);
void doc_page_export_text(int index, char* out, int maxlen);
extern doc_entry_t doc_store[DOC_STORE_MAX];
extern int active_doc;
extern int doc_count;
extern int current_page;
void render_doc(void);
void doc_page_save(void);
#endif
