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

/**
 * This file contains function to handle input editing, parsing of called ex
 * commands from inputbox and the ex commands.
 */

#include <JavaScriptCore/JavaScript.h>
#include <string.h>
#include <sys/wait.h>

#include "ascii.h"
#include "bookmark.h"
#include "command.h"
#include "completion.h"
#include "config.h"
#include "ex.h"
#include "handler.h"
#include "hints.h"
#include "history.h"
#include "main.h"
#include "map.h"
#include "setting.h"
#include "shortcut.h"
#include "util.h"
#include "ext-proxy.h"
#include "autocmd.h"
#include "util.h"

typedef enum {
#ifdef FEATURE_AUTOCMD
    EX_AUTOCMD,
    EX_AUGROUP,
#endif
    EX_BMA,
    EX_BMR,
    EX_EVAL,
    EX_HARDCOPY,
    EX_CLEARDATA,
    EX_CMAP,
    EX_CNOREMAP,
    EX_HANDADD,
    EX_HANDREM,
    EX_IMAP,
    EX_NMAP,
    EX_NNOREMAP,
    EX_CUNMAP,
    EX_IUNMAP,
    EX_INOREMAP,
    EX_NUNMAP,
    EX_NORMAL,
    EX_OPEN,
#ifdef FEATURE_QUEUE
    EX_QCLEAR,
    EX_QPOP,
    EX_QPUSH,
    EX_QUNSHIFT,
#endif
    EX_QUIT,
    EX_REG,
    EX_SAVE,
    EX_SCA,
    EX_SCD,
    EX_SCR,
    EX_SET,
    EX_SHELLCMD,
    EX_SOURCE,
    EX_TABOPEN,
} ExCode;

typedef enum {
    PHASE_START,
    PHASE_REG,
} Phase;

typedef struct {
    int        count;    /* commands count */
    int        idx;      /* index in commands array */
    const char *name;    /* name of the command */
    ExCode     code;     /* id of the command */
    gboolean   bang;     /* if the command was called with a bang ! */
    GString    *lhs;     /* left hand side of the command - single word */
    GString    *rhs;     /* right hand side of the command - multiple words */
    int        flags;    /* flags for the already parsed command */
} ExArg;

typedef VbCmdResult (*ExFunc)(Client *c, const ExArg *arg);

typedef struct {
    const char *name;         /* full name of the command even if called abbreviated */
    ExCode    code;           /* constant id for the command */
    ExFunc    func;
#define EX_FLAG_NONE   0x000  /* no flags set */
#define EX_FLAG_BANG   0x001  /* command uses the bang ! after command name */
#define EX_FLAG_LHS    0x002  /* command has a single word after the command name */
#define EX_FLAG_RHS    0x004  /* command has a right hand side */
#define EX_FLAG_EXP    0x008  /* expand pattern like % or %:p in rhs */
#define EX_FLAG_CMD    0x010  /* like EX_FLAG_RHS but can contain | chars */
    int        flags;
} ExInfo;

static struct {
    char reg;    /* char for the yank register */
    Phase phase; /* current parsing phase */
} info = {'\0', PHASE_START};

static void input_activate(Client *c);
static gboolean parse(Client *c, const char **input, ExArg *arg, gboolean *nohist);
static gboolean parse_count(const char **input, ExArg *arg);
static gboolean parse_command_name(Client *c, const char **input, ExArg *arg);
static gboolean parse_bang(const char **input, ExArg *arg);
static gboolean parse_lhs(const char **input, ExArg *arg);
static gboolean parse_rhs(const char **input, ExArg *arg);
static void skip_whitespace(const char **input);
static void free_cmdarg(ExArg *arg);
static VbCmdResult execute(Client *c, const ExArg *arg);

#ifdef FEATURE_AUTOCMD
static VbCmdResult ex_augroup(Client *c, const ExArg *arg);
static VbCmdResult ex_autocmd(Client *c, const ExArg *arg);
#endif
static VbCmdResult ex_bookmark(Client *c, const ExArg *arg);
static VbCmdResult ex_eval(Client *c, const ExArg *arg);
static void on_eval_script_finished(GDBusProxy *proxy, GAsyncResult *result, Client *c);
static VbCmdResult ex_cleardata(Client *c, const ExArg *arg);
static VbCmdResult ex_hardcopy(Client *c, const ExArg *arg);
static void print_failed_cb(WebKitPrintOperation* op, GError *err, Client *c);
static VbCmdResult ex_map(Client *c, const ExArg *arg);
static VbCmdResult ex_unmap(Client *c, const ExArg *arg);
static VbCmdResult ex_normal(Client *c, const ExArg *arg);
static VbCmdResult ex_open(Client *c, const ExArg *arg);
#ifdef FEATURE_QUEUE
static VbCmdResult ex_queue(Client *c, const ExArg *arg);
#endif
static VbCmdResult ex_register(Client *c, const ExArg *arg);
static VbCmdResult ex_quit(Client *c, const ExArg *arg);
static VbCmdResult ex_save(Client *c, const ExArg *arg);
static VbCmdResult ex_set(Client *c, const ExArg *arg);
static VbCmdResult ex_shellcmd(Client *c, const ExArg *arg);
static VbCmdResult ex_shortcut(Client *c, const ExArg *arg);
static VbCmdResult ex_source(Client *c, const ExArg *arg);
static VbCmdResult ex_handlers(Client *c, const ExArg *arg);

static gboolean complete(Client *c, short direction);
static void completion_select(Client *c, char *match);
static gboolean history(Client *c, gboolean prev);
static void history_rewind(void);

/* The order of following command names is significant. If there exists
 * ambiguous commands matching to the users input, the first defined will be
 * the preferred match.
 * Also the sorting and grouping of command names matters, so we give up
 * searching for a matching command if the next compared character did not
 * match. */
