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

#include <pwd.h>
#include <gtk/gtk.h>
#include <src/util.h>

static void check_expand(State state, const char *str, const char *expected)
{
    char *result = util_expand(state, str, UTIL_EXP_DOLLAR|UTIL_EXP_TILDE|UTIL_EXP_SPECIAL);
    g_assert_cmpstr(result, ==, expected);
    g_free(result);
}

static void test_expand_evn(void)
{
    State state = {0};
    /* set environment var for testing expansion */
    g_setenv("VIMB_VAR", "value", true);

    check_expand(state, "$VIMB_VAR", "value");
    check_expand(state, "$VIMB_VAR", "value");
    check_expand(state, "$VIMB_VAR$VIMB_VAR", "valuevalue");
    check_expand(state, "${VIMB_VAR}", "value");
    check_expand(state, "my$VIMB_VAR", "myvalue");
    check_expand(state, "'$VIMB_VAR'", "'value'");
    check_expand(state, "${VIMB_VAR}s ", "values ");

    g_unsetenv("UNKNOWN");

    check_expand(state, "$UNKNOWN", "");
    check_expand(state, "${UNKNOWN}", "");
    check_expand(state, "'$UNKNOWN'", "''");
}

static void test_expand_escaped(void)
{
    State state = {0};
    g_setenv("VIMB_VAR", "value", true);

    check_expand(state, "\\$VIMB_VAR", "$VIMB_VAR");
    check_expand(state, "\\${VIMB_VAR}", "${VIMB_VAR}");
    check_expand(state, "\\~/", "~/");
    check_expand(state, "\\~/vimb", "~/vimb");
    check_expand(state, "\\~root", "~root");
    check_expand(state, "\\%", "%");
    check_expand(state, "\\\\$VIMB_VAR", "\\value");         /* \\$VAR becomes \ExpandedVar */
    check_expand(state, "\\\\\\$VIMB_VAR", "\\$VIMB_VAR");   /* \\\$VAR becomes \$VAR */
}

static void test_expand_tilde_home(void)
{
    State state = {0};
    char *dir;
    const char *home = g_get_home_dir();

    check_expand(state, "~", "~");
    check_expand(state, "~/", home);
    check_expand(state, "foo~/bar", "foo~/bar");
    check_expand(state, "~/foo", (dir = g_strdup_printf("%s/foo", home)));
    g_free(dir);

    check_expand(state, "foo ~/bar", (dir = g_strdup_printf("foo %s/bar", home)));
    g_free(dir);

    check_expand(state, "~/~", (dir = g_strdup_printf("%s/~", home)));
    g_free(dir);

    check_expand(state, "~/~/", (dir = g_strdup_printf("%s/~/", home)));
    g_free(dir);
}

static void test_expand_tilde_user(void)
{
    State state = {0};
    const char *user = g_get_user_name();
    const char *home;
    char *in, *out;
    struct passwd *pwd;

    /* initialize home */
    pwd = getpwnam(user);
    g_assert_nonnull(pwd);
    home = pwd->pw_dir;

    /* don't expand within words */
    in = g_strdup_printf("foo~%s/bar", user);
    check_expand(state, in, in);
    g_free(in);

    check_expand(state, (in = g_strdup_printf("foo ~%s", user)), (out = g_strdup_printf("foo %s", home)));
    g_free(in);
    g_free(out);

    check_expand(state, (in = g_strdup_printf("~%s", user)), home);
    g_free(in);

    check_expand(state, (in = g_strdup_printf("~%s/bar", user)), (out = g_strdup_printf("%s/bar", home)));
    g_free(in);
    g_free(out);
}

static void test_expand_speacial(void)
{
    State state = {.uri = "http://fanglingsu.github.io/vimb/"};

    check_expand(state, "%", "http://fanglingsu.github.io/vimb/");
    check_expand(state, "'%'", "'http://fanglingsu.github.io/vimb/'");
}

static void test_strcasestr(void)
{
    g_assert_nonnull(util_strcasestr("Vim like Browser", "browser"));
    g_assert_nonnull(util_strcasestr("Vim like Browser", "vim LIKE"));
}

