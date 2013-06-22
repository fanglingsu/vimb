/* mode: l - links, i - images, e - editables */
/* usage: O - open, T - open in new window, U - use source */
function VimbHints(mode, usage, bg, bgf, fg, style, maxHints)
{
    "use strict";
    var hClass      = "__hint";
    var hClassFocus = "__hintFocus";
    var hConts      = [];
    var hints       = [];
    var focusNum    = 1;

    this.create = function(inputText)
    {
        var topwin     = window;
        var top_height = topwin.innerHeight;
        var top_width  = topwin.innerWidth;
        var hCount     = 0;

        this.clear();

        function _helper(win, offsetX, offsetY)
        {
            /* document may be undefined for frames out of the same origin */
            /* policy and will break the whole code - so we check this before */
            if (win.document === undefined) {
                return;
            }
            var doc = win.document;

            var fragment = doc.createDocumentFragment();
            var xpath    = _getXpath(inputText);

            var res = doc.evaluate(
                xpath, doc,
                function (p) {return "http://www.w3.org/1999/xhtml";},
                XPathResult.ORDERED_NODE_SNAPSHOT_TYPE, null
            );

            /* generate basic hint element which will be cloned and updated later */
            var hintSpan = doc.createElement("span");
            hintSpan.setAttribute("class", hClass);
            hintSpan.style.cssText = style;

            /* Bounds */
            var minX = offsetX < 0 ? -offsetX : 0;
            var minY = offsetY < 0 ? -offsetY : 0;
            var maxX = offsetX + win.width > top_width ? top_width - offsetX : top_width;
            var maxY = offsetY + win.height > top_height ? top_height - offsetY : top_height;

            /* due to the different XPath result type, we will need two counter variables */
            var rect, e;
            for (i = 0; i < res.snapshotLength; i++) {
                if (hCount >= maxHints) {
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
                    fg:   e.style.color}
                );

                /* change the foreground and background colors of the hinted items */
                e.style.color = fg;
                e.style.background = bg;
            }

            var hDiv = doc.createElement("div");
            hDiv.id  = "hint_container";

            hDiv.appendChild(fragment);
            doc.documentElement.appendChild(hDiv);

            hConts.push(hDiv);

            /* recurse into any iframe or frame element */
            var frameTags = ["frame","iframe"];
            for (var f = 0; f < frameTags.length; ++f) {
                var frames = doc.getElementsByTagName(frameTags[f]);
                for (var i = 0, nframes = frames.length; i < nframes; ++i) {
                    e = frames[i];
                    rect = e.getBoundingClientRect();
                    if (!e.contentWindow || !rect || rect.left > maxX || rect.right < minX || rect.top > maxY || rect.bottom < minY) {
                        continue;
                    }
                    _helper(e.contentWindow, offsetX + rect.left, offsetY + rect.top);
                }
            }
        }

        _helper(topwin, 0, 0);

        if (hCount <= 1) {
            return this.fire(1);
        }
        return _focus(1);
    };

    /* set focus to next avaiable hint */
    this.focusNext = function()
    {
        var i = _getHintIdByNum(focusNum);

        if (hints[i + 1] !== undefined) {
            return _focus(hints[i + 1].num);
        }
        return _focus(hints[0].num);
    };

    /* set focus to previous avaiable hint */
    this.focusPrev = function()
    {
        var i = _getHintIdByNum(focusNum);
        if (i !== 0 && hints[i - 1].num !== undefined) {
            return _focus(hints[i - 1].num);
        }
        return _focus(hints[hints.length - 1].num);
    };

    /* filters hints matching given number */
    this.update = function(n)
    {
        if (n === 0) {
            return this.create();
        }
        /* remove none matching hints */
        var remove = [];
        var i;
        for (i = 0; i < hints.length; ++i) {
            var hint = hints[i];
            if (0 !== hint.num.toString().indexOf(n.toString())) {
                remove.push(hint.num);
            }
        }

        for (i = 0; i < remove.length; ++i) {
            _removeHint(remove[i]);
        }

        if (hints.length === 1) {
            return this.fire(hints[0].num);
        }
        return _focus(n);
    };

    /* remove all hints and set previous style to them */
    this.clear = function()
    {
        if (hints.length === 0) {
            return;
        }
        for (var i = 0; i < hints.length; ++i) {
            var hint = hints[i];
            if (hint.e) {
                hint.e.style.background = hint.bg;
                hint.e.style.color = hint.fg;
                hint.e.classList.remove(hClassFocus);
                hint.e.classList.remove(hClass);
                hint.span.parentNode.removeChild(hint.span);
            }
        }
        hints = [];
        for (var i = 0; i < hConts.length; ++i) {
            hConts[i].parentNode.removeChild(hConts[i]);
        }
        hConts = [];
    };

    /* fires the modeevent on hint with given number */
    this.fire = function(n)
    {
        var hint = _getHintByNum(n ? n : focusNum);
        if (!hint) {
            return "DONE:";
        }

        var e    = hint.e;
        var tag  = e.nodeName.toLowerCase();
        var type = e.type ? e.type : "";

        this.clear();

        if (tag === "input" || tag === "textarea" || tag === "select") {
            if (type === "radio" || type === "checkbox") {
                e.focus();
                _click(e);
                return "DONE:";
            }
            if (type === "submit" || type === "reset" || type  === "button" || type === "image") {
                _click(e);
                return "DONE:";
            }
            e.focus();
            return "INSERT:";
        } else if (tag === "iframe" || tag === "frame") {
            e.focus();
            return "DONE:";
        }

        switch (usage) {
            case "T": _open(e, true); return "DONE:";
            case "O": _open(e, false); return "DONE:";
            default: return "DATA:" + _getSrc(e);
        }
    };

    /* opens given element */
    function _open(e, newWin)
    {
        var oldTarget = e.target;
        if (newWin) {
            /* set target to open in new window */
            e.target = "_blank";
        } else if (e.target === "_blank") {
            e.removeAttribute("target");
        }
        _click(e);
        e.target = oldTarget;
    }

    /* set focus on hint with given number */
    function _focus(n)
    {
        /* reset previous focused hint */
        var hint = _getHintByNum(focusNum);
        if (hint) {
            hint.e.classList.remove(hClassFocus);
            hint.e.style.background = bg;
            _mouseEvent(hint.e, "mouseout");
        }

        focusNum = n;

        /* mark new hint as focused */
        hint = _getHintByNum(focusNum);
        if (hint) {
            hint.e.classList.add(hClassFocus);
            hint.e.style.background = bgf;
            var source              = _getSrc(hint.e);
            _mouseEvent(hint.e, "mouseover");

            return "OVER:" + (source ? source : "");
        }
    }

    /* retrieves the hint for given hint number */
    function _getHintByNum(n)
    {
        var i = _getHintIdByNum(n);
        return i !== null ? hints[i] : null;
    }

    /* retrieves the id of hint with given number */
    function _getHintIdByNum(n)
    {
        for (var i = 0; i < hints.length; ++i) {
            if (hints[i].num === n) {
                return i;
            }
        }
        return null;
    }

    /* removes hint with given number from hints array */
    function _removeHint(n)
    {
        var i = _getHintIdByNum(n);
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

    function _click(e)
    {
        _mouseEvent(e, "mouseover");
        _mouseEvent(e, "mousedown");
        _mouseEvent(e, "mouseup");
        _mouseEvent(e, "click");
    }

    function _mouseEvent(e, name)
    {
        var evObj = e.ownerDocument.createEvent("MouseEvents");
        evObj.initMouseEvent(name, false, true, e.contentWindow, 0, 0, 0, 0, 0, false, false, false, false, 0, null);
        e.dispatchEvent(evObj);
    }

    /* retrieves the url of given element */
    function _getSrc(e)
    {
        return e.href || e.src;
    }

    /* retrieves the xpath expression according to mode */
    function _getXpath(s)
    {
        if (s === undefined) {
            s = "";
        }
        /* replace $WHAT in xpath to contains(translate(WHAT, 'SEARCH', 'search'), 'search') */
        function _buildQuery(what, x, s)
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

        switch (mode) {
            case "l":
                if (!s) {
                    return "//a[@href] | //*[@onclick or @tabindex or @class='lk' or @role='link' or @role='button'] | //input[not(@type='hidden' or @disabled or @readonly)] | //area[@href] | //textarea[not(@disabled or @readonly)] | //button | //select";
                }
                return _buildQuery(
                    ["@value", "."],
                    "//a[@href and $.] | //*[(@onclick or @class='lk' or @role='link' or role='button') and $.] | //input[not(@type='hidden' or @disabled or @readonly) and $@value] | //area[$.] | //textarea[not(@disabled or @readonly) and $.] | //button[$.] | //select[$.]",
                    s
                );
            case "e":
                if (!s) {
                    return "//input[@type='text'] | //textarea";
                }
                return _buildQuery(
                    ["@value", "."],
                    "//input[@type='text' and $@value] | //textarea[$.]",
                    s
                );
            case "i":
                if (!s) {
                    return "//img[@src]";
                }
                return _buildQuery(["@title", "@alt"], "//img[@src and ($@title or $@alt)]", s);
        }
    }
};
