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
#include <src/shortcut.h>
#include <src/main.h>

Shortcut *sc = NULL;

static void test_shortcut(void)
{
    char *uri;
    unsigned int i;
    struct {
        char *in;
        char *out;
    } data[] = {
        /* call with shortcut identifier */
        {"_vimb1_ zero one", "only-zero:zero%20one"},
        /* don't fail on unmatches quotes if there are only $0 placeholders */
        {"_vimb1_ 'unmatches quote", "only-zero:'unmatches%20quote"},
        /* check if all placeholders $0 are replaces */
        {"_vimb5_ one two", "double-zero:one%20two-one%20two"},
        /* check the defautl shortcut is used */
        {"zero one two three", "default:zero-two%20three"},
        /* don't remove non matched placeholders */
        {"zero", "default:zero-$2"},
        {"_vimb3_ zero one two three four five six seven eight nine", "fullrange:zero-one-nine"}
    };

    for (i = 0; i < LENGTH(data); i++) {
        uri = shortcut_get_uri(sc, data->in);
        g_assert_cmpstr(uri, ==, data->out);
        g_free(uri);
    }
}

static void test_shortcut_shell_param(void)
{
    char *uri;

    /* double quotes */
    uri = shortcut_get_uri(sc, "_vimb6_ \"rail station\" city hall");
    g_assert_cmpstr(uri, ==, "shell:rail%20station-city%20hall");
    g_free(uri);

    /* single quotes */
    uri = shortcut_get_uri(sc, "_vimb6_ 'rail station' 'city hall'");
    g_assert_cmpstr(uri, ==, "shell:rail%20station-city%20hall");
    g_free(uri);

    /* ignore none matching quote errors */
    uri = shortcut_get_uri(sc, "_vimb6_ \"rail station\" \"city hall");
    g_assert_cmpstr(uri, ==, "shell:rail%20station-city%20hall");
    g_free(uri);

    /* don't fill up quoted param with unquoted stuff */
    uri = shortcut_get_uri(sc, "_vimb6_ \"param 1\" \"param 2\" ignored params");
    g_assert_cmpstr(uri, ==, "shell:param%201-param%202");
    g_free(uri);

    /* allo quotes within tha last parameter */
    uri = shortcut_get_uri(sc, "_vimb6_ param1 param2 \"containing quotes\"");
    g_assert_cmpstr(uri, ==, "shell:param1-param2%20%22containing%20quotes%22");
    g_free(uri);
}

static void test_shortcut_remove(void)
{
    char *uri;

    g_assert_true(shortcut_remove(sc, "_vimb4_"));

    /* check if the shortcut is really no used */
    uri = shortcut_get_uri(sc, "_vimb4_ test");
    g_assert_cmpstr(uri, ==, "default:_vimb4_-$2");
    g_free(uri);

    g_assert_false(shortcut_remove(sc, "_vimb4_"));
}

int main(int argc, char *argv[])
{
    int result;
    sc = shortcut_new();

    g_assert_true(shortcut_add(sc, "_vimb1_", "only-zero:$0"));
    g_assert_true(shortcut_add(sc, "_vimb2_", "default:$0-$2"));
    g_assert_true(shortcut_add(sc, "_vimb3_", "fullrange:$0-$1-$9"));
    g_assert_true(shortcut_add(sc, "_vimb4_", "for-remove:$0"));
    g_assert_true(shortcut_add(sc, "_vimb5_", "double-zero:$0-$0"));
    g_assert_true(shortcut_add(sc, "_vimb6_", "shell:$0-$1"));
    g_assert_true(shortcut_set_default(sc, "_vimb2_"));

    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/test-shortcut/get_uri/single", test_shortcut);
    g_test_add_func("/test-shortcut/get_uri/shell-param", test_shortcut_shell_param);
    g_test_add_func("/test-shortcut/remove", test_shortcut_remove);

    result = g_test_run();

    shortcut_free(sc);

    return result;
}
