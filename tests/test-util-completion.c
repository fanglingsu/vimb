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
#include <src/util.h>
#include <src/completion.h>

static void *simple_content_func(const char *key, const char *data)
{
    /* return a copy of key as the list item */
    return g_strdup(key);
}

static void test_strv_to_unique_list_basic(void)
{
    char **lines;
    GList *result;

    /* create lines array with duplicates */
    lines = g_strsplit("alpha\nbeta\nalpha\ngamma\nbeta\n", "\n", -1);

    result = util_strv_to_unique_list(lines, simple_content_func, 0);

    /* should have 3 unique items: alpha, beta, gamma */
    g_assert_cmpint(g_list_length(result), ==, 3);

    g_list_free_full(result, g_free);
    g_strfreev(lines);
}

static void test_strv_to_unique_list_max_items(void)
{
    char **lines;
    GList *result;

    lines = g_strsplit("one\ntwo\nthree\nfour\nfive\n", "\n", -1);

    /* limit to 3 items */
    result = util_strv_to_unique_list(lines, simple_content_func, 3);

    g_assert_cmpint(g_list_length(result), <=, 3);

    g_list_free_full(result, g_free);
    g_strfreev(lines);
}

static void test_strv_to_unique_list_null(void)
{
    GList *result;

    /* NULL input should return NULL */
    result = util_strv_to_unique_list(NULL, simple_content_func, 0);
    g_assert_null(result);
}

static void test_strv_to_unique_list_empty(void)
{
    char **lines;
    GList *result;

    /* empty lines - only contains whitespace/empty entries */
    lines = g_strsplit("", "\n", -1);

    result = util_strv_to_unique_list(lines, simple_content_func, 0);

    /* empty strings are stripped/skipped, should be NULL or empty */
    g_assert_cmpint(g_list_length(result), ==, 0);

    g_list_free_full(result, g_free);
    g_strfreev(lines);
}

static void test_strv_to_unique_list_with_tabs(void)
{
    char **lines;
    GList *result, *l;

    /* Lines with tab-separated key\tdata - uniqueness is based on key part */
    lines = g_strsplit("url1\tTitle One\nurl2\tTitle Two\nurl1\tTitle Dup\n", "\n", -1);

    result = util_strv_to_unique_list(lines, simple_content_func, 0);

    /* url1 appears twice but should be deduplicated */
    g_assert_cmpint(g_list_length(result), ==, 2);

    /* verify the keys are url1 and url2 */
    l = result;
    g_assert_true(
        (strcmp(l->data, "url1") == 0 || strcmp(l->data, "url2") == 0)
    );
    l = l->next;
    g_assert_true(
        (strcmp(l->data, "url1") == 0 || strcmp(l->data, "url2") == 0)
    );

    g_list_free_full(result, g_free);
    g_strfreev(lines);
}

static void test_fill_completion_empty_input(void)
{
    GListStore *store;
    GList *src = NULL;

    src = g_list_append(src, "alpha");
    src = g_list_append(src, "beta");
    src = g_list_append(src, "gamma");

    store = g_list_store_new(COMPLETION_TYPE_ITEM);

    /* empty input should match all items */
    g_assert_true(util_fill_completion(store, "", src));
    g_assert_cmpint(g_list_model_get_n_items(G_LIST_MODEL(store)), ==, 3);

    g_list_store_remove_all(store);
    g_list_free(src);
    g_object_unref(store);
}

static void test_fill_completion_prefix_match(void)
{
    GListStore *store;
    GList *src = NULL;

    src = g_list_append(src, "apple");
    src = g_list_append(src, "apricot");
    src = g_list_append(src, "banana");
    src = g_list_append(src, "avocado");

    store = g_list_store_new(COMPLETION_TYPE_ITEM);

    /* "ap" should match "apple" and "apricot" */
    g_assert_true(util_fill_completion(store, "ap", src));
    g_assert_cmpint(g_list_model_get_n_items(G_LIST_MODEL(store)), ==, 2);

    g_list_store_remove_all(store);

    /* "b" should match "banana" only */
    g_assert_true(util_fill_completion(store, "b", src));
    g_assert_cmpint(g_list_model_get_n_items(G_LIST_MODEL(store)), ==, 1);

    g_list_store_remove_all(store);

    /* "z" should match nothing */
    g_assert_false(util_fill_completion(store, "z", src));
    g_assert_cmpint(g_list_model_get_n_items(G_LIST_MODEL(store)), ==, 0);

    g_list_store_remove_all(store);
    g_list_free(src);
    g_object_unref(store);
}

static void test_fill_completion_no_source(void)
{
    GListStore *store;

    store = g_list_store_new(COMPLETION_TYPE_ITEM);

    /* NULL source list with empty input should not find anything */
    g_assert_false(util_fill_completion(store, "", NULL));
    g_assert_cmpint(g_list_model_get_n_items(G_LIST_MODEL(store)), ==, 0);

    g_list_store_remove_all(store);

    /* NULL source list with some input */
    g_assert_false(util_fill_completion(store, "test", NULL));
    g_assert_cmpint(g_list_model_get_n_items(G_LIST_MODEL(store)), ==, 0);

    g_object_unref(store);
}

static void test_fill_completion_exact_match(void)
{
    GListStore *store;
    GList *src = NULL;

    src = g_list_append(src, "exact");
    src = g_list_append(src, "exactly");
    src = g_list_append(src, "other");

    store = g_list_store_new(COMPLETION_TYPE_ITEM);

    /* "exact" should match both "exact" and "exactly" */
    g_assert_true(util_fill_completion(store, "exact", src));
    g_assert_cmpint(g_list_model_get_n_items(G_LIST_MODEL(store)), ==, 2);

    g_list_store_remove_all(store);
    g_list_free(src);
    g_object_unref(store);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/test-util-completion/strv-to-unique-list-basic", test_strv_to_unique_list_basic);
    g_test_add_func("/test-util-completion/strv-to-unique-list-max-items", test_strv_to_unique_list_max_items);
    g_test_add_func("/test-util-completion/strv-to-unique-list-null", test_strv_to_unique_list_null);
    g_test_add_func("/test-util-completion/strv-to-unique-list-empty", test_strv_to_unique_list_empty);
    g_test_add_func("/test-util-completion/strv-to-unique-list-with-tabs", test_strv_to_unique_list_with_tabs);
    g_test_add_func("/test-util-completion/fill-completion-empty-input", test_fill_completion_empty_input);
    g_test_add_func("/test-util-completion/fill-completion-prefix-match", test_fill_completion_prefix_match);
    g_test_add_func("/test-util-completion/fill-completion-no-source", test_fill_completion_no_source);
    g_test_add_func("/test-util-completion/fill-completion-exact-match", test_fill_completion_exact_match);

    return g_test_run();
}