static ExInfo commands[] = {
    /* command           code            func           flags */
#ifdef FEATURE_AUTOCMD
    {"autocmd",          EX_AUTOCMD,     ex_autocmd,    EX_FLAG_CMD|EX_FLAG_BANG},
    {"augroup",          EX_AUGROUP,     ex_augroup,    EX_FLAG_LHS|EX_FLAG_BANG},
#endif
    {"bma",              EX_BMA,         ex_bookmark,   EX_FLAG_RHS},
    {"bmr",              EX_BMR,         ex_bookmark,   EX_FLAG_RHS},
    {"cmap",             EX_CMAP,        ex_map,        EX_FLAG_LHS|EX_FLAG_CMD},
    {"cnoremap",         EX_CNOREMAP,    ex_map,        EX_FLAG_LHS|EX_FLAG_CMD},
    {"cunmap",           EX_CUNMAP,      ex_unmap,      EX_FLAG_LHS},
    {"cleardata",        EX_CLEARDATA,   ex_cleardata,  EX_FLAG_LHS|EX_FLAG_RHS},
    {"hardcopy",         EX_HARDCOPY,    ex_hardcopy,   EX_FLAG_NONE},
    {"handler-add",      EX_HANDADD,     ex_handlers,   EX_FLAG_RHS},
    {"handler-remove",   EX_HANDREM,     ex_handlers,   EX_FLAG_RHS},
    {"eval",             EX_EVAL,        ex_eval,       EX_FLAG_CMD|EX_FLAG_BANG},
    {"imap",             EX_IMAP,        ex_map,        EX_FLAG_LHS|EX_FLAG_CMD},
    {"inoremap",         EX_INOREMAP,    ex_map,        EX_FLAG_LHS|EX_FLAG_CMD},
    {"iunmap",           EX_IUNMAP,      ex_unmap,      EX_FLAG_LHS},
    {"nmap",             EX_NMAP,        ex_map,        EX_FLAG_LHS|EX_FLAG_CMD},
    {"nnoremap",         EX_NNOREMAP,    ex_map,        EX_FLAG_LHS|EX_FLAG_CMD},
    {"normal",           EX_NORMAL,      ex_normal,     EX_FLAG_BANG|EX_FLAG_CMD},
    {"nunmap",           EX_NUNMAP,      ex_unmap,      EX_FLAG_LHS},
    {"open",             EX_OPEN,        ex_open,       EX_FLAG_CMD},
    {"quit",             EX_QUIT,        ex_quit,       EX_FLAG_NONE|EX_FLAG_BANG},
#ifdef FEATURE_QUEUE
    {"qunshift",         EX_QUNSHIFT,    ex_queue,      EX_FLAG_RHS},
    {"qclear",           EX_QCLEAR,      ex_queue,      EX_FLAG_RHS},
    {"qpop",             EX_QPOP,        ex_queue,      EX_FLAG_NONE},
    {"qpush",            EX_QPUSH,       ex_queue,      EX_FLAG_RHS},
#endif
    {"register",         EX_REG,         ex_register,   EX_FLAG_NONE},
    {"save",             EX_SAVE,        ex_save,       EX_FLAG_RHS|EX_FLAG_EXP},
    {"set",              EX_SET,         ex_set,        EX_FLAG_RHS},
    {"shellcmd",         EX_SHELLCMD,    ex_shellcmd,   EX_FLAG_CMD|EX_FLAG_EXP|EX_FLAG_BANG},
    {"shortcut-add",     EX_SCA,         ex_shortcut,   EX_FLAG_RHS},
    {"shortcut-default", EX_SCD,         ex_shortcut,   EX_FLAG_RHS},
    {"shortcut-remove",  EX_SCR,         ex_shortcut,   EX_FLAG_RHS},
    {"source",           EX_SOURCE,      ex_source,     EX_FLAG_RHS|EX_FLAG_EXP},
    {"tabopen",          EX_TABOPEN,     ex_open,       EX_FLAG_CMD},
};

static struct {
    guint count;
    char  *prefix;  /* completion prefix like :, ? and / */
    char  *current; /* holds the current written input box content */
    char  *token;   /* initial filter content */
} excomp;

static struct {
    char  *prefix;  /* prefix that is prepended to the history item to for the complete command */
    char  *query;   /* part of input text to match the history items */
    GList *active;
} exhist;

extern struct Vimb vb;

/**
 * Function called when vimb enters the command mode.
 */
void ex_enter(Client *c)
{
    gtk_widget_grab_focus(GTK_WIDGET(c->input));
#if 0
    dom_clear_focus(c->webview);
#endif
}

/**
 * Called when the command mode is left.
 */
void ex_leave(Client *c)
{
    completion_clean(c);
    hints_clear(c);
    if (c->config.incsearch) {
        command_search(c, &((Arg){0, NULL}), FALSE);
    }
}

/**
 * Handles the keypress events from webview and inputbox.
 */
