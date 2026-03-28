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
#include <string.h>
#include <src/main.h>
#include <src/map.h>

/* provide a minimal Vimb struct required by map.c */
struct Vimb vb;

static Client *test_client = NULL;

static void setup_client(void)
{
    test_client = g_new0(Client, 1);
    map_init(test_client);
}

static void teardown_client(void)
{
    map_cleanup(test_client);
    g_free(test_client);
    test_client = NULL;
}

static void test_map_insert_and_delete(void)
{
    setup_client();

    /* insert a mapping for normal mode 'n' */
    map_insert(test_client, "gg", "G", 'n', FALSE);

    /* the mapping should be in the list */
    g_assert_nonnull(test_client->map.list);
    g_assert_cmpint(g_slist_length(test_client->map.list), ==, 1);

    /* verify the mapping contents */
    Map *m = (Map*)test_client->map.list->data;
    g_assert_cmpint(m->inlen, ==, 2);
    g_assert_cmpint(m->mappedlen, ==, 1);
    g_assert_cmpint(m->mode, ==, 'n');
    g_assert_false(m->remap);

    /* delete the mapping */
    g_assert_true(map_delete(test_client, "gg", 'n'));

    /* list should be empty */
    g_assert_cmpint(g_slist_length(test_client->map.list), ==, 0);

    teardown_client();
}

static void test_map_delete_nonexistent(void)
{
    setup_client();

    /* deleting a mapping that was never inserted should return FALSE */
    g_assert_false(map_delete(test_client, "xx", 'n'));

    teardown_client();
}

static void test_map_insert_multiple_modes(void)
{
    setup_client();

    /* insert mappings for different modes */
    map_insert(test_client, "dd", "delete-line", 'n', FALSE);
    map_insert(test_client, "jj", "<Esc>", 'i', FALSE);
    map_insert(test_client, "qq", "quit", 'c', TRUE);

    g_assert_cmpint(g_slist_length(test_client->map.list), ==, 3);

    /* delete only the insert mode mapping */
    g_assert_true(map_delete(test_client, "jj", 'i'));
    g_assert_cmpint(g_slist_length(test_client->map.list), ==, 2);

    /* deleting same mapping again should fail */
    g_assert_false(map_delete(test_client, "jj", 'i'));

    /* the normal mode mapping should still be there */
    g_assert_true(map_delete(test_client, "dd", 'n'));
    g_assert_cmpint(g_slist_length(test_client->map.list), ==, 1);

    /* the command mode mapping with remap should still be there */
    Map *m = (Map*)test_client->map.list->data;
    g_assert_true(m->remap);
    g_assert_cmpint(m->mode, ==, 'c');

    teardown_client();
}

static void test_map_insert_overwrite(void)
{
    setup_client();

    /* insert a mapping and then overwrite with a new rhs */
    map_insert(test_client, "gg", "old-mapping", 'n', FALSE);
    g_assert_cmpint(g_slist_length(test_client->map.list), ==, 1);

    /* overwrite the same lhs */
    map_insert(test_client, "gg", "new-mapping", 'n', TRUE);

    /* should still be just 1 mapping (the old was removed) */
    g_assert_cmpint(g_slist_length(test_client->map.list), ==, 1);

    /* verify the new mapping is in effect */
    Map *m = (Map*)test_client->map.list->data;
    g_assert_true(m->remap);

    teardown_client();
}

static void test_map_delete_wrong_mode(void)
{
    setup_client();

    /* insert for normal mode */
    map_insert(test_client, "gg", "top", 'n', FALSE);

    /* deleting for insert mode should fail */
    g_assert_false(map_delete(test_client, "gg", 'i'));

    /* original mapping still there */
    g_assert_cmpint(g_slist_length(test_client->map.list), ==, 1);

    teardown_client();
}

static void test_map_special_keys(void)
{
    setup_client();

    /* test mapping with special key labels like <CR> */
    map_insert(test_client, "<CR>", "enter-action", 'n', FALSE);
    g_assert_cmpint(g_slist_length(test_client->map.list), ==, 1);

    /* the <CR> label should have been converted to internal representation */
    Map *m = (Map*)test_client->map.list->data;
    /* <CR> becomes \x0d which is 1 byte */
    g_assert_cmpint(m->inlen, ==, 1);
    g_assert_cmpint(m->in[0], ==, '\x0d');

    /* delete using the same key label */
    g_assert_true(map_delete(test_client, "<CR>", 'n'));
    g_assert_cmpint(g_slist_length(test_client->map.list), ==, 0);

    teardown_client();
}

static void test_map_ctrl_key(void)
{
    setup_client();

    /* test mapping with <C-x> style control keys */
    map_insert(test_client, "<C-f>", "page-down", 'n', FALSE);
    g_assert_cmpint(g_slist_length(test_client->map.list), ==, 1);

    Map *m = (Map*)test_client->map.list->data;
    /* <C-f> = 'f' - 0x60 = 0x06, so inlen should be 1 */
    g_assert_cmpint(m->inlen, ==, 1);

    g_assert_true(map_delete(test_client, "<C-f>", 'n'));

    teardown_client();
}

static void test_map_init_cleanup(void)
{
    Client *c = g_new0(Client, 1);

    map_init(c);

    /* queue should be initialized */
    g_assert_nonnull(c->map.queue);
    g_assert_cmpint(c->map.timeoutlen, ==, 1000);

    map_cleanup(c);

    g_free(c);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    memset(&vb, 0, sizeof(vb));

    g_test_add_func("/test-map/insert-and-delete", test_map_insert_and_delete);
    g_test_add_func("/test-map/delete-nonexistent", test_map_delete_nonexistent);
    g_test_add_func("/test-map/insert-multiple-modes", test_map_insert_multiple_modes);
    g_test_add_func("/test-map/insert-overwrite", test_map_insert_overwrite);
    g_test_add_func("/test-map/delete-wrong-mode", test_map_delete_wrong_mode);
    g_test_add_func("/test-map/special-keys", test_map_special_keys);
    g_test_add_func("/test-map/ctrl-key", test_map_ctrl_key);
    g_test_add_func("/test-map/init-cleanup", test_map_init_cleanup);

    return g_test_run();
}
