---
title:  Vimb - user scripts
layout: default
meta:   Tipp, Tricks and scripts to make vimb even more productive
active: scripts
---

# user scripts

* toc
{:toc}

## formfiller
{:#formfiller}

Following snippets may help to get a working formfiller that fits into your
setup and your way to use a vimb.

1. Add a method that takes the form data and put it into the form element on
   current page. A sample [scripts.js][ffjs] that provides this functionality
   can be found in the sources of vimb.
2. Add a shell script into your `PATH` that takes an URI as argument and
   provides the form data to be feed into the form fields. This needs vimb to
   be compiled with `FEATURE_SOCKET` and started with `--socket` option. A
   sample [formfiller][ffsh] script, that uses gpg encrypted or unencrypted
   files is also available in the sources of vimb.

   Form data are save in the key file as JSON array with one or more items
   containing of selector for [document.querySelectorAll()][jsqsa] to find the form
   field, and a value to put into.

       ["input[name='user']:daniel", "input[name='password']:p45w0rD"]

3. Make sure the commands to fill the form are not written to command history
   of vimb.
4. Start formfill. There are two ways to start the formfill, by hand on
   request via `:sh formfiller %` or by autocmd automatic on page load

       au LoadFinished https://github.com/login* sh! formfiller %

## dark theme

This can be used as *style.css* file for vimb.

{% highlight css %}
*,div,pre,textarea,body,input,td,tr,p {
    background-color: #303030 !important;
    background-image: none !important;
    color: #bbbbbb !important;
}
h1,h2,h3,h4 {
    background-color: #303030 !important;
    color: #b8ddea !important;
}
a {
    color: #70e070 !important;
}
a:hover,a:focus {
    color: #7070e0 !important;
}
a:visited {
    color: #e07070 !important;
}
img {
    opacity: .5;
}
img:hover {
    opacity: 1;
}
._hintLabel {
    text-transform: uppercase !important;
    background-color: #e0e070 !important;
    font-size: .9em !important;
    color: #000 !important;
    opacity: 0.4;
}
._hintElem._hintFocus {
    color: #f0f070 !important;
}
._hintElem._hintFocus img {
    opacity: 1;
}
{% endhighlight %}

[ffjs]:     https://raw.githubusercontent.com/fanglingsu/vimb/master/examples/formfiller/scripts.js
[ffsh]:     https://raw.githubusercontent.com/fanglingsu/vimb/master/examples/formfiller/formfiller
[jsqsa]:    http://mdn.beonex.com/en/DOM/document.querySelectorAll.html