VbResult ex_keypress(Client *c, int key)
{
    GtkTextIter start, end;
    gboolean check_empty  = FALSE;
    GtkTextBuffer *buffer = c->buffer;
    GtkTextMark *mark;
    VbResult res;
    const char *text;

    if (key == CTRL('C')) {
        vb_enter(c, 'n');
        return RESULT_COMPLETE;
    }

    /* delegate call to hint mode if this is active */
    if (c->mode->flags & FLAG_HINTING
        && RESULT_COMPLETE == hints_keypress(c, key)) {

        return RESULT_COMPLETE;
    }

    /* process the register */
    if (info.phase == PHASE_REG) {
        info.reg   = (char)key;
        info.phase = PHASE_REG;

        /* insert the register text at cursor position */
        text = vb_register_get(c, (char)key);
        if (text) {
            gtk_text_buffer_insert_at_cursor(buffer, text, strlen(text));
        }

        res = RESULT_COMPLETE;
    } else {
        res = RESULT_COMPLETE;
        switch (key) {
            case KEY_TAB:
                complete(c, 1);
                break;

            case KEY_SHIFT_TAB:
                complete(c, -1);
                break;

            case KEY_UP: /* fall through */
            case CTRL('P'):
                history(c, TRUE);
                break;

            case KEY_DOWN: /* fall through */
            case CTRL('N'):
                history(c, FALSE);
                break;

            case KEY_CR:
                input_activate(c);
                break;

            case CTRL('['):
                vb_enter(c, 'n');
                vb_input_set_text(c, "");
                break;

            /* basic command line editing */
            case CTRL('H'):
                /* delete the last char before the cursor */
                mark = gtk_text_buffer_get_insert(buffer);
                gtk_text_buffer_get_iter_at_mark(buffer, &start, mark);
                gtk_text_buffer_backspace(buffer, &start, TRUE, TRUE);
                check_empty = TRUE;
                break;

            case CTRL('W'):
                /* delete word backward from cursor */
                mark = gtk_text_buffer_get_insert(buffer);
                gtk_text_buffer_get_iter_at_mark(buffer, &end, mark);

                /* copy the iter to build start and end point for deletion */
                start = end;

                /* move the iterator to the beginning of previous word */
                if (gtk_text_iter_backward_word_start(&start)) {
                    gtk_text_buffer_delete(buffer, &start, &end);
                }
                check_empty = TRUE;
                break;

            case CTRL('B'):
                /* move the cursor direct behind the prompt */
                gtk_text_buffer_get_iter_at_offset(buffer, &start, strlen(c->state.prompt));
                gtk_text_buffer_place_cursor(buffer, &start);
                break;

            case CTRL('E'):
                /* move the cursor to the end of line */
                gtk_text_buffer_get_end_iter(buffer, &start);
                gtk_text_buffer_place_cursor(buffer, &start);
                break;

            case CTRL('U'):
                /* remove everything between cursor and prompt */
                mark = gtk_text_buffer_get_insert(buffer);
                gtk_text_buffer_get_iter_at_mark(buffer, &end, mark);
                gtk_text_buffer_get_iter_at_offset(buffer, &start, strlen(c->state.prompt));
                gtk_text_buffer_delete(buffer, &start, &end);
                break;

            case CTRL('R'):
                info.phase      = PHASE_REG;
                c->mode->flags |= FLAG_NOMAP;
                res             = RESULT_MORE;
                break;

            default:
                /* if is printable ascii char, than write it at the cursor
                 * position into input box */
                if (key >= 0x20 && key <= 0x7e) {
                    gtk_text_buffer_insert_at_cursor(buffer, (char[2]){key, 0}, 1);
                } else {
                    c->state.processed_key = FALSE;
                }
        }
    }

    /* if the user deleted some content of the inputbox we check if the
     * inputbox is empty - if so we switch back to normal like vim does */
    if (check_empty) {
        gtk_text_buffer_get_bounds(buffer, &start, &end);
        if (gtk_text_iter_equal(&start, &end)) {
            vb_enter(c, 'n');
            vb_input_set_text(c, "");
        }
    }

    if (res == RESULT_COMPLETE) {
        info.reg   = 0;
        info.phase = PHASE_START;
    }

    return res;
}

/**
 * Handles changes in the inputbox.
 */
void ex_input_changed(Client *c, const char *text)
{
    GtkTextIter start, end;
    GtkTextBuffer *buffer = c->buffer;

    /* don't add line breaks if content is pasted from clipboard into inputbox */
    if (gtk_text_buffer_get_line_count(buffer) > 1) {
        /* remove everything from the buffer, except of the first line */
        gtk_text_buffer_get_iter_at_line(buffer, &start, 0);
        if (gtk_text_iter_forward_to_line_end(&start)) {
            gtk_text_buffer_get_end_iter(buffer, &end);

            /* TODO the following line creates a GTK warning.
             * ex_input_changed() is called from the "changed" event handler of
             * GtkTextBuffer. Apparently it's not supported to change a text
             * buffer in the changed handler!?
             *
             * Gtk-WARNING **: Invalid text buffer iterator: either the
             * iterator is uninitialized, or the characters/pixbufs/widgets in
             * the buffer have been modified since the iterator was created.
             */
            gtk_text_buffer_delete(buffer, &start, &end);
        }
    }

    switch (*text) {
        case ';': /* fall through - the modes are handled by hints_create */
        case 'g':
            hints_create(c, text);
            break;
        case '/': /* fall through */
        case '?':
            if (c->config.incsearch) {
                command_search(c, &((Arg){*text == '/' ? 1 : -1, (char*)text + 1}), FALSE);
            }
            break;
    }
}

gboolean ex_fill_completion(GtkListStore *store, const char *input)
{
    GtkTreeIter iter;
    ExInfo *cmd;
    gboolean found = FALSE;

    if (!input || *input == '\0') {
        for (int i = 0; i < LENGTH(commands); i++) {
            cmd = &commands[i];
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, COMPLETION_STORE_FIRST, cmd->name, -1);
            found = TRUE;
        }
    } else {
        for (int i = 0; i < LENGTH(commands); i++) {
            cmd = &commands[i];
            if (g_str_has_prefix(cmd->name, input)) {
                gtk_list_store_append(store, &iter);
                gtk_list_store_set(store, &iter, COMPLETION_STORE_FIRST, cmd->name, -1);
                found = TRUE;
            }
        }
    }

    return found;
}

/**
 * Run all ex commands from a file.
 */
VbCmdResult ex_run_file(Client *c, const char *filename)
{
    int length, i;
    char *line, **lines;
    VbCmdResult res = CMD_SUCCESS;

    lines = util_get_lines(filename);
    if (!lines) {
        return res;
    }

    length = g_strv_length(lines);
    for (i = 0; i < length; i++) {
        line = lines[i];
        /* skip commented or empty lines */
        if (*line == '#' || !*line) {
            continue;
        }
        if ((ex_run_string(c, line, FALSE) & ~CMD_KEEPINPUT) == CMD_ERROR) {
            res = CMD_ERROR | CMD_KEEPINPUT;
            g_warning("Invalid command in %s line #%u : '%s'", filename, i+1, line);
        }
    }
    g_strfreev(lines);

    return res;
}

