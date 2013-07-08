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

## User-Scripts does not seem to have any effect

The precedance of the user style is lower than that of the website so you have
to mark your style definition to have higher priority.

    a:link {color: #0f0 !important;}

## How can I use Emacs keybinding for input editing?

If the key theme is installed, following woul do the job for GTK2.

    echo 'gtk-key-theme-name = "Emacs"' >> ~/.gtkrc-2.0

[tabbed]: http://tools.suckless.org/tabbed/
