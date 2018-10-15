/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2018 Daniel Carl
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
#include <src/handler.h>
#include <src/completion.h>

static Handler *handler = NULL;

#define TEST_URI "http://fanglingsu.github.io/vimb/"

static void test_handler_add(void)
{
    g_assert_true(handler_add(handler, "https", "e"));
}

static void test_handler_remove(void)
{
    g_assert_true(handler_add(handler, "https", "e"));

    g_assert_true(handler_remove(handler, "https"));
    g_assert_false(handler_remove(handler, "https"));
}

static void test_handler_run_success(void)
{
    if (g_test_subprocess()) {
        handler_add(handler, "http", "echo -n 'handled uri %s'");
        handler_handle_uri(handler, TEST_URI);
        return;
    }
    g_test_trap_subprocess(NULL, 0, 0);
    g_test_trap_assert_passed();
    g_test_trap_assert_stdout("handled uri " TEST_URI);
}

static void test_handler_run_failed(void)
{
    if (g_test_subprocess()) {
        handler_add(handler, "http", "unknown-program %s");
        handler_handle_uri(handler, TEST_URI);
        return;
    }
    g_test_trap_subprocess(NULL, 0, 0);
    g_test_trap_assert_failed();
    g_test_trap_assert_stderr("*Can't run *unknown-program*");
}

static void test_handler_fill_completion(void)
{
    GtkListStore *store;
    g_assert_true(handler_add(handler, "http", "echo"));
    g_assert_true(handler_add(handler, "https", "echo"));
    g_assert_true(handler_add(handler, "about", "echo"));
    g_assert_true(handler_add(handler, "ftp", "echo"));

    store = gtk_list_store_new(COMPLETION_STORE_NUM, G_TYPE_STRING, G_TYPE_STRING);
    /* check case where multiple matches are found */
    g_assert_true(handler_fill_completion(handler, store, "http"));
    g_assert_cmpint(gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), NULL), ==, 2);
    gtk_list_store_clear(store);

    /* check case where only one matches are found */
    g_assert_true(handler_fill_completion(handler, store, "f"));
    g_assert_cmpint(gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), NULL), ==, 1);
    gtk_list_store_clear(store);

    /* check case where no match is found */
    g_assert_false(handler_fill_completion(handler, store, "unknown"));
    g_assert_cmpint(gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), NULL), ==, 0);
    gtk_list_store_clear(store);

    /* check case without apllied filters */
    g_assert_true(handler_fill_completion(handler, store, ""));
    g_assert_cmpint(gtk_tree_model_iter_n_children(GTK_TREE_MODEL(store), NULL), ==, 4);
    gtk_list_store_clear(store);
}

int main(int argc, char *argv[])
{
    int result;
    handler = handler_new();

    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/test-handlers/add", test_handler_add);
    g_test_add_func("/test-handlers/remove", test_handler_remove);
    g_test_add_func("/test-handlers/handle_uri/success", test_handler_run_success);
    g_test_add_func("/test-handlers/handle_uri/failed", test_handler_run_failed); 
    g_test_add_func("/test-handlers/fill-completion", test_handler_fill_completion); 
    result = g_test_run();

    handler_free(handler);

    return result;
}