VbCmdResult ex_run_string(Client *c, const char *input, gboolean enable_history)
{
    /* copy to have original command for history */
    const char *in  = input;
    gboolean nohist = FALSE;
    VbCmdResult res = CMD_ERROR | CMD_KEEPINPUT;
    ExArg *arg = g_slice_new0(ExArg);
    arg->lhs   = g_string_new("");
    arg->rhs   = g_string_new("");

    while (in && *in) {
        if (!parse(c, &in, arg, &nohist) || !(res = execute(c, arg))) {
            break;
        }
    }

    if (enable_history && !nohist) {
        history_add(c, HISTORY_COMMAND, input, NULL);
        vb_register_add(c, ':', input);
    }

    free_cmdarg(arg);

    return res;
}

/**
 * This is called if the user typed <nl> or <cr> into the inputbox.
 */
static void input_activate(Client *c)
{
    int count = -1;
    char *text, *cmd;
    VbCmdResult res;

    text = vb_input_get_text(c);

    /* skip leading prompt char like ':' or '/' */
    cmd = text + 1;
    switch (*text) {
        case '/': count = 1; /* fall through */
        case '?':
            vb_enter(c, 'n');

            command_search(c, &((Arg){count, strlen(cmd) ? cmd : NULL}), TRUE);
            break;

        case ';': /* fall through */
        case 'g':
            /* TODO fire hints */
            break;

        case ':':
            vb_enter(c, 'n');
            res = ex_run_string(c, cmd, TRUE);
            if (!(res & CMD_KEEPINPUT)) {
                /* clear input on success if this is not explicit ommited */
                vb_input_set_text(c, "");
            }
            break;

    }
    g_free(text);
}

/**
 * Parses given input string into given ExArg pointer.
 */
static gboolean parse(Client *c, const char **input, ExArg *arg, gboolean *nohist)
{
    if (!*input || !**input) {
        return FALSE;
    }

    /* truncate string from potentially previous run */
    g_string_truncate(arg->lhs, 0);
    g_string_truncate(arg->rhs, 0);

    /* remove leading whitespace and : */
    while (**input && (**input == ':' || VB_IS_SPACE(**input))) {
        (*input)++;
        /* Command started with additional ':' or whitespce - don't record it
         * in history or registry. */
        *nohist = TRUE;
    }
    parse_count(input, arg);

    skip_whitespace(input);
    if (!parse_command_name(c, input, arg)) {
        return FALSE;
    }

    /* parse the bang if this is allowed */
    if (arg->flags & EX_FLAG_BANG) {
        parse_bang(input, arg);
    }

    /* parse the lhs if this is available */
    skip_whitespace(input);
    if (arg->flags & EX_FLAG_LHS) {
        parse_lhs(input, arg);
    }
    /* parse the rhs if this is available */
    skip_whitespace(input);
    parse_rhs(input, arg);

    if (**input) {
        (*input)++;
    }

    return TRUE;
}

/**
 * Parses possible found count of given input into ExArg pointer.
 */
static gboolean parse_count(const char **input, ExArg *arg)
{
    if (!*input || !VB_IS_DIGIT(**input)) {
        arg->count = 0;
    } else {
        do {
            arg->count = arg->count * 10 + (**input - '0');
            (*input)++;
        } while (VB_IS_DIGIT(**input));
    }
    return TRUE;
}

/**
 * Parse the command name from given input.
 */
static gboolean parse_command_name(Client *c, const char **input, ExArg *arg)
{
    int len      = 0;
    int first    = 0;   /* number of first found command */
    int matches  = 0;   /* number of commands that matches the input */
    char cmd[20] = {0}; /* name of found command */

    do {
        /* copy the next char into the cmd buffer */
        cmd[len++] = **input;
        int i;
        for (i = first, matches = 0; i < LENGTH(commands); i++) {
            /* commands are grouped by their first letters, if we reached the
             * end of the group there are no more possible matches to find */
            if (len > 1 && strncmp(commands[i].name, cmd, len - 1)) {
                break;
            }
            if (commands[i].name[len - 1] == **input) {
                /* partial match found */
                if (!matches) {
                    /* if this is the first then remember it */
                    first = i;
                }
                matches++;
            }
        }
        (*input)++;
    } while (matches > 0 && **input && !VB_IS_SPACE(**input) && **input != '!');

    if (!matches) {
        /* read until next whitespace or end of input to get command name for
         * error message - vim uses the whole rest of the input string - but
         * the first word seems to bee enough for the error message */
        for (; len < (LENGTH(cmd) - 1) && *input && !VB_IS_SPACE(**input); (*input)++) {
            cmd[len++] = **input;
        }
        cmd[len] = '\0';

        vb_echo(c, MSG_ERROR, TRUE, "Unknown command: %s", cmd);
        return FALSE;
    }

    arg->idx   = first;
    arg->code  = commands[first].code;
    arg->name  = commands[first].name;
    arg->flags = commands[first].flags;

    return TRUE;
}

/**
 * Parse a single bang ! after the command.
 */
static gboolean parse_bang(const char **input, ExArg *arg)
{
    if (*input && **input == '!') {
        arg->bang = TRUE;
        (*input)++;
    }
    return TRUE;
}

/**
 * Parse a single word left hand side of a command arg.
 */
static gboolean parse_lhs(const char **input, ExArg *arg)
{
    char quote = '\\';

    if (!*input || !**input) {
        return FALSE;
    }

    /* get the char until the next none escaped whitespace and save it into
     * the lhs */
    while (**input && !VB_IS_SPACE(**input)) {
        /* if we find a backslash this escapes the next whitespace */
        if (**input == quote) {
            /* move pointer to the next char */
            (*input)++;
            if (!*input) {
                /* if input ends here - use only the backslash */
                g_string_append_c(arg->lhs, quote);
            } else if (**input == ' ') {
                /* escaped whitespace becomes only whitespace */
                g_string_append_c(arg->lhs, **input);
            } else {
                /* put escape char and next char into the result string */
                g_string_append_c(arg->lhs, quote);
                g_string_append_c(arg->lhs, **input);
            }
        } else {
            /* unquoted char */
            g_string_append_c(arg->lhs, **input);
        }
        (*input)++;
    }
    return TRUE;
}

