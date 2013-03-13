/* mode: l - links, i - images */
/* usage: O - open, T - open in new window, U - use source */
VimpHints = function Hints(mode, usage, bg, bgf, fg, style, maxHints) {
    "use strict";
    var hClass = "__hint";
    var hClassFocus = "__hint_container";
    var hCont;
    var curFocusNum = 1;
    var hints = [];

    this.create = function(inputText)
    {
        var topwin = window;
        var top_height = topwin.innerHeight;
        var top_width = topwin.innerWidth;
        var xpath_expr;

        var hCount = 0;
        this.clear();

        function _helper (win, offsetX, offsetY)
        {
            var doc = win.document;

            var win_height = win.height;
            var win_width = win.width;

            /* Bounds */
            var minX = offsetX < 0 ? -offsetX : 0;
            var minY = offsetY < 0 ? -offsetY : 0;
            var maxX = offsetX + win_width > top_width ? top_width - offsetX : top_width;
            var maxY = offsetY + win_height > top_height ? top_height - offsetY : top_height;

            var scrollX = win.scrollX;
            var scrollY = win.scrollY;

            var fragment = doc.createDocumentFragment();
            xpath_expr = _getXpath(inputText);

            var res = doc.evaluate(
                xpath_expr, doc,
                function (p) {return "http://www.w3.org/1999/xhtml";},
                XPathResult.ORDERED_NODE_SNAPSHOT_TYPE, null
            );

            /* generate basic hint element which will be cloned and updated later */
            var hintSpan = doc.createElement("span");
            hintSpan.setAttribute("class", hClass);
            hintSpan.style.cssText = style;

            /* due to the different XPath result type, we will need two counter variables */
            var rect, e, text, node, show_text;
            for (i = 0; i < res.snapshotLength; i++) {
                if (hCount >= maxHints) {
                    break;
                }

                e = res.snapshotItem(i);
                rect = e.getBoundingClientRect();
                if (!rect || rect.left > maxX || rect.right < minX || rect.top > maxY || rect.bottom < minY) {
                    continue;
                }

                var cStyle = topwin.getComputedStyle(e, "");
                if (cStyle.display === "none" || cStyle.visibility !== "visible") {
                    continue;
                }

                var leftpos = Math.max((rect.left + scrollX), scrollX);
                var toppos = Math.max((rect.top + scrollY), scrollY);

                /* making this block DOM compliant */
                var hint = hintSpan.cloneNode(false);
                hint.style.left = leftpos - 3 + "px";
                hint.style.top =  toppos - 3 + "px";
                text = doc.createTextNode(hCount + 1);
                hint.appendChild(text);

                fragment.appendChild(hint);
                hCount++;
                hints.push({
                    e:    e,
                    num:  hCount,
                    span: hint,
                    bg:   e.style.background,
                    fg:   e.style.color}
                );

                /* make the link black to ensure it's readable */
                e.style.color = fg;
                e.style.background = bg;
            }

            hCont = doc.createElement("div");
            hCont.id = "hint_container";

            hCont.appendChild(fragment);
            doc.documentElement.appendChild(hCont);

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
        var index = _getHintIdByNum(curFocusNum);

        if (typeof(hints[index + 1]) !== "undefined") {
            return _focus(hints[index + 1].num);
        }
        return _focus(hints[0].num);
    };

    /* set focus to previous avaiable hint */
    this.focusPrev = function()
    {
        var index = _getHintIdByNum(curFocusNum);
        if (index !== 0 && typeof(hints[index - 1].num) !== "undefined") {
            return _focus(hints[index - 1].num);
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
                hint.span.parentNode.removeChild(hint.span);
            }
        }
        hints = [];
        hCont.parentNode.removeChild(hCont);
        window.onkeyup = null;
    };

    /* fires the modeevent on hint with given number */
    this.fire = function(n)
    {
        n = n ? n : curFocusNum;
        var hint = _getHintByNum(n);
        if (!hint || typeof(hint.e) == "undefined") {
            return "DONE:";
        }

        var e  = hint.e;
        var tag = e.nodeName.toLowerCase();
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
        var hint = _getHintByNum(curFocusNum);
        if (hint !== null) {
            hint.e.className = hint.e.className.replace(hClassFocus, hClass);
            hint.e.style.background = bg;
            _mouseEvent(hint.e, "mouseout");
        }

        curFocusNum = n;

        /* mark new hint as focused */
        hint = _getHintByNum(curFocusNum);
        if (hint !== null) {
            hint.e.className = hint.e.className.replace(hClass, hClassFocus);
            hint.e.style.background = bgf;
            _mouseEvent(hint.e, "mouseover");
            var source = _getSrc(hint.e);

            return "OVER:" + (source ? source : "");
        }
    }

    /* retrieves the hint for given hint number */
    function _getHintByNum(n)
    {
        var index = _getHintIdByNum(n);
        if (index !== null) {
            return hints[index];
        }
        return null;
    }

    /* retrieves the id of hint with given number */
    function _getHintIdByNum(n)
    {
        var hint;
        for (var i = 0; i < hints.length; ++i) {
            hint = hints[i];
            if (hint.num === n) {
                return i;
            }
        }
        return null;
    }

    /* removes hint with given number from hints array */
    function _removeHint(n)
    {
        var index = _getHintIdByNum(n);
        if (index === null) {
            return;
        }
        var hint = hints[index];
        if (hint.num === n) {
            hint.e.style.background = hint.bg;
            hint.e.style.color = hint.fg;
            hint.span.parentNode.removeChild(hint.span);

            /* remove hints from all hints */
            hints.splice(index, 1);
        }
    }

    function _click(e) {
        _mouseEvent(e, "mouseover");
        _mouseEvent(e, "mousedown");
        _mouseEvent(e, "mouseup");
        _mouseEvent(e, "click");
    }

    function _mouseEvent(e, name)
    {
        var doc = e.ownerDocument;
        var view = e.contentWindow;

        var evObj = doc.createEvent("MouseEvents");
        evObj.initMouseEvent(name, true, true, view, 0, 0, 0, 0, 0, false, false, false, false, 0, null);
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
        var expr;
        if (typeof(s) === "undefined") {
            s = "";
        }
        switch (mode) {
            case "l":
                if (s === "") {
                    expr = "//*[@onclick or @onmouseover or @onmousedown or @onmouseup or @oncommand or @class='lk' or @role='link' or @href] | //input[not(@type='hidden')] | //a[href] | //area | //textarea | //button | //select";
                } else {
                    expr = "//*[(@onclick or @onmouseover or @onmousedown or @onmouseup or @oncommand or @class='lk' or @role='link' or @href) and contains(., '" + s + "')] | //input[not(@type='hidden') and contains(., '" + s + "')] | //a[@href and contains(., '" + s + "')] | //area[contains(., '" + s + "')] |  //textarea[contains(., '" + s + "')] | //button[contains(@value, '" + s + "')] | //select[contains(., '" + s + "')]";
                }
                break;
            case "i":
                if (s === "") {
                    expr = "//img[@src]";
                } else {
                    expr = "//img[@src and contains(., '" + s + "')]";
                }
                break;
        }
        return expr;
    }
};
