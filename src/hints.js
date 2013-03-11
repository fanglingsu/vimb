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

        var hintCount = 0;
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
            var rect, elem, text, node, show_text;
            for (i = 0; i < res.snapshotLength; i++) {
                if (hintCount >= maxHints) {
                    break;
                }

                elem = res.snapshotItem(i);
                rect = elem.getBoundingClientRect();
                if (!rect || rect.left > maxX || rect.right < minX || rect.top > maxY || rect.bottom < minY) {
                    continue;
                }

                var cStyle = topwin.getComputedStyle(elem, "");
                if (cStyle.display === "none" || cStyle.visibility !== "visible") {
                    continue;
                }

                var leftpos = Math.max((rect.left + scrollX), scrollX);
                var toppos = Math.max((rect.top + scrollY), scrollY);

                /* making this block DOM compliant */
                var hint = hintSpan.cloneNode(false);
                hint.style.left = leftpos - 3 + "px";
                hint.style.top =  toppos - 3 + "px";
                text = doc.createTextNode(hintCount + 1);
                hint.appendChild(text);

                fragment.appendChild(hint);
                hintCount++;
                hints.push({
                    elem:       elem,
                    number:     hintCount,
                    span:       hint,
                    background: elem.style.background,
                    foreground: elem.style.color}
                );

                /* make the link black to ensure it's readable */
                elem.style.color = fg;
                elem.style.background = bg;
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
                    elem = frames[i];
                    rect = elem.getBoundingClientRect();
                    if (!elem.contentWindow || !rect || rect.left > maxX || rect.right < minX || rect.top > maxY || rect.bottom < minY) {
                        continue;
                    }
                    _helper(elem.contentWindow, offsetX + rect.left, offsetY + rect.top);
                }
            }
        }

        _helper(topwin, 0, 0);

        if (hintCount <= 1) {
            return this.fire(1);
        }
        return _focus(1);
    };

    /* set focus to next avaiable hint */
    this.focusNext = function()
    {
        var index = _getHintIdByNumber(curFocusNum);

        if (typeof(hints[index + 1]) !== "undefined") {
            return _focus(hints[index + 1].number);
        }
        return _focus(hints[0].number);
    };

    /* set focus to previous avaiable hint */
    this.focusPrev = function()
    {
        var index = _getHintIdByNumber(curFocusNum);
        if (index !== 0 && typeof(hints[index - 1].number) !== "undefined") {
            return _focus(hints[index - 1].number);
        }
        return _focus(hints[hints.length - 1].number);
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
            if (0 !== hint.number.toString().indexOf(n.toString())) {
                remove.push(hint.number);
            }
        }

        for (i = 0; i < remove.length; ++i) {
            _removeHint(remove[i]);
        }

        if (hints.length === 1) {
            return this.fire(hints[0].number);
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
            if (hint.elem) {
                hint.elem.style.background = hint.background;
                hint.elem.style.color = hint.foreground;
                hCont.removeChild(hint.span);
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
        var hint = _getHintByNumber(n);
        if (!hint || typeof(hint.elem) == "undefined") {
            return "DONE:";
        }

        var el  = hint.elem;
        var tag = el.nodeName.toLowerCase();

        this.clear();

        if (tag === "iframe" || tag === "frame" || tag === "textarea" || tag === "select" || tag === "input"
            && (el.type !== "image" && el.type !== "submit")
        ) {
            el.focus();
            if (tag === "input" || tag === "textarea") {
                return "INSERT:";
            }
            return "DONE:";
        }

        switch (usage) {
            case "T": _openNewWindow(el); return "DONE:";
            case "O": _open(el); return "DONE:";
            default: return "DATA:" + _getSrc(el);
        }
    };

    /* opens given element */
    function _open(elem)
    {
        if (elem.target == "_blank") {
            elem.removeAttribute("target");
        }
        _mouseEvent(elem, "moudedown", 0);
        _mouseEvent(elem, "click", 0);
    }

    /* opens given element into new window */
    function _openNewWindow(elem)
    {
        var oldTarget = elem.target;

        /* set target to open in new window */
        elem.target = "_blank";
        _mouseEvent(elem, "moudedown");
        _mouseEvent(elem, "click");
        elem.target = oldTarget;
    }


    /* set focus on hint with given number */
    function _focus(n)
    {
        /* reset previous focused hint */
        var hint = _getHintByNumber(curFocusNum);
        if (hint !== null) {
            hint.elem.className = hint.elem.className.replace(hClassFocus, hClass);
            hint.elem.style.background = bg;
            _mouseEvent(hint.elem, "mouseout");
        }

        curFocusNum = n;

        /* mark new hint as focused */
        hint = _getHintByNumber(curFocusNum);
        if (hint !== null) {
            hint.elem.className = hint.elem.className.replace(hClass, hClassFocus);
            hint.elem.style.background = bgf;
            _mouseEvent(hint.elem, "mouseover");
            var source = _getSrc(hint.elem);

            return "OVER:" + (source ? source : "");
        }
    }

    /* retrieves the hint for given hint number */
    function _getHintByNumber(n)
    {
        var index = _getHintIdByNumber(n);
        if (index !== null) {
            return hints[index];
        }
        return null;
    }

    /* retrieves the id of hint with given number */
    function _getHintIdByNumber(n)
    {
        var hint;
        for (var i = 0; i < hints.length; ++i) {
            hint = hints[i];
            if (hint.number === n) {
                return i;
            }
        }
        return null;
    }

    /* removes hint with given number from hints array */
    function _removeHint(n)
    {
        var index = _getHintIdByNumber(n);
        if (index === null) {
            return;
        }
        var hint = hints[index];
        if (hint.number === n) {
            hint.elem.style.background = hint.background;
            hint.elem.style.color = hint.foreground;
            hint.span.parentNode.removeChild(hint.span);

            /* remove hints from all hints */
            hints.splice(index, 1);
        }
    }

    function _mouseEvent(elem, name)
    {
        var doc = elem.ownerDocument;
        var view = elem.contentWindow;

        var evObj = doc.createEvent("MouseEvents");
        evObj.initMouseEvent(name, true, true, view, 0, 0, 0, 0, 0, false, false, false, false, 0, null);
        elem.dispatchEvent(evObj);
    }

    /* retrieves the url of given element */
    function _getSrc(elem)
    {
        return elem.href || elem.src;
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