/**
 * Parses the right hand side of command args. If flag EX_FLAG_CMD is set the
 * command can contain any char accept of the newline, else the right hand
 * side end on the first none escaped | or newline.
 */
static gboolean parse_rhs(const char **input, ExArg *arg)
{
    int expflags, flags;
    gboolean cmdlist;

    /* don't do anything if command has no right hand side or command list or
     * there is nothing to parse */
    if ((arg->flags & (EX_FLAG_RHS|EX_FLAG_CMD)) == 0
        || !*input || !**input
    ) {
        return FALSE;
    }

    cmdlist  = (arg->flags & EX_FLAG_CMD) != 0;
    expflags = (arg->flags & EX_FLAG_EXP)
        ? UTIL_EXP_TILDE|UTIL_EXP_DOLLAR
        : 0;
    flags = expflags;

    /* Get char until the end of command. Command ends on newline, and if
     * EX_FLAG_CMD is not set also on | */
    while (**input && **input != '\n' && (cmdlist || **input != '|')) {
        /* check for expansion placeholder */
        util_parse_expansion(input, arg->rhs, flags, "|\\");

        if (VB_IS_SEPARATOR(**input)) {
            /* add tilde expansion for next loop needs to be first char or to
             * be after a space */
            flags = expflags;
        } else {
            /* remove tile expansion for next loop */
            flags &= ~UTIL_EXP_TILDE;
        }
        (*input)++;
    }
    return TRUE;
}

/**
 * Executes the command given by ExArg.
 */
static VbCmdResult execute(Client *c, const ExArg *arg)
{
    return (commands[arg->idx].func)(c, arg);
}

static void skip_whitespace(const char **input)
{
    while (**input && VB_IS_SPACE(**input)) {
        (*input)++;
    }
}

static void free_cmdarg(ExArg *arg)
{
    if (arg->lhs) {
        g_string_free(arg->lhs, TRUE);
    }
    if (arg->rhs) {
        g_string_free(arg->rhs, TRUE);
    }
    g_slice_free(ExArg, arg);
}

#ifdef FEATURE_AUTOCMD
static VbCmdResult ex_augroup(Client *c, const ExArg *arg)
{
    return autocmd_augroup(c, arg->lhs->str, arg->bang) ? CMD_SUCCESS : CMD_ERROR;
}

static VbCmdResult ex_autocmd(Client *c, const ExArg *arg)
{
    return autocmd_add(c, arg->rhs->str, arg->bang) ? CMD_SUCCESS : CMD_ERROR;
}
#endif

static VbCmdResult ex_bookmark(Client *c, const ExArg *arg)
{
    if (arg->code == EX_BMR) {
        if (bookmark_remove(*arg->rhs->str ? arg->rhs->str : c->state.uri)) {
            vb_echo_force(c, MSG_NORMAL, TRUE, "  Bookmark removed");

            return CMD_SUCCESS | CMD_KEEPINPUT;
        }
    } else if (bookmark_add(c->state.uri, c->state.title, arg->rhs->str)) {
        vb_echo_force(c, MSG_NORMAL, TRUE, "  Bookmark added");

        return CMD_SUCCESS | CMD_KEEPINPUT;
    }

    return CMD_ERROR;
}

static VbCmdResult ex_eval(Client *c, const ExArg *arg)
{
    /* Called as :eval! - don't print to inputbox. */
    if (arg->bang) {
        ext_proxy_eval_script(c, arg->rhs->str, NULL);
    } else {
        ext_proxy_eval_script(c, arg->rhs->str, (GAsyncReadyCallback)on_eval_script_finished);
    }

    return CMD_SUCCESS;
}

static void on_eval_script_finished(GDBusProxy *proxy, GAsyncResult *result, Client *c)
{
    gboolean success = FALSE;
    char *string = NULL;

    GVariant *return_value = g_dbus_proxy_call_finish(proxy, result, NULL);
    if (return_value) {
        g_variant_get(return_value, "(bs)", &success, &string);
        if (success) {
            vb_echo(c, MSG_NORMAL, FALSE, "%s", string);
        } else {
            vb_echo(c, MSG_ERROR, TRUE, "%s", string);
        }
    } else {
        vb_echo(c, MSG_ERROR, TRUE, "");
    }
}

/**
 * Clear website data by ':cleardata {dataTypeList} {timespan}'.
 */
static VbCmdResult ex_cleardata(Client *c, const ExArg *arg)
{
    unsigned int data_types = 0;
    WebKitWebsiteDataManager *manager;
    VbCmdResult result = CMD_SUCCESS;
    GTimeSpan timespan = 0;

    /* Parse the left hand side if this is available and not '-' */
    if (arg->lhs->len && strcmp(arg->lhs->str, "-") != 0) {
        GString *str;
        char **types;
        char *value;
        unsigned int len, i, t;
        gboolean found;
        static const Arg type_map[] = {
            {WEBKIT_WEBSITE_DATA_MEMORY_CACHE,              "memory-cache"},
            {WEBKIT_WEBSITE_DATA_DISK_CACHE,                "disk-cache"},
            {WEBKIT_WEBSITE_DATA_OFFLINE_APPLICATION_CACHE, "offline-cache"},
            {WEBKIT_WEBSITE_DATA_SESSION_STORAGE,           "session-storage"},
            {WEBKIT_WEBSITE_DATA_LOCAL_STORAGE,             "local-storage"},
            {WEBKIT_WEBSITE_DATA_INDEXEDDB_DATABASES,       "indexeddb-databases"},
            {WEBKIT_WEBSITE_DATA_PLUGIN_DATA,               "plugin-data"},
            {WEBKIT_WEBSITE_DATA_COOKIES,                   "cookies"},
#if WEBKIT_CHECK_VERSION(2, 26, 0)
            {WEBKIT_WEBSITE_DATA_HSTS_CACHE,                "hsts-cache"},
#endif
            {0, ""},
        };

        /* Walk through list of data types given as rhs. */
        types = g_strsplit(arg->rhs->str, ",", -1);
        len   = g_strv_length(types);
        str   = g_string_new(NULL);

        /* Loop over given types and collect the type values into bitmap. */
        for (i = 0; i < len; i++) {
            value = types[i];
            found = FALSE;
            for (t = 0; type_map[t].i; t++) {
                if (!strcmp(value, type_map[t].s)) {
                    data_types |= type_map[t].i;
                    found       = TRUE;
                    break;
                }
            }

            if (!found) {
                /* collect unknown types to be shown in error message. */
                g_string_append_printf(str, "%s ", value);
            }
        }
        if (*str->str) {
            vb_echo(c, MSG_ERROR, TRUE, "unknown data type(s): %s", str->str);
            result = CMD_ERROR|CMD_KEEPINPUT;
        }
        g_string_free(str, TRUE);
        g_strfreev(types);
    } else {
        /* No special type or '-' given - clear all known types. */
        data_types = WEBKIT_WEBSITE_DATA_ALL;
    }

    if (arg->rhs->len) {
        timespan = util_string_to_timespan(arg->rhs->str);
    }
    manager = webkit_web_context_get_website_data_manager(webkit_web_view_get_context(c->webview));
    webkit_website_data_manager_clear(manager, data_types, timespan, NULL, NULL, NULL);

    return result;
}

