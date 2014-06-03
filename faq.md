---
title:  Vimb - Frequently asked Questions
layout: default
meta:   FAQ
active: faq
---

# FAQ

## How can I have tabs?

Vimb does not support tabs. Every new page is opened in a new browser instance
with own settings which makes things easier and secure. But vimb can be
plugged into another xembed aware window that allows tabbing like [tabbed][].

    tabbed -c vimb -e

To manage the tabs from within the vim like browser, you can use [xdotool][]
to run keyevents on tabbed and call the xdotool from within vimb.

Following keybindings simulate a little bit the vim behaviour.

    nnoremap gt :sh! xdotool key --window \%@ ctrl+shift+l<CR><Esc>
    nnoremap gT :sh! xdotool key --window \%@ ctrl+shift+h<CR><Esc>
    nnoremap 1gt :sh! xdotool key --window \%@ ctrl+1<CR><Esc>
    ...
    nnoremap 9gt :sh! xdotool key --window \%@ ctrl+9<CR><Esc>

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


[tabbed]:  http://tools.suckless.org/tabbed/
[xdotool]: http://www.semicomplete.com/projects/xdotool/
