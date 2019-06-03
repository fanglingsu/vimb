/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2019 Daniel Carl
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

#include <src/file-storage.h>
#include <gio/gio.h>
#include <stdio.h>

static char *pwd;
static char *none_existing_file = "_absent.txt";
static char *created_file       = "_created.txt";
static char *existing_file      = "_existent.txt";

static void test_ephemeral_no_file(void)
{
    FileStorage *s;
    char **lines;
    char *file_path;

    file_path = g_build_filename(pwd, none_existing_file, NULL);

    /* make sure the file does not exist */
    remove(file_path);
    s = file_storage_new(pwd, none_existing_file, TRUE);
    g_assert_nonnull(s);
    g_assert_cmpstr(file_path, ==, file_storage_get_path(s));
    g_assert_true(file_storage_is_readonly(s));

    /* empty file storage */
    lines = file_storage_get_lines(s);
    g_assert_cmpint(g_strv_length(lines), ==, 0);
    g_strfreev(lines);

    file_storage_append(s, "-%s\n", "foo");
    file_storage_append(s, "-%s\n", "bar");

    /* File must not be created on appending data to storage */
    g_assert_false(g_file_test(file_path, G_FILE_TEST_IS_REGULAR));

    lines = file_storage_get_lines(s);
    g_assert_cmpint(g_strv_length(lines), ==, 3);
    g_assert_cmpstr(lines[0], ==, "-foo");
    g_assert_cmpstr(lines[1], ==, "-bar");
    g_strfreev(lines);
    file_storage_free(s);
    g_free(file_path);
}

static void test_file_created(void)
{
    FileStorage *s;
    char *file_path;

    file_path = g_build_filename(pwd, created_file, NULL);
    remove(file_path);

    g_assert_false(g_file_test(file_path, G_FILE_TEST_IS_REGULAR));
    s = file_storage_new(pwd, created_file, FALSE);
    g_assert_false(file_storage_is_readonly(s));
    g_assert_cmpstr(file_path, ==, file_storage_get_path(s));

    /* check that file is created only on first write */
    g_assert_false(g_file_test(file_path, G_FILE_TEST_IS_REGULAR));
    file_storage_append(s, "");
    g_assert_true(g_file_test(file_path, G_FILE_TEST_IS_REGULAR));

    file_storage_free(s);
    g_free(file_path);
}

static void test_ephemeral_with_file(void)
{
    FileStorage *s;
    char *file_path;
    char **lines;
    char *content = NULL;
    gboolean result;

    file_path = g_build_filename(pwd, existing_file, NULL);

    s = file_storage_new(pwd, existing_file, TRUE);
    g_assert_nonnull(s);
    g_assert_true(file_storage_is_readonly(s));
    g_assert_cmpstr(file_path, ==, file_storage_get_path(s));

    /* file does not exists yet */
    lines = file_storage_get_lines(s);
    g_assert_cmpint(g_strv_length(lines), ==, 0);
    g_strfreev(lines);

    /* empty file storage but file with two lines */
    result = g_file_set_contents(file_path, "one\n", -1, NULL);
    g_assert_true(result);
    lines = file_storage_get_lines(s);
    g_assert_cmpint(g_strv_length(lines), ==, 2);
    g_strfreev(lines);

    file_storage_append(s, "%s\n", "two ephemeral");

    lines = file_storage_get_lines(s);
    g_assert_cmpint(g_strv_length(lines), ==, 3);
    g_assert_cmpstr(lines[0], ==, "one");
    g_assert_cmpstr(lines[1], ==, "two ephemeral");
    g_strfreev(lines);

    /* now make sure the file was not changed */
    g_file_get_contents(file_path, &content, NULL, NULL);
    g_assert_cmpstr(content, ==, "one\n");

    file_storage_free(s);
    g_free(file_path);
}

int main(int argc, char *argv[])
{
    int result;
    g_test_init(&argc, &argv, NULL);

    pwd = g_get_current_dir();

    g_test_add_func("/test-file-storage/ephemeral-no-file", test_ephemeral_no_file);
    g_test_add_func("/test-file-storage/file-created", test_file_created);
    g_test_add_func("/test-file-storage/ephemeral-with-file", test_ephemeral_with_file);

    result = g_test_run();

    remove(existing_file);
    remove(created_file);
    g_free(pwd);

    return result;
}