/**
 * Opens the gtk print dialog.
 */
static VbCmdResult ex_hardcopy(Client *c, const ExArg *arg)
{
    WebKitPrintOperation *op   = webkit_print_operation_new(c->webview);
    GtkPrintSettings *settings = gtk_print_settings_new();
    gtk_print_settings_set(settings, GTK_PRINT_SETTINGS_OUTPUT_BASENAME, c->state.title);

    g_signal_connect(op, "failed", G_CALLBACK(print_failed_cb), c);

    webkit_print_operation_set_print_settings(op, settings);
    webkit_print_operation_run_dialog(op, NULL);
    g_object_unref(op);
    g_object_unref(settings);

    return CMD_SUCCESS;
}

/**
 * Callback called when printing failed.
 */
static void print_failed_cb(WebKitPrintOperation* op, GError *err, Client *c)
{
    vb_echo(c, MSG_ERROR, FALSE, "print failed: %s", err->message);
}

static VbCmdResult ex_map(Client *c, const ExArg *arg)
{
    if (!arg->lhs->len || !arg->rhs->len) {
        return CMD_ERROR;
    }

    /* instead of using the EX_XMAP constants we use the first char of the
     * command name as mode and the second to determine if noremap is used */
    map_insert(c, arg->lhs->str, arg->rhs->str, arg->name[0], arg->name[1] != 'n');

    return CMD_SUCCESS;
}

static VbCmdResult ex_unmap(Client *c, const ExArg *arg)
{
    char *lhs;
    if (!arg->lhs->len) {
        return CMD_ERROR;
    }

    lhs = arg->lhs->str;

    if (arg->code == EX_NUNMAP) {
        map_delete(c, lhs, 'n');
    } else if (arg->code == EX_CUNMAP) {
        map_delete(c, lhs, 'c');
    } else {
        map_delete(c, lhs, 'i');
    }
    return CMD_SUCCESS;
}

static VbCmdResult ex_normal(Client *c, const ExArg *arg)
{
    vb_enter(c, 'n');

    /* if called with bang - don't apply mapping */
    map_handle_string(c, arg->rhs->str, !arg->bang);

    return CMD_SUCCESS | CMD_KEEPINPUT;
}

static VbCmdResult ex_open(Client *c, const ExArg *arg)
{
    if (arg->code == EX_TABOPEN) {
        return vb_load_uri(c, &((Arg){TARGET_NEW, arg->rhs->str})) ? CMD_SUCCESS : CMD_ERROR;
    }
    return vb_load_uri(c, &((Arg){TARGET_CURRENT, arg->rhs->str})) ? CMD_SUCCESS :CMD_ERROR;
}

#ifdef FEATURE_QUEUE
static VbCmdResult ex_queue(Client *c, const ExArg *arg)
{
    Arg a = {0};

    switch (arg->code) {
        case EX_QPUSH:
            a.i = COMMAND_QUEUE_PUSH;
            break;

        case EX_QUNSHIFT:
            a.i = COMMAND_QUEUE_UNSHIFT;
            break;

        case EX_QPOP:
            a.i = COMMAND_QUEUE_POP;
            break;

        case EX_QCLEAR:
            a.i = COMMAND_QUEUE_CLEAR;
            break;

        default:
            return CMD_ERROR;
    }

    /* if no argument is found in rhs, keep the uri in arg null to force
     * command_queue() to use current URI */
    if (arg->rhs->len) {
        a.s = arg->rhs->str;
    }

    return command_queue(c, &a)
        ? CMD_SUCCESS | CMD_KEEPINPUT
        : CMD_ERROR;
}
#endif

/**
 * Show the contents of the registers :reg.
 */
static VbCmdResult ex_register(Client *c, const ExArg *arg)
{
    int idx;
    char *reg;
    const char *regchars = REG_CHARS;
    GString *str = g_string_new("-- Register --");

    for (idx = 0; idx < REG_SIZE; idx++) {
        /* show only filled registers */
        if (c->state.reg[idx]) {
            /* replace all newlines */
            reg = util_str_replace("\n", "^J", c->state.reg[idx]);
            g_string_append_printf(str, "\n\"%c   %s", regchars[idx], reg);
            g_free(reg);
        }
    }

    vb_echo(c, MSG_NORMAL, FALSE, "%s", str->str);
    g_string_free(str, TRUE);

    return CMD_SUCCESS | CMD_KEEPINPUT;
}

static VbCmdResult ex_quit(Client *c, const ExArg *arg)
{
    vb_quit(c, arg->bang);
    return CMD_SUCCESS;
}

