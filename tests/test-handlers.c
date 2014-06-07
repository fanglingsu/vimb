/**
 * vimb - a webkit based vim like browser.
 *
 * Copyright (C) 2012-2014 Daniel Carl
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
#include <src/handlers.h>

#define TEST_URI "http://fanglingsu.github.io/vimb/"

static void test_handler_add(void)
{
    /* check none handled http first */
    g_assert_true(handler_add("http", "echo -n 'handled uri %s'"));
}

static void test_handler_remove(void)
{
    handler_add("http", "e");

    g_assert_true(handler_remove("http"));
    g_assert_false(handler_remove("http"));
}

static void test_handler_run_success(void)
{
    if (g_test_subprocess()) {
        handler_add("http", "echo -n 'handled uri %s'");
        handle_uri(TEST_URI);
        return;
    }
    g_test_trap_subprocess(NULL, 0, 0);
    g_test_trap_assert_passed();
    g_test_trap_assert_stdout("handled uri " TEST_URI);
}

static void test_handler_run_failed(void)
{
    if (g_test_subprocess()) {
        handler_add("http", "unknown-program %s");
        handle_uri(TEST_URI);
        return;
    }
    g_test_trap_subprocess(NULL, 0, 0);
    g_test_trap_assert_failed();
    g_test_trap_assert_stderr("*Can't run *unknown-program*");
}

int main(int argc, char *argv[])
{
    int result;
    handlers_init();

    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/test-handlers/add", test_handler_add);
    g_test_add_func("/test-handlers/remove", test_handler_remove);
    g_test_add_func("/test-handlers/handle_uri/success", test_handler_run_success);
    g_test_add_func("/test-handlers/handle_uri/failed", test_handler_run_failed);

    result = g_test_run();

    handlers_cleanup();

    return result;
}