static void test_str_replace(void)
{
    char *value;

    value = util_str_replace("a", "uu", "foo bar baz");
    g_assert_cmpstr(value, ==, "foo buur buuz");
    g_free(value);

    value = util_str_replace("$1", "placeholder", "string with $1");
    g_assert_cmpstr(value, ==, "string with placeholder");
    g_free(value);
}

static void test_wildmatch_simple(void)
{
    g_assert_true(util_wildmatch("", ""));
    g_assert_true(util_wildmatch("w", "w"));
    g_assert_true(util_wildmatch(".", "."));
    g_assert_true(util_wildmatch("~", "~"));
    g_assert_true(util_wildmatch("wildmatch", "WildMatch"));
    g_assert_true(util_wildmatch("wild\\match", "wild\\match"));

    /* no special meaning of . and ~ in pattern */
    g_assert_false(util_wildmatch(".", "w"));
    g_assert_false(util_wildmatch("~", "w"));
    g_assert_false(util_wildmatch("wild", "wild "));
    g_assert_false(util_wildmatch("wild", " wild"));
    g_assert_false(util_wildmatch("wild", "\\ wild"));
    g_assert_false(util_wildmatch("wild", "\\wild"));
    g_assert_false(util_wildmatch("wild", "wild\\"));
    g_assert_false(util_wildmatch("wild\\1", "wild\\2"));
}

static void test_wildmatch_questionmark(void)
{
    g_assert_true(util_wildmatch("wild?atch", "wildmatch"));
    g_assert_true(util_wildmatch("wild?atch", "wildBatch"));
    g_assert_true(util_wildmatch("wild?atch", "wild?atch"));
    g_assert_true(util_wildmatch("?ild?atch", "MildBatch"));
    g_assert_true(util_wildmatch("foo\\?bar", "foo?bar"));
    g_assert_true(util_wildmatch("???", "foo"));
    g_assert_true(util_wildmatch("???", "bar"));

    g_assert_false(util_wildmatch("foo\\?bar", "foorbar"));
    g_assert_false(util_wildmatch("?", ""));
    g_assert_false(util_wildmatch("b??r", "bar"));
    /* ? does not match / in contrast to * which does */
    g_assert_false(util_wildmatch("user?share", "user/share"));
}

static void test_wildmatch_wildcard(void)
{
    g_assert_true(util_wildmatch("*", ""));
    g_assert_true(util_wildmatch("*", "Match as much as possible"));
    g_assert_true(util_wildmatch("*match", "prefix match"));
    g_assert_true(util_wildmatch("match*", "match suffix"));
    g_assert_true(util_wildmatch("match*", "match*"));
    g_assert_true(util_wildmatch("match\\*", "match*"));
    g_assert_true(util_wildmatch("match\\\\*", "match\\*"));
    g_assert_true(util_wildmatch("do * match", "do a infix match"));
    /* '*' matches also / in contrast to other implementations */
    g_assert_true(util_wildmatch("start*end", "start/something/end"));
    g_assert_true(util_wildmatch("*://*.io/*", "http://fanglingsu.github.io/vimb/"));
    /* multiple * should act like a single one */
    g_assert_true(util_wildmatch("**", ""));
    g_assert_true(util_wildmatch("match **", "Match as much as possible"));
    g_assert_true(util_wildmatch("f***u", "fu"));

    g_assert_false(util_wildmatch("match\\*", "match fail"));
    g_assert_false(util_wildmatch("f***u", "full"));
}

