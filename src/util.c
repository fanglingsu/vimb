#include "util.h"
#include <stdio.h>
#include <glib.h>

char* util_get_config_dir(void)
{
    char *path = g_build_filename(g_get_user_config_dir(), PROJECT, NULL);
    util_create_dir_if_not_exists(path);

    return path;
}

char* util_get_cache_dir(void)
{
    char *path = g_build_filename(g_get_user_cache_dir(), PROJECT, NULL);
    util_create_dir_if_not_exists(path);

    return path;
}

void util_create_dir_if_not_exists(const gchar* dirpath)
{
    if (!g_file_test(dirpath, G_FILE_TEST_IS_DIR)) {
        g_mkdir_with_parents(dirpath, 0755);
    }
}

void util_create_file_if_not_exists(const char* filename) {
    if (!g_file_test(filename, G_FILE_TEST_IS_REGULAR)) {
        FILE* f = fopen(filename, "a");
        fclose(f);
    }
}
