/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2016 Daniel Carl
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 */

#include "config.h"
#include "main.h"
#include "bookmark.h"
#include "util.h"
#include "completion.h"

extern VbCore vb;

typedef struct {
    char *uri;
    char *title;
    char *tags;
} Bookmark;

static GList *load(const char *file);
static gboolean bookmark_contains_all_tags(Bookmark *bm, char **query,
    unsigned int qlen);
static Bookmark *line_to_bookmark(const char *uri, const char *data);
static void free_bookmark(Bookmark *bm);

/**
 * Write a new bookmark entry to the end of bookmark file.
 */
gboolean bookmark_add(const char *uri, const char *title, const char *tags)
{
    const char *file = vb.files[FILES_BOOKMARK];
    gboolean ret;
    if (tags) {
        ret = util_file_append(file, "%s\t%s\t%s\n", uri, title ? title : "", tags);
    }
    else if (title) {
        ret = util_file_append(file, "%s\t%s\n", uri, title);
    }
    else
    {
        ret = util_file_append(file, "%s\n", uri);
    }
    bookmark_build_html_file();
    return ret;
}

gboolean bookmark_remove(const char *uri)
{
    char **lines, *line, *p;
    int len, i;
    GString *new;
    gboolean removed = false;

    if (!uri) {
        return false;
    }

    lines = util_get_lines(vb.files[FILES_BOOKMARK]);
    if (lines) {
        new = g_string_new(NULL);
        len = g_strv_length(lines) - 1;
        for (i = 0; i < len; i++) {
            line = lines[i];
            g_strstrip(line);
            /* ignore the title or bookmark tags and test only the uri */
            if ((p = strchr(line, '\t'))) {
                *p = '\0';
                if (!strcmp(uri, line)) {
                    removed = true;
                    continue;
                } else {
                    /* reappend the tags */
                    *p = '\t';
                }
            }
            if (!strcmp(uri, line)) {
                removed = true;
                continue;
            }
            g_string_append_printf(new, "%s\n", line);
        }
        g_strfreev(lines);
        g_file_set_contents(vb.files[FILES_BOOKMARK], new->str, -1, NULL);
        g_string_free(new, true);
    }

    bookmark_build_html_file();
    return removed;
}

gboolean bookmark_fill_completion(GtkListStore *store, const char *input)
{
    gboolean found = false;
    char **parts;
    unsigned int len;
    GtkTreeIter iter;
    GList *src = NULL;
    Bookmark *bm;

    src = load(vb.files[FILES_BOOKMARK]);
    src = g_list_reverse(src);
    if (!input || !*input) {
        /* without any tags return all bookmarked items */
        for (GList *l = src; l; l = l->next) {
            bm = (Bookmark*)l->data;
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(
                store, &iter,
                COMPLETION_STORE_FIRST, bm->uri,
#ifdef FEATURE_TITLE_IN_COMPLETION
                COMPLETION_STORE_SECOND, bm->title,
#endif
                -1
            );
            found = true;
        }
    } else {
        parts = g_strsplit(input, " ", 0);
        len   = g_strv_length(parts);

        for (GList *l = src; l; l = l->next) {
            bm = (Bookmark*)l->data;
            if (bookmark_contains_all_tags(bm, parts, len)) {
                gtk_list_store_append(store, &iter);
                gtk_list_store_set(
                    store, &iter,
                    COMPLETION_STORE_FIRST, bm->uri,
#ifdef FEATURE_TITLE_IN_COMPLETION
                    COMPLETION_STORE_SECOND, bm->title,
#endif
                    -1
                );
                found = true;
            }
        }
        g_strfreev(parts);
    }

    g_list_free_full(src, (GDestroyNotify)free_bookmark);

    return found;
}

