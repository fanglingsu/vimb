/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2015 Daniel Carl
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
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkkeysyms-compat.h>
#include <src/map.h>
#include <src/main.h>

static char queue[20];  /* receives the keypresses */
static int  qpos = 0;   /* points to the queue entry for the next keypress */

#define QUEUE_APPEND(c) {    \
    queue[qpos++] = (char)c; \
    queue[qpos]   = '\0';    \
}
#define QUEUE_CLEAR() {queue[(qpos = 0)] = '\0';}
#define ASSERT_MAPPING(s, e) {  \
    QUEUE_CLEAR();                   \
    map_handle_string(s, true);      \
    g_assert_cmpstr(queue, ==, e);   \
}

typedef struct {
    guint state;
    guint keyval;
} TestKeypress;

VbResult keypress(int key)
{
    /* put the key into the queue */
    QUEUE_APPEND(key);

    return RESULT_COMPLETE;
}

static void test_handle_string_simple(void)
{
    /* test simple mappings */
    ASSERT_MAPPING("a", "[a]");
    ASSERT_MAPPING("b", "[b]");
    ASSERT_MAPPING("[c]", "c");
    ASSERT_MAPPING("d", " [d]");
    ASSERT_MAPPING("<Tab>", "[tab]");
    ASSERT_MAPPING("<S-Tab>", "[shift-tab]");
    ASSERT_MAPPING("<C-F>", "[ctrl-f]");
    ASSERT_MAPPING("<C-f>", "[ctrl-f]");
    ASSERT_MAPPING("<CR>", "[cr]");
    ASSERT_MAPPING("foobar", "[baz]");

    /* key sequences that are not changed by mappings */
    ASSERT_MAPPING("fghi", "fghi");
}

static void test_handle_string_alias(void)
{
    /* CTRL-I is the same like <Tab> and CTRL-M like <CR> */
    ASSERT_MAPPING("<C-I>", "[tab]");
    ASSERT_MAPPING("<C-M>", "[cr]");
}

static void test_handle_string_remapped(void)
{
    /* test multiple mappings together */
    ASSERT_MAPPING("ba", "[b][a]");
    ASSERT_MAPPING("ab12345[c]", "[a][b]12345c");

    /* incomplete ambiguous sequences are not matched jet */
    ASSERT_MAPPING("foob", "");
    ASSERT_MAPPING("ar", "[baz]");

    /* test remapping */
    map_insert("c", "baza", 't', true);
    ASSERT_MAPPING("c", "[b][a]z[a]");
    map_insert("d", "cki", 't', true);
    ASSERT_MAPPING("d", "[b][a]z[a]ki");

    /* remove the no more needed mappings */
    map_delete("c", 't');
    map_delete("d", 't');
}

static void test_handle_string_overrule(void)
{
    /* add another map for 'a' and check if this overrules the previous set */
    map_insert("a", "overruled", 't', false);
    ASSERT_MAPPING("a", "overruled");
}

static void test_remove(void)
{
    map_insert("x", "[x]", 't', false);
    /* make sure that the mapping works */
    ASSERT_MAPPING("x", "[x]");

    map_delete("x", 't');

    /* make sure the mapping  removed */
    ASSERT_MAPPING("x", "x");
}

static void test_keypress_single(void)
{
    QUEUE_CLEAR();

    map_keypress(NULL, &((GdkEventKey){.keyval = GDK_Tab, .state = GDK_SHIFT_MASK}), NULL);
    g_assert_cmpstr(queue, ==, "[shift-tab]");
}

static void test_keypress_sequence(void)
{
    QUEUE_CLEAR();

    map_keypress(NULL, &((GdkEventKey){.keyval = GDK_f}), NULL);
    g_assert_cmpstr(queue, ==, "");
    map_keypress(NULL, &((GdkEventKey){.keyval = GDK_o}), NULL);
    g_assert_cmpstr(queue, ==, "");
    map_keypress(NULL, &((GdkEventKey){.keyval = GDK_o}), NULL);
    g_assert_cmpstr(queue, ==, "");
    map_keypress(NULL, &((GdkEventKey){.keyval = GDK_b}), NULL);
    g_assert_cmpstr(queue, ==, "");
    map_keypress(NULL, &((GdkEventKey){.keyval = GDK_a}), NULL);
    g_assert_cmpstr(queue, ==, "");
    map_keypress(NULL, &((GdkEventKey){.keyval = GDK_r}), NULL);
    g_assert_cmpstr(queue, ==, "[baz]");
}

int main(int argc, char *argv[])
{
    int result;
    g_test_init(&argc, &argv, NULL);

    /* add a test mode to handle the maped sequences */
    vb_add_mode('t', NULL, NULL, keypress, NULL);
    vb_enter('t');
    map_init();

    g_test_add_func("/test-map/handle_string/simple", test_handle_string_simple);
    g_test_add_func("/test-map/handle_string/alias", test_handle_string_alias);
    g_test_add_func("/test-map/handle_string/remapped", test_handle_string_remapped);
    g_test_add_func("/test-map/handle_string/overrule", test_handle_string_overrule);
    g_test_add_func("/test-map/remove", test_remove);
    g_test_add_func("/test-map/keypress/single-char", test_keypress_single);
    g_test_add_func("/test-map/keypress/sequence", test_keypress_sequence);

    /* add some mappings to test */
    map_insert("a", "[a]", 't', false);         /* inlen < mappedlen  */
    map_insert("b", "[b]", 't', false);
    map_insert("d", "<Space>[d]", 't', false);
    map_insert("[c]", "c", 't', false);         /* inlen > mappedlen  */
    map_insert("foobar", "[baz]", 't', false);
    map_insert("<Tab>", "[tab]", 't', false);
    map_insert("<S-Tab>", "[shift-tab]", 't', false);
    map_insert("<C-F>", "[ctrl-f]", 't', false);
    map_insert("<CR>", "[cr]", 't', false);

    result = g_test_run();
    map_cleanup();
    return result;
}
