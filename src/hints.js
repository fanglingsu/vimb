VimpHints = function Hints(bg, bgf, fg, style) {
    "use strict";
    var config = {
        maxHints: 200,
        hintCss: style,
        hintClass: "__hint",
        hintClassFocus: "__hint_container",
        eBg: bg,
        eBgf: bgf,
        eFg: fg
    };

    var hCont;
    var curFocusNum = 1;
    var hints = [];
    var mode;

    this.create = function(inputText, hintMode)
    {
        if (hintMode) {
            mode = hintMode;
        }

        var topwin = window;
        var top_height = topwin.innerHeight;
        var top_width = topwin.innerWidth;
        var xpath_expr;

        var hintCount = 0;
        this.clear();

        function _helper (win, offsetX, offsetY) {
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

            hCont = doc.createElement("div");
            hCont.id = "hint_container";

            xpath_expr = _getXpathXpression(inputText);

            var res = doc.evaluate(xpath_expr, doc,
                function (p) {
                    return "http://www.w3.org/1999/xhtml";
                }, XPathResult.ORDERED_NODE_SNAPSHOT_TYPE, null);

            /* generate basic hint element which will be cloned and updated later */
            var hintSpan = doc.createElement("span");
            hintSpan.setAttribute("class", config.hintClass);
            hintSpan.style.cssText = config.hintCss;

            /* due to the different XPath result type, we will need two counter variables */
            var rect, elem, text, node, show_text;
            for (i = 0; i < res.snapshotLength; i++) {
                if (hintCount >= config.maxHints) {
                    break;
                }

                elem = res.snapshotItem(i);
                rect = elem.getBoundingClientRect();
                if (!rect || rect.left > maxX || rect.right < minX || rect.top > maxY || rect.bottom < minY) {
                    continue;
                }

                var style = topwin.getComputedStyle(elem, "");
                if (style.display == "none" || style.visibility != "visible") {
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

                hCont.appendChild(hint);
                hintCount++;
                hints.push({
                    elem:       elem,
                    number:     hintCount,
                    span:       hint,
                    background: elem.style.background,
                    foreground: elem.style.color}
                );

                /* make the link black to ensure it's readable */
                elem.style.color = config.eFg;
                elem.style.background = config.eBg;
            }

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
        _focus(1);
    };

    /* set focus to next avaiable hint */
    this.focusNext = function()
    {
        var index = _getHintIdByNumber(curFocusNum);

        if (typeof(hints[index + 1]) !== "undefined") {
            _focus(hints[index + 1].number);
        } else {
            _focus(hints[0].number);
        }
    };

    /* set focus to previous avaiable hint */
    this.focusPrev = function()
    {
        var index = _getHintIdByNumber(curFocusNum);
        if (index !== 0 && typeof(hints[index - 1].number) !== "undefined") {
            _focus(hints[index - 1].number);
        } else {
            _focus(hints[hints.length - 1].number);
        }
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
            if (typeof(hint.elem) !== "undefined") {
                hint.elem.style.background = hint.background;
                hint.elem.style.color = hint.foreground;
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
        var result = "DONE:";
        var hint = _getHintByNumber(n);
        if (!hint || typeof(hint.elem) == "undefined") {
            return result;
        }

        var el  = hint.elem;
        var tag = el.nodeName.toLowerCase();

        this.clear();

        if (tag === "iframe" || tag === "frame" || tag === "textarea" || tag === "select" || tag === "input"
            && (el.type != "image" && el.type != "submit")
        ) {
            el.focus();
            if (tag === "input" || tag === "textarea") {
                return "INSERT:";
            }
            return "DONE:";
        }

        switch (mode) {
            case "f":
                _open(el);
                break;
            case "F":
                _openNewWindow(el);
                break;
            default:
                result = "DATA:" + _getElemtSource(el);
                break;
        }

        return result;
    };

    /* set focus on hint with given number */
    function _focus(n)
    {
        /* reset previous focused hint */
        var hint = _getHintByNumber(curFocusNum);
        if (hint !== null) {
            hint.elem.className = hint.elem.className.replace(config.hintClassFocus, config.hintClass);
            hint.elem.style.background = config.eBg;
            _mouseEvent(hint.elem, "mouseout");
        }

        curFocusNum = n;

        /* mark new hint as focused */
        hint = _getHintByNumber(curFocusNum);
        if (hint !== null) {
            hint.elem.className = hint.elem.className.replace(config.hintClass, config.hintClassFocus);
            hint.elem.style.background = config.eBgf;
            _mouseEvent(hint.elem, "mouseover");
        }
    }

    /* retrieves text content fro given element */
    function _getTextFromElement(el)
    {
        var text;
        if (el instanceof HTMLInputElement || el instanceof HTMLTextAreaElement) {
            text = el.value;
        } else if (el instanceof HTMLSelectElement) {
            if (el.selectedIndex >= 0) {
                text = el.item(el.selectedIndex).text;
            } else{
                text = "";
            }
        } else {
            text = el.textContent;
        }
        return text.toLowerCase();
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

    /* opens given element */
    function _open(elem)
    {
        if (elem.target == "_blank") {
            elem.removeAttribute("target");
        }
        _mouseEvent(elem, "moudedown");
        _mouseEvent(elem, "click");
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

    function _mouseEvent(elem, name)
    {
        var doc = elem.ownerDocument;
        var view = elem.contentWindow;

        var evObj = doc.createEvent("MouseEvents");
        evObj.initMouseEvent(name, true, true, view, 0, 0, 0, 0, 0, false, false, false, false, 0, null);
        elem.dispatchEvent(evObj);
    }

    /* retrieves the url of given element */
    function _getElemtSource(elem)
    {
        var url = elem.href || elem.src;
        return url;
    }

    /* retrieves the xpath expression according to mode */
    function _getXpathXpression(text)
    {
        var expr;
        if (typeof(text) === "undefined") {
            text = "";
        }
        switch (mode) {
            case "f":
            case "F":
                if (text === "") {
                    expr = "//*[@onclick or @onmouseover or @onmousedown or @onmouseup or @oncommand or @class='lk' or @role='link' or @href] | //input[not(@type='hidden')] | //a[href] | //area | //textarea | //button | //select";
                } else {
                    expr = "//*[(@onclick or @onmouseover or @onmousedown or @onmouseup or @oncommand or @class='lk' or @role='link' or @href) and contains(., '" + text + "')] | //input[not(@type='hidden') and contains(., '" + text + "')] | //a[@href and contains(., '" + text + "')] | //area[contains(., '" + text + "')] |  //textarea[contains(., '" + text + "')] | //button[contains(@value, '" + text + "')] | //select[contains(., '" + text + "')]";
                }
                break;
            case "i":
            case "I":
                if (text === "") {
                    expr = "//img[@src]";
                } else {
                    expr = "//img[@src and contains(., '" + text + "')]";
                }
                break;
            default:
                if (text === "") {
                    expr = "//*[@role='link' or @href] | //a[href] | //area | //img[not(ancestor::a)]";
                } else {
                    expr = "//*[(@role='link' or @href) and contains(., '" + text + "')] | //a[@href and contains(., '" + text + "')] | //area[contains(., '" + text + "')] | //img[not(ancestor::a) and contains(., '" + text + "')]";
                }
                break;
        }
        return expr;
    }
};