static VbCmdResult ex_save(Client *c, const ExArg *arg)
{
    return command_save(c, &((Arg){COMMAND_SAVE_CURRENT, arg->rhs->str}))
        ? CMD_SUCCESS | CMD_KEEPINPUT
        : CMD_ERROR | CMD_KEEPINPUT;
}

static VbCmdResult ex_set(Client *c, const ExArg *arg)
{
    char *param = NULL;

    if (!arg->rhs->len) {
        return FALSE;
    }

    /* split the input string into parameter and value part */
    if ((param = strchr(arg->rhs->str, '='))) {
        *param++ = '\0';
        g_strstrip(arg->rhs->str);
        g_strstrip(param);

        return setting_run(c, arg->rhs->str, param);
    }

    return setting_run(c, arg->rhs->str, NULL);
}

static VbCmdResult ex_shellcmd(Client *c, const ExArg *arg)
{
    int status;
    char *stdOut = NULL, *stdErr = NULL, *selection = NULL;
    VbCmdResult res;
    GError *error = NULL;

    if (!*arg->rhs->str) {
        return CMD_ERROR;
    }

    /* Get current selection and write it as VIMB_SELECTION into env. */
    selection = ext_proxy_get_current_selection(c);
    if (selection) {
        g_setenv("VIMB_SELECTION", selection, TRUE);
        g_free(selection);
    } else {
        g_setenv("VIMB_SELECTION", "", TRUE);
    }

    if (arg->bang) {
        if (!g_spawn_command_line_async(arg->rhs->str, &error)) {
            g_warning("Can't run '%s': %s", arg->rhs->str, error->message);
            g_clear_error(&error);
            res = CMD_ERROR | CMD_KEEPINPUT;
        } else {
            res = CMD_SUCCESS;
        }
    } else {
        if (!g_spawn_command_line_sync(arg->rhs->str, &stdOut, &stdErr, &status, &error)) {
            g_warning("Can't run '%s': %s", arg->rhs->str, error->message);
            g_clear_error(&error);
            res = CMD_ERROR | CMD_KEEPINPUT;
        } else {
            /* the commands success depends not on the return code of the
             * called shell command, so we know the result already here */
            res = CMD_SUCCESS | CMD_KEEPINPUT;
        }

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            vb_echo(c, MSG_NORMAL, FALSE, "%s", stdOut);
        } else {
            vb_echo(c, MSG_ERROR, TRUE, "[%d] %s", WEXITSTATUS(status), stdErr);
        }
    }

    return res;
}

static VbCmdResult ex_handlers(Client *c, const ExArg *arg)
{
    char *p;
    gboolean res = FALSE;

    switch (arg->code) {
        case EX_HANDADD:
            if (arg->rhs->len && (p = strchr(arg->rhs->str, '='))) {
                *p++ = '\0';
                res = handler_add(c->handler, arg->rhs->str, p);
            }
            break;

        case EX_HANDREM:
            res = handler_remove(c->handler, arg->rhs->str);
            break;

        default:
            break;
    }

    return res ? CMD_SUCCESS : CMD_ERROR;
}

static VbCmdResult ex_shortcut(Client *c, const ExArg *arg)
{
    gchar *uri;
    gboolean success = FALSE;

    if (!c) {
        return CMD_ERROR;
    }

    if (!arg->rhs || !arg->rhs->str || !*arg->rhs->str) {
        return CMD_ERROR;
    }

    switch(arg->code) {
        case EX_SCA:
            if ((uri = strchr(arg->rhs->str, '='))) {
                *uri++ = '\0'; /* devide key and uri */
                g_strstrip(arg->rhs->str);
                g_strstrip(uri);
                success = shortcut_add(c->config.shortcuts, arg->rhs->str, uri);
            }
            break;

        case EX_SCR:
            g_strstrip(arg->rhs->str);
            success = shortcut_remove(c->config.shortcuts, arg->rhs->str);
            break;

        case EX_SCD:
            g_strstrip(arg->rhs->str);
            success = shortcut_set_default(c->config.shortcuts, arg->rhs->str);
            break;

        default:
            break;
    }

    return success ? CMD_SUCCESS : CMD_ERROR;
}

static VbCmdResult ex_source(Client *c, const ExArg *arg)
{
    return ex_run_file(c, arg->rhs->str);
}

/**
 * Manage the generation and stepping through completions.
 * This function prepared some prefix and suffix string that are required to
 * put the matched data back to inputbox, and prepares the tree list store
 * model containing matched values.
 */
