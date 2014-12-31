---
title:  Vimb - Frequently asked Questions
layout: default
meta:   FAQ
active: faq
---

# FAQ

## Does vimb provide formfiller?

No, vimb has no built in formfiller. But it's some kind of execise to set this
up by youself, by using the features of vimb.
Following snippets may help to get a working formfiller that fits into your
setup and your way to use a vimb.

1. Add a method that takes the form data and put it into the form element on
   current page. A sample [scripts.js][ffjs] that provides this functionality
   can be found in the sources of vimb.
2. Add a shell script into your `PATH` that takes an URI as argument and
   provides the formdata to be feed into the form fields. This needs vimb to
   be compiled with `FEATURE_SOCKET`. A sample [formfiller][ffsh] script, that
   uses gpg encrypted or unencrypted files is also available in the sources of
   vimb.
3. Make sure the commands to fill the form are not written to command history
   of vimb.
4. Start formfill. There are two ways to start the formfill, by hand on
   request via `:sh formfiller %` or by autocmd automatic on page load

       au LoadFinished https://github.com/login* sh! formfiller %


## Editor opens without the contents of textarea

There are two known cases of the `editor-command` setting that do not work.

1. If gvim or vim -g is used without the `-f` option. So to use gvim as editor
   you should use

       :set editor-command=gvim -f %s

2. If an editor ist started in `urxvtc` terminal as client for the `urxvtd`
   daemon. This does not work. So if you intend to use vim or another cli
   editor together with `urxvt` make sure you use `urxvt` and not `urxvtc`
   even if you still have the daemon running.

       :set editor-command=urxvt -e vim %s

## How can I have tabs?
{:#tabbed}

Vimb does not support tabs. Every new page is opened in a new browser instance
with own settings which makes things easier and secure. But vimb can be
plugged into another xembed aware window that allows tabbing like [tabbed][].

    tabbed -c vimb -e

To manage the tabs from within the vim like browser, you can use [xdotool][]
to run keyevents on tabbed and call the xdotool from within vimb.

Following keybindings simulate a little bit the vim behaviour.

    nnoremap gt :sh! xdotool key --window $VIMB_XID ctrl+shift+l<CR><Esc>
    nnoremap gT :sh! xdotool key --window $VIMB_XID ctrl+shift+h<CR><Esc>
    nnoremap 1gt :sh! xdotool key --window $VIMB_XID ctrl+1<CR><Esc>
    ...
    nnoremap 9gt :sh! xdotool key --window $VIMB_XID ctrl+9<CR><Esc>

## User-Scripts does not seem to have any effect
{:#user-scripts}

The precedence of the user style is lower than that of the website so you have
to mark your style definition to have higher priority.

    a:link {color: #0f0 !important;}

## How to change the colors of the hints?

Vimb hints can be styled by the user style-sheet
(`$XDG_CONFIG_HOME/vimb/style.css`)

This is the default style that is applied to the hints. The `_hintLabel` class
applies to the label with the hint numbers, the `_hintElem` class to the hinted
element (the link, input field or image) and the `_hintFocus` class is added to
the current focused element as well as the label. Following default style is
applied to the hints. To change already defined style it might be required to
use the `!importen` flag on your style definition to take effect
([see User-Scripts](#user-scripts)).

    ._hintLabel {
        -webkit-transform:translate(-4px,-4px);
        position: absolute;
        z-index: 100000;
        font:bold .8em monospace;
        color: #000;
        background-color: #fff;
        margin: 0;
        padding: 0px 1px;
        border: 1px solid #444;
        opacity: 0.7
    }
    ._hintElem {
        background-color: #ff0 !important;
        color: #000 !important
    }
    ._hintElem._hintFocus{
        background-color: #8f0 !important
    }
    ._hintLabel._hintFocus{
        z-index: 100001;
        opacity: 1
    }


[ffjs]:    https://raw.githubusercontent.com/fanglingsu/vimb/master/examples/formfiller/scripts.js
[ffsh]:    https://raw.githubusercontent.com/fanglingsu/vimb/master/examples/formfiller/formfiller
[tabbed]:  http://tools.suckless.org/tabbed/
[xdotool]: http://www.semicomplete.com/projects/xdotool/
