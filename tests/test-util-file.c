/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2026 Daniel Carl
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

#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <src/util.h>

static char *tmpdir;

static void test_sanitize_filename(void)
{
    char *result;

    /* directory separators are replaced with underscore */
    result = g_strdup("path/to/file");
    util_sanitize_filename(result);
    g_assert_cmpstr(result, ==, "path_to_file");
    g_free(result);

    /* no separator - string unchanged */
    result = g_strdup("simple.txt");
    util_sanitize_filename(result);
    g_assert_cmpstr(result, ==, "simple.txt");
    g_free(result);

    /* empty string */
    result = g_strdup("");
    util_sanitize_filename(result);
    g_assert_cmpstr(result, ==, "");
    g_free(result);

    /* leading separator */
    result = g_strdup("/leading");
    util_sanitize_filename(result);
    g_assert_cmpstr(result, ==, "_leading");
    g_free(result);

    /* trailing separator */
    result = g_strdup("trailing/");
    util_sanitize_filename(result);
    g_assert_cmpstr(result, ==, "trailing_");
    g_free(result);
}

static void test_sanitize_uri(void)
{
    char *result;

    /* NULL input returns NULL */
    result = util_sanitize_uri(NULL);
    g_assert_null(result);

    /* simple uri without credentials - returned as is */
    result = util_sanitize_uri("http://example.com/path");
    g_assert_nonnull(result);
    g_free(result);

    /* uri with credentials - password should be hidden by G_URI_HIDE_PASSWORD */
    result = util_sanitize_uri("http://user:secret@example.com/path");
    g_assert_nonnull(result);
    /* the password must be stripped from the result */
    g_assert_cmpstr(result, ==, "http://user@example.com/path");
    g_free(result);

    /* uri without @ sign should pass through */
    result = util_sanitize_uri("https://example.com");
    g_assert_nonnull(result);
    g_assert_cmpstr(result, ==, "https://example.com");
    g_free(result);
}

static void test_create_tmp_file(void)
{
    char *file = NULL;
    gboolean result;
    char *content = NULL;

    /* create temp file with content */
    result = util_create_tmp_file("hello world", &file);
    g_assert_true(result);
    g_assert_nonnull(file);
    g_assert_true(g_file_test(file, G_FILE_TEST_IS_REGULAR));

    /* verify content */
    g_file_get_contents(file, &content, NULL, NULL);
    g_assert_cmpstr(content, ==, "hello world");
    g_free(content);

    remove(file);
    g_free(file);

    /* create temp file with NULL content */
    file = NULL;
    result = util_create_tmp_file(NULL, &file);
    g_assert_true(result);
    g_assert_nonnull(file);
    g_assert_true(g_file_test(file, G_FILE_TEST_IS_REGULAR));

    remove(file);
    g_free(file);
}

static void test_file_set_content(void)
{
    char *filepath;
    char *readback = NULL;

    filepath = g_build_filename(tmpdir, "test_set_content.txt", NULL);

    /* write content */
    g_assert_true(util_file_set_content(filepath, "first content"));

    /* verify it was written */
    g_file_get_contents(filepath, &readback, NULL, NULL);
    g_assert_cmpstr(readback, ==, "first content");
    g_free(readback);

    /* overwrite with new content (atomic) */
    g_assert_true(util_file_set_content(filepath, "second content"));

    g_file_get_contents(filepath, &readback, NULL, NULL);
    g_assert_cmpstr(readback, ==, "second content");
    g_free(readback);

    /* empty content */
    g_assert_true(util_file_set_content(filepath, ""));

    g_file_get_contents(filepath, &readback, NULL, NULL);
    g_assert_cmpstr(readback, ==, "");
    g_free(readback);

    remove(filepath);
    g_free(filepath);
}

static void test_file_append(void)
{
    char *filepath;
    char *readback = NULL;

    filepath = g_build_filename(tmpdir, "test_append.txt", NULL);
    remove(filepath);

    /* append to new file */
    g_assert_true(util_file_append(filepath, "line1\n"));

    /* append more */
    g_assert_true(util_file_append(filepath, "line2\n"));

    /* verify both lines present */
    g_file_get_contents(filepath, &readback, NULL, NULL);
    g_assert_cmpstr(readback, ==, "line1\nline2\n");
    g_free(readback);

    /* NULL file should return FALSE */
    g_assert_false(util_file_append(NULL, "data"));

    remove(filepath);
    g_free(filepath);
}

static void test_file_prepend(void)
{
    char *filepath;
    char *readback = NULL;

    filepath = g_build_filename(tmpdir, "test_prepend.txt", NULL);

    /* create file with initial content */
    g_file_set_contents(filepath, "original\n", -1, NULL);

    /* prepend new content */
    g_assert_true(util_file_prepend(filepath, "prepended\n"));

    g_file_get_contents(filepath, &readback, NULL, NULL);
    g_assert_cmpstr(readback, ==, "prepended\noriginal\n");
    g_free(readback);

    /* NULL file should return FALSE */
    g_assert_false(util_file_prepend(NULL, "data"));

    remove(filepath);
    g_free(filepath);
}

