#include "main.h"
#include "url_history.h"

static void url_history_free(UrlHist* item);


void url_history_init(void)
{
    /* read the history items from file */
    char buf[512] = {0};
    guint max = core.config.url_history_max;
    FILE* file = fopen(core.files[FILES_HISTORY], "r");
    if (!file) {
        return;
    }

    file_lock_set(fileno(file), F_WRLCK);

    for (guint i = 0; i < max && fgets(buf, sizeof(buf), file); i++) {
        char** argv = NULL;
        gint   argc = 0;
        if (g_shell_parse_argv(buf, &argc, &argv, NULL)) {
            url_history_add(argv[0], argc > 1 ? argv[1] : NULL);
        }
        g_strfreev(argv);
    }

    file_lock_set(fileno(file), F_UNLCK);
    fclose(file);
}

void url_history_cleanup(void)
{
    FILE* file = fopen(core.files[FILES_HISTORY], "w");
    if (file) {
        file_lock_set(fileno(file), F_WRLCK);

        /* write the history to the file */
        GList* link;
        for (link = core.behave.url_history; link != NULL; link = link->next) {
            UrlHist* item = (UrlHist*)link->data;
            char* title = g_shell_quote(item->title ? item->title : "");
            char* new   = g_strdup_printf("%s %s\n", item->uri, title);

            fwrite(new, strlen(new), 1, file);

            g_free(title);
            g_free(new);
        }
        file_lock_set(fileno(file), F_UNLCK);
        fclose(file);
    }

    if (core.behave.url_history) {
        g_list_free_full(core.behave.url_history, (GDestroyNotify)url_history_free);
    }
}

void url_history_add(const char* url, const char* title)
{
    /* uf the url is already in history, remove this entry */
    for (GList* link = core.behave.url_history; link; link = link->next) {
        UrlHist* hi = (UrlHist*)link->data;
        if (!g_strcmp0(url, hi->uri)) {
            url_history_free(hi);
            core.behave.url_history = g_list_delete_link(core.behave.url_history, link);
            break;
        }
    }

    while (core.config.url_history_max < g_list_length(core.behave.url_history)) {
        /* if list is too long - remove items from end */
        core.behave.url_history = g_list_delete_link(
            core.behave.url_history,
            g_list_last(core.behave.url_history)
        );
    }

    UrlHist* item = g_new0(UrlHist, 1);
    item->uri   = g_strdup(url);
    item->title = title ? g_strdup(title) : NULL;

    core.behave.url_history = g_list_prepend(core.behave.url_history, item);
}

static void url_history_free(UrlHist* item)
{
    g_free(item->uri);
    if (item->title) {
        g_free(item->title);
    }
}
