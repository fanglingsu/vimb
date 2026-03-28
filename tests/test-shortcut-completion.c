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
#include <src/shortcut.h>
#include <src/completion.h>
#include <src/main.h>

static Shortcut *sc = NULL;

static void test_shortcut_fill_completion_all(void)
{
    GListStore *store;

    store = g_list_store_new(COMPLETION_TYPE_ITEM);

    /* empty input should return all shortcuts */
    g_assert_true(shortcut_fill_completion(sc, store, ""));
    g_assert_cmpint(g_list_model_get_n_items(G_LIST_MODEL(store)), >=, 3);

    g_list_store_remove_all(store);
    g_object_unref(store);
}

static void test_shortcut_fill_completion_prefix(void)
{
    GListStore *store;

    store = g_list_store_new(COMPLETION_TYPE_ITEM);

    /* "goo" should match "google" only */
    g_assert_true(shortcut_fill_completion(sc, store, "goo"));
    g_assert_cmpint(g_list_model_get_n_items(G_LIST_MODEL(store)), ==, 1);

    g_list_store_remove_all(store);

    /* "d" should match "duck" and "devs" */
    g_assert_true(shortcut_fill_completion(sc, store, "d"));
    g_assert_cmpint(g_list_model_get_n_items(G_LIST_MODEL(store)), ==, 2);

    g_list_store_remove_all(store);
    g_object_unref(store);
}

static void test_shortcut_fill_completion_no_match(void)
{
    GListStore *store;

    store = g_list_store_new(COMPLETION_TYPE_ITEM);

    /* no shortcut starts with "zzz" */
    g_assert_false(shortcut_fill_completion(sc, store, "zzz"));
    g_assert_cmpint(g_list_model_get_n_items(G_LIST_MODEL(store)), ==, 0);

    g_list_store_remove_all(store);
    g_object_unref(store);
}

static void test_shortcut_get_uri_no_placeholder(void)
{
    char *uri;

    /* shortcut without any placeholder */
    g_assert_true(shortcut_add(sc, "home", "https://start.page/"));

    uri = shortcut_get_uri(sc, "home");
    g_assert_cmpstr(uri, ==, "https://start.page/");
    g_free(uri);
}

static void test_shortcut_get_uri_no_match(void)
{
    char *uri;

    /* non-existent shortcut without default -> should try default */
    uri = shortcut_get_uri(sc, "nonexistent_sc query");
    /* since we set a default, it should use the default shortcut */
    g_assert_nonnull(uri);
    g_free(uri);
}

static void test_shortcut_set_default_before_add(void)
{
    Shortcut *sc2 = shortcut_new();
    char *uri;

    /* set default before the shortcut is added */
    g_assert_true(shortcut_set_default(sc2, "late"));
    g_assert_true(shortcut_add(sc2, "late", "https://late.example.com/$0"));

    uri = shortcut_get_uri(sc2, "some query");
    g_assert_cmpstr(uri, ==, "https://late.example.com/some%20query");
    g_free(uri);

    shortcut_free(sc2);
}

static void test_shortcut_overwrite(void)
{
    Shortcut *sc2 = shortcut_new();
    char *uri;

    /* add a shortcut and then overwrite it */
    g_assert_true(shortcut_add(sc2, "test", "https://old.example.com/$0"));
    g_assert_true(shortcut_add(sc2, "test", "https://new.example.com/$0"));

    uri = shortcut_get_uri(sc2, "test query");
    g_assert_cmpstr(uri, ==, "https://new.example.com/query");
    g_free(uri);

    shortcut_free(sc2);
}

static void test_shortcut_remove_nonexistent(void)
{
    Shortcut *sc2 = shortcut_new();

    /* removing a non-existent shortcut should return FALSE */
    g_assert_false(shortcut_remove(sc2, "does_not_exist"));

    shortcut_free(sc2);
}

int main(int argc, char *argv[])
{
    int result;
    sc = shortcut_new();

    g_assert_true(shortcut_add(sc, "google", "https://www.google.com/search?q=$0"));
    g_assert_true(shortcut_add(sc, "duck", "https://duckduckgo.com/?q=$0"));
    g_assert_true(shortcut_add(sc, "devs", "https://devdocs.io/#q=$0"));
    g_assert_true(shortcut_set_default(sc, "google"));

    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/test-shortcut-completion/fill-all", test_shortcut_fill_completion_all);
    g_test_add_func("/test-shortcut-completion/fill-prefix", test_shortcut_fill_completion_prefix);
    g_test_add_func("/test-shortcut-completion/fill-no-match", test_shortcut_fill_completion_no_match);
    g_test_add_func("/test-shortcut-completion/get-uri-no-placeholder", test_shortcut_get_uri_no_placeholder);
    g_test_add_func("/test-shortcut-completion/get-uri-no-match", test_shortcut_get_uri_no_match);
    g_test_add_func("/test-shortcut-completion/set-default-before-add", test_shortcut_set_default_before_add);
    g_test_add_func("/test-shortcut-completion/overwrite", test_shortcut_overwrite);
    g_test_add_func("/test-shortcut-completion/remove-nonexistent", test_shortcut_remove_nonexistent);

    result = g_test_run();

    shortcut_free(sc);

    return result;
}