static void test_wildmatch_curlybraces(void)
{
    g_assert_true(util_wildmatch("{foo}", "foo"));
    g_assert_true(util_wildmatch("{foo,bar}", "foo"));
    g_assert_true(util_wildmatch("{foo,bar}", "bar"));
    g_assert_true(util_wildmatch("foo{lish,t}bar", "foolishbar"));
    g_assert_true(util_wildmatch("foo{lish,t}bar", "footbar"));
    /* esacped special chars */
    g_assert_true(util_wildmatch("foo\\{l\\}bar", "foo{l}bar"));
    g_assert_true(util_wildmatch("ba{r,z\\{\\}}", "bar"));
    g_assert_true(util_wildmatch("ba{r,z\\{\\}}", "baz{}"));
    g_assert_true(util_wildmatch("test{one\\,two,three}", "testone,two"));
    g_assert_true(util_wildmatch("test{one\\,two,three}", "testthree"));
    /* backslash before none special char is a normal char */
    g_assert_true(util_wildmatch("back{\\slash,}", "back\\slash"));
    g_assert_true(util_wildmatch("one\\two", "one\\two"));
    g_assert_true(util_wildmatch("\\}match", "}match"));
    g_assert_true(util_wildmatch("\\{", "{"));
    /* empty list parts */
    g_assert_true(util_wildmatch("{}", ""));
    g_assert_true(util_wildmatch("{,}", ""));
    g_assert_true(util_wildmatch("{,foo}", ""));
    g_assert_true(util_wildmatch("{,foo}", "foo"));
    g_assert_true(util_wildmatch("{bar,}", ""));
    g_assert_true(util_wildmatch("{bar,}", "bar"));
    /* no special meaning of ? and * in curly braces */
    g_assert_true(util_wildmatch("ab{*,cd}ef", "ab*ef"));
    g_assert_true(util_wildmatch("ab{d,?}ef", "ab?ef"));

    g_assert_false(util_wildmatch("{foo,bar}", "foo,bar"));
    g_assert_false(util_wildmatch("}match{ it", "}match{ anything"));
    /* don't match single parts that are seperated by escaped ',' */
    g_assert_false(util_wildmatch("{a,b\\,c,d}", "b"));
    g_assert_false(util_wildmatch("{a,b\\,c,d}", "c"));
    /* lonesome braces - this is a syntax error and will always be false */
    g_assert_false(util_wildmatch("}", "}"));
    g_assert_false(util_wildmatch("}", ""));
    g_assert_false(util_wildmatch("}suffix", "}suffux"));
    g_assert_false(util_wildmatch("}suffix", "suffux"));
    g_assert_false(util_wildmatch("{", "{"));
    g_assert_false(util_wildmatch("{", ""));
    g_assert_false(util_wildmatch("{foo", "{foo"));
    g_assert_false(util_wildmatch("{foo", "foo"));
    g_assert_false(util_wildmatch("foo{bar", "foo{bar"));
}

static void test_wildmatch_complete(void)
{
    g_assert_true(util_wildmatch("http{s,}://{fanglingsu.,}github.{io,com}/*vimb/", "http://fanglingsu.github.io/vimb/"));
    g_assert_true(util_wildmatch("http{s,}://{fanglingsu.,}github.{io,com}/*vimb/", "https://github.com/fanglingsu/vimb/"));
}

static void test_wildmatch_multi(void)
{
    g_assert_true(util_wildmatch("foo,?", "foo"));
    g_assert_true(util_wildmatch("foo,?", "f"));
    g_assert_true(util_wildmatch("foo,b{a,o,}r,ba?", "foo"));
    g_assert_true(util_wildmatch("foo,b{a,o,}r,ba?", "bar"));
    g_assert_true(util_wildmatch("foo,b{a,o,}r,ba?", "bor"));
    g_assert_true(util_wildmatch("foo,b{a,o,}r,ba?", "br"));
    g_assert_true(util_wildmatch("foo,b{a,o,}r,ba?", "baz"));
    g_assert_true(util_wildmatch("foo,b{a,o,}r,ba?", "bat"));

    g_assert_false(util_wildmatch("foo,b{a,o,}r,ba?", "foo,"));
    g_assert_false(util_wildmatch("foo,?", "fo"));
}