gboolean bookmark_fill_tag_completion(GtkListStore *store, const char *input)
{
    gboolean found;
    unsigned int len, i;
    char **tags, *tag;
    GList *src = NULL, *taglist = NULL;
    Bookmark *bm;

    /* get all distinct tags from bookmark file */
    src = load(vb.files[FILES_BOOKMARK]);
    for (GList *l = src; l; l = l->next) {
        bm = (Bookmark*)l->data;
        /* if bookmark contains no tags we can go to the next bookmark */
        if (!bm->tags) {
            continue;
        }

        tags = g_strsplit(bm->tags, " ", -1);
        len  = g_strv_length(tags);
        for (i = 0; i < len; i++) {
            tag = tags[i];
            /* add tag only if it isn't already in the list */
            if (!g_list_find_custom(taglist, tag, (GCompareFunc)strcmp)) {
                taglist = g_list_prepend(taglist, g_strdup(tag));
            }
        }
        g_strfreev(tags);
    }

    /* generate the completion with the found tags */
    found = util_fill_completion(store, input, taglist);

    g_list_free_full(src, (GDestroyNotify)free_bookmark);
    g_list_free_full(taglist, (GDestroyNotify)g_free);

    return found;
}

/**
  * Converts bookmark file to a simple HTML5 page
  *
  * @input_bookmark_path: the path to the input bookmark file
  * @output_html_path: the path to the output html file
  *
  * Returns: A boolean to say weather there was an error or not
  */
gboolean bookmark_to_html(char* input_bookmark_path, char* output_html_path)
{
  FILE* input = fopen(input_bookmark_path, "r");
  FILE* output = NULL;

  char line[512];

  if(input == NULL)
  {
    return FALSE;
  }

  output = fopen(output_html_path, "w");
  if (output == NULL)
  {
    fclose(input);
    return FALSE;
  }

  //If the bookmark file is empty we redirect the user to main vimb page
  fseek(input, 0, SEEK_END);
  if (ftell(input) == 0)
  {
    fprintf(output,
      "<!DOCTYPE HTML>\n"
      "<html lang='en-US'>\n"
      "\t<head>\n"
      "\t\t<meta http-equiv='refresh' content='0; url=%s' />\n"
      "\t</head>\n"
      "</html>\n",
      SETTING_HOME_PAGE);

    fclose(input);
    fclose(output);
    return TRUE;
  }
  fseek(input, 0, SEEK_SET);

  fprintf(output,
    "<!doctype html>\n"
    "<html lang='en'>\n"
    "\t<head>\n"
    "\t\t<meta charset='utf-8'>\n"
    "\t\t<link rel='stylesheet' type='text/css' href='%s/vimb/bookmark.css'>\n"
    "\t\t<title>Bookmarks</title>\n"
    "\t</head>\n"
    "\t<body>\n"
    "\t\t<table>\n"
    "\t\t\t<tr>\n"
    "\t\t\t\t<th>\n"
    "\t\t\t\t\tBookmark\n"
    "\t\t\t\t</th>\n"
    "\t\t\t\t<th>\n"
    "\t\t\t\t\tTag(s)\n"
    "\t\t\t\t</th>\n"
    "\t\t\t</tr>\n",
    (g_get_system_data_dirs())[0]);

  while (fgets(line, sizeof(line), input))
  {
    char* item1 = NULL;
    char* item2 = NULL;
    char* item3 = NULL;

    //Tried this on ugly/malformed strings, seems OK
    //NOT sure though ! Valgrind seems unhappy with it :/
    item1 = strtok(line, "\t");
    item2 = strtok(NULL, "\t");
    item3 = strtok(NULL, "\t");

    //Remove trailing \n if any
    if (item3 == NULL && item2 != NULL)
       item2[strlen(item2) - 1] = '\0';
    if(item3 != NULL)
      item3[strlen(item3) - 1] = '\0';

    if(item1 != NULL && item2 != NULL)
    {
      fprintf(output,
        "\t\t\t<tr>\n"
        "\t\t\t\t<td>\n"
        "\t\t\t\t\t<a href='%s' title='%s'>\n"
        "\t\t\t\t\t\t%s\n"
        "\t\t\t\t\t</a>\n"
        "\t\t\t\t</td>\n"
        "\t\t\t\t<td>\n"
        "%s%s%s"
        "\t\t\t\t</td>\n"
        "\t\t\t</tr>\n",
        item1,
        item2,
        item2,
        (item3 != NULL) ? "\t\t\t\t\t" : "",
        (item3 != NULL) ? item3 : "",
        (item3 != NULL) ? "\n" : "");
    }
  }

  fprintf(output,
    "\t\t</table>\n"
    "\t</body>\n"
    "</html>\n");

  fclose(input);
  fclose(output);
  return TRUE;
}

