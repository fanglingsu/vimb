#ifndef _URL_HISTORY_H
#define _URL_HISTORY_H

typedef struct {
    char* uri;
    char* title;
} UrlHist;

void url_history_init(void);
void url_history_cleanup(void);
void url_history_add(const char* url, const char* title);
GList* url_history_get_all(void);

#endif /* end of include guard: _URL_HISTORY_H */