static void test_strescape(void)
{
    unsigned int i;
    char *value;
    struct {
        char *in;
        char *expected;
        char *excludes;
    } data[] = {
        {"", "", NULL},
        {"foo", "foo", NULL},
        {"a\nb\nc", "a\\nb\\nc", NULL},
        {"foo\"bar", "foo\\bar", NULL},
        {"s\\t", "s\\\\t", NULL},
        {"u\bv", "u\\bv", NULL},
        {"w\fx", "w\\fx", NULL},
        {"y\rz", "y\\rz", NULL},
        {"tab\tdi\t", "tab\\tdi\\t", NULL},
        {"❧äüö\n@foo\t\"bar\"", "❧äüö\\n@foo\\t\\\"bar\\\"", NULL},
        {"❧äüö\n@foo\t\"bar\"", "❧äüö\\n@foo\\t\"bar\"", "\""},
        {"❧äüö\n@foo\t\"bar\"", "❧äüö\n@foo\t\\\"bar\\\"", "\n\t"},
    };
    for (i = 0; i < LENGTH(data); i++) {
        value = util_strescape(data->in, data->excludes);
        g_assert_cmpstr(value, ==, data->expected);
        g_free(value);
    }
}

static void test_string_to_timespan(void)
{
    g_assert_cmpuint(util_string_to_timespan("d"), ==, G_TIME_SPAN_DAY);
    g_assert_cmpuint(util_string_to_timespan("h"), ==, G_TIME_SPAN_HOUR);
    g_assert_cmpuint(util_string_to_timespan("m"), ==, G_TIME_SPAN_MINUTE);
    g_assert_cmpuint(util_string_to_timespan("s"), ==, G_TIME_SPAN_SECOND);

    g_assert_cmpuint(util_string_to_timespan("y"), ==, G_TIME_SPAN_DAY * 365);
    g_assert_cmpuint(util_string_to_timespan("w"), ==, G_TIME_SPAN_DAY * 7);

    /* use counters */
    g_assert_cmpuint(util_string_to_timespan("1s"), ==, G_TIME_SPAN_SECOND);
    g_assert_cmpuint(util_string_to_timespan("2s"), ==, 2 * G_TIME_SPAN_SECOND);
    g_assert_cmpuint(util_string_to_timespan("34s"), ==, 34 * G_TIME_SPAN_SECOND);
    g_assert_cmpuint(util_string_to_timespan("0s"), ==, 0);

    /* combine counters and different units */
    g_assert_cmpuint(util_string_to_timespan("ds"), ==, G_TIME_SPAN_DAY + G_TIME_SPAN_SECOND);
    g_assert_cmpuint(util_string_to_timespan("2dh0s"), ==, (2 * G_TIME_SPAN_DAY) + G_TIME_SPAN_HOUR);

    /* unparsabel values */
    g_assert_cmpuint(util_string_to_timespan(""), ==, 0);
    g_assert_cmpuint(util_string_to_timespan("-"), ==, 0);
    g_assert_cmpuint(util_string_to_timespan("5-"), ==, 0);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/test-util/expand-env", test_expand_evn);
    g_test_add_func("/test-util/expand-escaped", test_expand_escaped);
    g_test_add_func("/test-util/expand-tilde-home", test_expand_tilde_home);
    g_test_add_func("/test-util/expand-tilde-user", test_expand_tilde_user);
    g_test_add_func("/test-util/expand-spacial", test_expand_speacial);
    g_test_add_func("/test-util/strcasestr", test_strcasestr);
    g_test_add_func("/test-util/str_replace", test_str_replace);
    g_test_add_func("/test-util/wildmatch-simple", test_wildmatch_simple);
    g_test_add_func("/test-util/wildmatch-questionmark", test_wildmatch_questionmark);
    g_test_add_func("/test-util/wildmatch-wildcard", test_wildmatch_wildcard);
    g_test_add_func("/test-util/wildmatch-curlybraces", test_wildmatch_curlybraces);
    g_test_add_func("/test-util/wildmatch-complete", test_wildmatch_complete);
    g_test_add_func("/test-util/wildmatch-multi", test_wildmatch_multi);
    g_test_add_func("/test-util/strescape", test_strescape);
    g_test_add_func("/test-util/string_to_timespan", test_string_to_timespan);

    return g_test_run();
}