/**
  * Creates the relevant arguments for the upper function and caals it.
  */
gboolean bookmark_build_html_file()
{
    char *input_path, *output_path;
    gboolean ret;

    input_path = vb.files[FILES_BOOKMARK];
    output_path = g_strconcat(input_path, ".html", NULL);

    ret = bookmark_to_html(input_path, output_path);

    free(input_path);
    free(output_path);

    return ret;
}

#ifdef FEATURE_QUEUE
/**
 * Push a uri to the end of the queue.
 *
 * @uri: URI to put into the queue
 */
gboolean bookmark_queue_push(const char *uri)
{
    return util_file_append(vb.files[FILES_QUEUE], "%s\n", uri);
}

/**
 * Push a uri to the beginning of the queue.
 *
 * @uri: URI to put into the queue
 */
gboolean bookmark_queue_unshift(const char *uri)
{
    return util_file_prepend(vb.files[FILES_QUEUE], "%s\n", uri);
}

/**
 * Retrieves the oldest entry from queue.
 *
 * @item_count: will be filled with the number of remaining items in queue.
 * Returned uri must be freed with g_free.
 */
char *bookmark_queue_pop(int *item_count)
{
    return util_file_pop_line(vb.files[FILES_QUEUE], item_count);
}

/**
 * Removes all contents from the queue file.
 */
gboolean bookmark_queue_clear(void)
{
    FILE *f;
    if ((f = fopen(vb.files[FILES_QUEUE], "w"))) {
        fclose(f);
        return true;
    }
    return false;
}
#endif /* FEATURE_QUEUE */

static GList *load(const char *file)
{
    return util_file_to_unique_list(file, (Util_Content_Func)line_to_bookmark, 0);
}

/**
 * Checks if the given bookmark matches all given query strings as prefix. If
 * the bookmark has no tags, the matching is done on the '/' splited URL.
 *
 * @bm:    bookmark to test if it matches
 * @query: char array with tags to search for
 * @qlen:  length of given query char array
 *
 * Return: true if the bookmark contained all tags
 */
static gboolean bookmark_contains_all_tags(Bookmark *bm, char **query,
    unsigned int qlen)
{
    const char *separators;
    char *cursor;
    unsigned int i;
    gboolean found;

    if (!qlen) {
        return true;
    }

    if (bm->tags) {
        /* If there are tags - use them for matching. */
        separators = " ";
        cursor     = bm->tags;
    } else {
        /* No tags available - matching is based on the path parts of the URL. */
        separators = "./";
        cursor     = bm->uri;
    }

    /* iterate over all query parts */
    for (i = 0; i < qlen; i++) {
        found = false;

        /* we want to do a prefix match on all bookmark tags - so we check for
         * a match on string begin - if this fails we move the cursor to the
         * next space and do the test again */
        while (cursor && *cursor) {
            /* match was not found at current cursor position */
            if (g_str_has_prefix(cursor, query[i])) {
                found = true;
                break;
            }
            /* If match was not found at the cursor position - move cursor
             * behind the next separator char. */
            if ((cursor = strpbrk(cursor, separators))) {
                cursor++;
            }
        }

        if (!found) {
            return false;
        }
    }

    return true;
}

static Bookmark *line_to_bookmark(const char *uri, const char *data)
{
    char *p;
    Bookmark *bm;

    /* data part may consist of title or title<tab>tags */
    bm      = g_slice_new(Bookmark);
    bm->uri = g_strdup(uri);
    if (data && (p = strchr(data, '\t'))) {
        *p        = '\0';
        bm->title = g_strdup(data);
        bm->tags  = g_strdup(p + 1);
    } else {
        bm->title = g_strdup(data);
        bm->tags  = NULL;
    }

    return bm;
}

static void free_bookmark(Bookmark *bm)
{
    g_free(bm->uri);
    g_free(bm->title);
    g_free(bm->tags);
    g_slice_free(Bookmark, bm);
}
