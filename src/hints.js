var VbHint = (function(){
    'use strict';

    var hClass      = "__hint",
        hClassFocus = "__hintFocus",
        hConts      = [],
        hints       = [],
        focusNum    = 1,
        config      = {};

    /* mode: l - links, i - images, e - editables */
    /* usage: O - open, T - open in new window, U - use source */
    function init(mode, usage, bg, bgf, fg, style, maxHints) {
        config      = {
            mode:     mode,
            usage:    usage,
            bg:       bg,
            bgf:      bgf,
            fg:       fg,
            style:    style,
            maxHints: maxHints
        };
    }

    function create(inputText) {
        var topwin     = window,
            top_height = topwin.innerHeight,
            top_width  = topwin.innerWidth,
            hCount     = 0;

        clear();

        function helper(win, offsetX, offsetY) {
            /* document may be undefined for frames out of the same origin */
            /* policy and will break the whole code - so we check this before */
            if (win.document === undefined) {
                return;
            }
            var doc      = win.document,
                fragment = doc.createDocumentFragment(),
                xpath    = getXpath(inputText),
                res      = doc.evaluate(
                    xpath, doc,
                    function (p) {return "http://www.w3.org/1999/xhtml";},
                    XPathResult.ORDERED_NODE_SNAPSHOT_TYPE, null
                ),

                /* generate basic hint element which will be cloned and updated later */
                hintSpan = doc.createElement("span"),
                /* Bounds */
                minX = offsetX < 0 ? -offsetX : 0,
                minY = offsetY < 0 ? -offsetY : 0,
                maxX = offsetX + win.width > top_width ? top_width - offsetX : top_width,
                maxY = offsetY + win.height > top_height ? top_height - offsetY : top_height,
                rect, e, i;

            hintSpan.setAttribute("class", hClass);
            hintSpan.style.cssText = config.style;

            /* due to the different XPath result type, we will need two counter variables */
            for (i = 0; i < res.snapshotLength; i++) {
                if (hCount >= config.maxHints) {
                    break;
                }

                e    = res.snapshotItem(i);
                rect = e.getBoundingClientRect();
                if (!rect || rect.left > maxX || rect.right < minX || rect.top > maxY || rect.bottom < minY) {
                    continue;
                }

                var cStyle = topwin.getComputedStyle(e, "");
                if (cStyle.display === "none" || cStyle.visibility !== "visible") {
                    continue;
                }

                /* making this block DOM compliant */
                var hint        = hintSpan.cloneNode(false);
                hint.style.left = Math.max((rect.left + win.scrollX), win.scrollX) - 3 + "px";
                hint.style.top  = Math.max((rect.top + win.scrollY), win.scrollY) - 3 + "px";
                hint.className  = hClass;
                hint.appendChild(doc.createTextNode(hCount + 1));

                fragment.appendChild(hint);

                hCount++;
                hints.push({
                    e:    e,
                    num:  hCount,
                    span: hint,
                    bg:   e.style.background,
                    fg:   e.style.color
                });

                /* change the foreground and background colors of the hinted items */
                e.style.color = config.fg;
                e.style.background = config.bg;
            }

            var hDiv = doc.createElement("div");
            hDiv.id  = "hint_container";

            hDiv.appendChild(fragment);
            doc.documentElement.appendChild(hDiv);

            hConts.push(hDiv);

            /* recurse into any iframe or frame element */
            var frameTags = ["frame","iframe"],
                frames, n, f, i;
            for (f = 0; f < frameTags.length; ++f) {
                frames = doc.getElementsByTagName(frameTags[f]);
                for (i = 0, n = frames.length; i < n; ++i) {
                    e    = frames[i];
                    rect = e.getBoundingClientRect();
                    if (!e.contentWindow || !rect || rect.left > maxX || rect.right < minX || rect.top > maxY || rect.bottom < minY) {
                        continue;
                    }
                    helper(e.contentWindow, offsetX + rect.left, offsetY + rect.top);
                }
            }
        }

        helper(topwin, 0, 0);

        if (hCount <= 1) {
            return fire(1);
        }
        return focusHint(1);
    }

    function focus(back) {
        var num,
            i = getHintId(focusNum);
        if (back) {
            num = (i !== 0 && hints[i - 1].num !== undefined)
                ? (hints[i - 1].num)
                : (hints[hints.length - 1].num);
        } else {
            num = (hints[i + 1] !== undefined)
                ? (hints[i + 1].num)
                : (hints[0].num);
        }
        return focusHint(num);
    }

    function update(n) {
        var remove = [], i, hint;
        if (n === 0) {
            return create();
        }
        /* remove none matching hints */
        for (i = 0; i < hints.length; ++i) {
            hint = hints[i];
            if (0 !== hint.num.toString().indexOf(n.toString())) {
                remove.push(hint.num);
            }
        }

        for (i = 0; i < remove.length; ++i) {
            removeHint(remove[i]);
        }

        if (hints.length === 1) {
            return fire(hints[0].num);
        }
        return focusHint(n);
    }

    function clear() {
        var i, hint;
        if (hints.length === 0) {
            return;
        }
        for (i = 0; i < hints.length; ++i) {
            hint = hints[i];
            if (hint.e) {
                hint.e.style.background = hint.bg;
                hint.e.style.color = hint.fg;
                hint.e.classList.remove(hClassFocus);
                hint.e.classList.remove(hClass);
                hint.span.parentNode.removeChild(hint.span);
            }
        }
        hints = [];
        for (i = 0; i < hConts.length; ++i) {
            hConts[i].parentNode.removeChild(hConts[i]);
        }
        hConts = [];
    }

    function fire(n) {
        var hint = getHint(n ? n : focusNum);
        if (!hint) {
            return "DONE:";
        }

        var e    = hint.e,
            tag  = e.nodeName.toLowerCase(),
            type = e.type ? e.type : "";

        clear();
        if (tag === "input" || tag === "textarea" || tag === "select") {
            if (type === "radio" || type === "checkbox") {
                e.focus();
                click(e);
                return "DONE:";
            }
            if (type === "submit" || type === "reset" || type  === "button" || type === "image") {
                click(e);
                return "DONE:";
            }
            e.focus();
            return "INSERT:";
        }
        if (tag === "iframe" || tag === "frame") {
            e.focus();
            return "DONE:";
        }

        switch (usage) {
            case "T": open(e, true); return "DONE:";
            case "O": open(e, false); return "DONE:";
            default: return "DATA:" + getSrc(e);
        }
    }

    /* fires the modeevent on hint with given number */
    function fire(n) {
        var hint = getHint(n ? n : focusNum);
        if (!hint) {
            return "DONE:";
        }

        var e    = hint.e,
            tag  = e.nodeName.toLowerCase(),
            type = e.type ? e.type : "";

        clear();

        if (tag === "input" || tag === "textarea" || tag === "select") {
            if (type === "radio" || type === "checkbox") {
                e.focus();
                click(e);
                return "DONE:";
            }
            if (type === "submit" || type === "reset" || type  === "button" || type === "image") {
                click(e);
                return "DONE:";
            }
            e.focus();
            return "INSERT:";
        } else if (tag === "iframe" || tag === "frame") {
            e.focus();
            return "DONE:";
        }

        switch (config.usage) {
            case "T": open(e, true); return "DONE:";
            case "O": open(e, false); return "DONE:";
            default: return "DATA:" + getSrc(e);
        }
    }

    /* internal used methods */
    function open(e, newWin)
    {
        var oldTarget = e.target;
        if (newWin) {
            /* set target to open in new window */
            e.target = "_blank";
        } else if (e.target === "_blank") {
            e.removeAttribute("target");
        }
        click(e);
        e.target = oldTarget;
    }

    /* set focus on hint with given number */
    function focusHint(n)
    {
        /* reset previous focused hint */
        var hint = getHint(focusNum);
        if (hint) {
            hint.e.classList.remove(hClassFocus);
            hint.e.style.background = config.bg;
            mouseEvent(hint.e, "mouseout");
        }

        focusNum = n;

        /* mark new hint as focused */
        hint = getHint(focusNum);
        if (hint) {
            hint.e.classList.add(hClassFocus);
            hint.e.style.background = config.bgf;
            var source              = getSrc(hint.e);
            mouseEvent(hint.e, "mouseover");

            return "OVER:" + (source ? source : "");
        }
    }

    /* retrieves the hint for given hint number */
    function getHint(n)
    {
        var i = getHintId(n);
        return i !== null ? hints[i] : null;
    }

    /* retrieves the id of hint with given number */
    function getHintId(n)
    {
        for (var i = 0; i < hints.length; ++i) {
            if (hints[i].num === n) {
                return i;
            }
        }
        return null;
    }

    /* removes hint with given number from hints array */
    function removeHint(n)
    {
        var i = getHintId(n);
        if (i === null) {
            return;
        }
        var hint = hints[i];
        hint.e.style.background = hint.bg;
        hint.e.style.color      = hint.fg;
        hint.span.parentNode.removeChild(hint.span);

        /* remove hints from all hints */
        hints.splice(i, 1);
    }

    function click(e)
    {
        mouseEvent(e, "mouseover");
        mouseEvent(e, "mousedown");
        mouseEvent(e, "mouseup");
        mouseEvent(e, "click");
    }

    function mouseEvent(e, name)
    {
        var evObj = e.ownerDocument.createEvent("MouseEvents");
        evObj.initMouseEvent(name, true, true, e.contentWindow, 0, 0, 0, 0, 0, false, false, false, false, 0, null);
        e.dispatchEvent(evObj);
    }

    /* retrieves the url of given element */
    function getSrc(e)
    {
        return e.href || e.src;
    }

    /* retrieves the xpath expression according to mode */
    function getXpath(s)
    {
        if (s === undefined) {
            s = "";
        }
        /* replace $WHAT in xpath to contains(translate(WHAT, 'SEARCH', 'search'), 'search') */
        function buildQuery(what, x, s)
        {
            var l, i, parts;
            l = s.toLowerCase();
            parts = [
                "contains(translate(",
                ",'"+s.toUpperCase()+"','"+l+"'),'"+l+"')"
            ];
            for (i = 0; i < what.length; ++i) {
                x = x.split("$" + what[i]).join(parts.join(what[i]));
            }
            return x;
        }

        switch (config.mode) {
            case "l":
                if (!s) {
                    return "//a[@href] | //*[@onclick or @tabindex or @class='lk' or @role='link' or @role='button'] | //input[not(@type='hidden' or @disabled or @readonly)] | //area[@href] | //textarea[not(@disabled or @readonly)] | //button | //select";
                }
                return buildQuery(
                    ["@value", ".", "@placeholder"],
                    "//a[@href and $.] | //*[(@onclick or @class='lk' or @role='link' or role='button') and $.] | //input[not(@type='hidden' or @disabled or @readonly) and ($@value or $@placeholder)] | //area[$.] | //textarea[not(@disabled or @readonly) and $.] | //button[$.] | //select[$.]",
                    s
                );
            case "e":
                if (!s) {
                    return "//input[@type='text'] | //textarea";
                }
                return buildQuery(
                    ["@value", ".", "@placeholder"],
                    "//input[@type='text' and ($@value or $@placeholder)] | //textarea[$.]",
                    s
                );
            case "i":
                if (!s) {
                    return "//img[@src]";
                }
                return buildQuery(["@title", "@alt"], "//img[@src and ($@title or $@alt)]", s);
        }
    }

    /* the api */
    return {
        init:   init,
        create: create,
        update: update,
        clear:  clear,
        fire:   fire,
        focus:  focus
    };
})();
Object.freeze(VbHint);