static void test_get_lines(void)
{
    char *filepath;
    char **lines;

    filepath = g_build_filename(tmpdir, "test_get_lines.txt", NULL);

    /* create file with known content */
    g_file_set_contents(filepath, "alpha\nbeta\ngamma\n", -1, NULL);

    lines = util_get_lines(filepath);
    g_assert_nonnull(lines);
    /* g_strsplit on "alpha\nbeta\ngamma\n" with "\n" gives 4 parts:
     * "alpha", "beta", "gamma", "" */
    g_assert_cmpint(g_strv_length(lines), ==, 4);
    g_assert_cmpstr(lines[0], ==, "alpha");
    g_assert_cmpstr(lines[1], ==, "beta");
    g_assert_cmpstr(lines[2], ==, "gamma");
    g_assert_cmpstr(lines[3], ==, "");
    g_strfreev(lines);

    /* NULL filename returns NULL */
    g_assert_null(util_get_lines(NULL));

    /* non-existent file returns NULL */
    g_assert_null(util_get_lines("/nonexistent/path/file.txt"));

    remove(filepath);
    g_free(filepath);
}

static void test_file_prepend_line(void)
{
    char *filepath;
    char **lines;

    filepath = g_build_filename(tmpdir, "test_prepend_line.txt", NULL);

    /* create file with 3 lines */
    g_file_set_contents(filepath, "one\ntwo\nthree\n", -1, NULL);

    /* prepend a line with max 3 lines */
    util_file_prepend_line(filepath, "zero", 3);

    lines = util_get_lines(filepath);
    g_assert_nonnull(lines);
    /* should have: zero, one, two (three removed due to max_lines=3)
     * plus trailing empty from final \n */
    g_assert_cmpstr(lines[0], ==, "zero");
    g_assert_cmpstr(lines[1], ==, "one");
    g_assert_cmpstr(lines[2], ==, "two");
    g_strfreev(lines);

    /* prepend another line with max 2 */
    util_file_prepend_line(filepath, "new", 2);

    lines = util_get_lines(filepath);
    g_assert_nonnull(lines);
    g_assert_cmpstr(lines[0], ==, "new");
    g_assert_cmpstr(lines[1], ==, "zero");
    g_strfreev(lines);

    remove(filepath);
    g_free(filepath);
}

static void test_file_pop_line(void)
{
    char *filepath;
    char *popped;
    int remaining;

    filepath = g_build_filename(tmpdir, "test_pop_line.txt", NULL);

    /* create file with 3 lines */
    g_file_set_contents(filepath, "first\nsecond\nthird\n", -1, NULL);

    /* pop the first line */
    popped = util_file_pop_line(filepath, &remaining);
    g_assert_cmpstr(popped, ==, "first");
    g_assert_cmpint(remaining, >=, 1);
    g_free(popped);

    /* pop again - should get second */
    popped = util_file_pop_line(filepath, &remaining);
    g_assert_cmpstr(popped, ==, "second");
    g_free(popped);

    /* pop again - should get third */
    popped = util_file_pop_line(filepath, &remaining);
    g_assert_cmpstr(popped, ==, "third");
    g_free(popped);

    /* NULL file should return NULL */
    g_assert_null(util_file_pop_line(NULL, NULL));

    remove(filepath);
    g_free(filepath);
}

static void test_get_file_contents(void)
{
    char *filepath;
    char *content;
    gsize length = 0;

    filepath = g_build_filename(tmpdir, "test_get_contents.txt", NULL);

    /* create file with known content */
    g_file_set_contents(filepath, "hello vimb", -1, NULL);

    content = util_get_file_contents(filepath, &length);
    g_assert_nonnull(content);
    g_assert_cmpstr(content, ==, "hello vimb");
    g_assert_cmpuint(length, ==, 10);
    g_free(content);

    /* NULL filename returns NULL */
    g_assert_null(util_get_file_contents(NULL, NULL));

    remove(filepath);
    g_free(filepath);
}

int main(int argc, char *argv[])
{
    int result;
    g_test_init(&argc, &argv, NULL);

    tmpdir = g_dir_make_tmp("vimb-test-XXXXXX", NULL);

    g_test_add_func("/test-util-file/sanitize-filename", test_sanitize_filename);
    g_test_add_func("/test-util-file/sanitize-uri", test_sanitize_uri);
    g_test_add_func("/test-util-file/create-tmp-file", test_create_tmp_file);
    g_test_add_func("/test-util-file/file-set-content", test_file_set_content);
    g_test_add_func("/test-util-file/file-append", test_file_append);
    g_test_add_func("/test-util-file/file-prepend", test_file_prepend);
    g_test_add_func("/test-util-file/get-lines", test_get_lines);
    g_test_add_func("/test-util-file/file-prepend-line", test_file_prepend_line);
    g_test_add_func("/test-util-file/file-pop-line", test_file_pop_line);
    g_test_add_func("/test-util-file/get-file-contents", test_get_file_contents);

    result = g_test_run();

    g_rmdir(tmpdir);
    g_free(tmpdir);

    return result;
}