static gboolean complete(Client *c, short direction)
{
    char *input;            /* input read from inputbox */
    const char *in;         /* pointer to input that we move */
    gboolean found = FALSE;
    gboolean sort  = TRUE;
    GtkListStore *store;

    input = vb_input_get_text(c);
    /* if completion was already started move to the next/prev item */
    if (c->mode->flags & FLAG_COMPLETION) {
        if (excomp.current && !strcmp(input, excomp.current)) {
            /* Step through the next/prev completion item. */
            if (!completion_next(c, direction < 0)) {
                /* If we stepped over the last/first item - put the initial content in */
                completion_select(c, excomp.token);
            }
            g_free(input);

            return TRUE;
        }

        /* if current input isn't the content of the completion item, stop
         * completion and start it after that again */
        completion_clean(c);
    }

    store = gtk_list_store_new(COMPLETION_STORE_NUM, G_TYPE_STRING, G_TYPE_STRING);

    in = (const char*)input;
    if (*in == ':') {
        const char *before_cmdname;
        /* skip leading ':' and whitespace */
        while (*in && (*in == ':' || VB_IS_SPACE(*in))) {
            in++;
        }

        ExArg *arg = g_slice_new0(ExArg);

        parse_count(&in, arg);

        /* Backup the current pointer so that we can restore the input pointer
         * if the command name parsing fails. */
        before_cmdname = in;

        /* Do ex command specific completion if the command is recognized and
         * there is a space after the command and the optional '!' bang. */
        if (parse_command_name(c, &in, arg) && parse_bang(&in, arg) && VB_IS_SPACE(*in)) {
            const char *token;
            /* Get only the last word of input string for the completion for
             * bookmark tag completion. */
            if (arg->code == EX_BMA) {
                /* Find the end of the input and search for the next
                 * whitespace toward the beginning. */
                token = strrchr(in, '\0');
                while (token >= in && !VB_IS_SPACE(*token)) {
                    token--;
                }
            } else {
                /* Use all input except of the command and it's possible bang
                 * itself for the completion. */
                token = in;
            }

            /* Save the string prefix that will not be part of completion like
             * the ':open ' if ':open something' is completed. This means that
             * the completion will only the none prefix part of the input */
            OVERWRITE_NSTRING(excomp.prefix, input, token - input + 1);
            OVERWRITE_STRING(excomp.token, token + 1);

            /* the token points to a space, skip this */
            skip_whitespace(&token);
            switch (arg->code) {
                case EX_OPEN:
                case EX_TABOPEN:
                    sort = FALSE;
                    if (*token == '!') {
                        found = bookmark_fill_completion(store, token + 1);
                    } else {
                        found = history_fill_completion(store, HISTORY_URL, token);
                    }
                    break;

                case EX_SET:
                    found = setting_fill_completion(c, store, token);
                    break;

                case EX_BMA:
                    found = bookmark_fill_tag_completion(store, token);
                    break;

                case EX_BMR:
                    sort  = FALSE;
                    found = bookmark_fill_completion(store, token);
                    break;

                case EX_SCR: /* Fallthrough */
                case EX_SCD:
                    found = shortcut_fill_completion(c->config.shortcuts, store, token);
                    break;

                case EX_HANDREM:
                    found = handler_fill_completion(c->handler, store, token);
                    break;

                case EX_SAVE: /* Fallthrough */
                case EX_SOURCE:
                    found = util_filename_fill_completion(store, token);
                    break;

#ifdef FEATURE_AUTOCMD
                case EX_AUTOCMD:
                    found = autocmd_fill_event_completion(c, store, token);
                    break;

                case EX_AUGROUP:
                    found = autocmd_fill_group_completion(c, store, token);
                    break;
#endif

                default:
                    break;
            }
        } else { /* complete command names */
            /* restore the 'in' pointer after try to parse command name */
            in = before_cmdname;
            OVERWRITE_STRING(excomp.token, in);

            /* Backup the parsed data so we can access them in
             * completion_select function. */
            excomp.count = arg->count;

            if (ex_fill_completion(store, in)) {
                /* Use all the input before the command as prefix. */
                OVERWRITE_NSTRING(excomp.prefix, input, in - input);
                found = TRUE;
            }
        }
        free_cmdarg(arg);
    } else if (*in == '/' || *in == '?') {
        if (history_fill_completion(store, HISTORY_SEARCH, in + 1)) {
            OVERWRITE_STRING(excomp.token, in + 1);
            OVERWRITE_NSTRING(excomp.prefix, in, 1);
            found = TRUE;
            sort  = FALSE;
        }
    }

    /* if the input could be parsed and the tree view could be filled */
    if (sort) {
        gtk_tree_sortable_set_sort_column_id(
            GTK_TREE_SORTABLE(store), COMPLETION_STORE_FIRST, GTK_SORT_ASCENDING
        );
    }

    if (found) {
        completion_create(c, GTK_TREE_MODEL(store), completion_select, direction < 0);
    }

    g_free(input);
    return TRUE;
}

/**
 * Callback called from the completion if a item is selected to write the
 * matched item according with previously saved prefix and command name to the
 * inputbox.
 */
static void completion_select(Client *c, char *match)
{
    OVERWRITE_STRING(excomp.current, NULL);

    if (excomp.count) {
        excomp.current = g_strdup_printf("%s%d%s", excomp.prefix, excomp.count, match);
    } else {
        excomp.current = g_strconcat(excomp.prefix, match, NULL);
    }
    vb_input_set_text(c, excomp.current);
}

static gboolean history(Client *c, gboolean prev)
{
    char *input;
    GList *new = NULL;

    input = vb_input_get_text(c);
    if (exhist.active) {
        /* calculate the actual content of the inpubox from history data, if
         * the theoretical content and the actual given input are different
         * rewind the history to recreate it later new */
        char *current = g_strconcat(exhist.prefix, (char*)exhist.active->data, NULL);
        if (strcmp(input, current)) {
            history_rewind();
        }
        g_free(current);
    }

    /* create the history list if the lookup is started or input was changed */
    if (!exhist.active) {
        int type;
        char prefix[2] = {0};
        const char *in = (const char*)input;

        skip_whitespace(&in);

        /* save the first char as prefix - this should be : / or ? */
        prefix[0] = *in;

        /* check which type of history we should use */
        if (*in == ':') {
            type = INPUT_COMMAND;
        } else if (*in == '/' || *in == '?') {
            /* the history does not distinguish between forward and backward
             * search, so we don't need the backward search here too */
            type = INPUT_SEARCH_FORWARD;
        } else {
            goto failed;
        }

        exhist.active = history_get_list(type, in + 1);
        if (!exhist.active) {
            goto failed;
        }
        OVERWRITE_STRING(exhist.prefix, prefix);
    }

    if (prev) {
        if ((new = g_list_next(exhist.active))) {
            exhist.active = new;
        }
    } else if ((new = g_list_previous(exhist.active))) {
        exhist.active = new;
    }

    if (!exhist.active) {
        goto failed;
    }

    vb_echo_force(c, MSG_NORMAL, FALSE, "%s%s", exhist.prefix, (char*)exhist.active->data);

    g_free(input);
    return TRUE;

failed:
    g_free(input);
    return FALSE;
}

static void history_rewind(void)
{
    if (exhist.active) {
        /* free temporary used history list */
        g_list_free_full(exhist.active, (GDestroyNotify)g_free);
        exhist.active = NULL;

        OVERWRITE_STRING(exhist.prefix, NULL);
    }
}
