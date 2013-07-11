var VbHint = (function(){
    'use strict';

    var hConts   = [],               /* holds the hintcontainers of the different documents */
        hints    = [],               /* holds all hint data (hinted element, label, number) */
        focusNum = 1,                /* number of current focused hint */
        cId      = "_hintContainer", /* id of the conteiner holding the hint lables */
        lClass   = "_hintLabel",     /* class used on the hint labels with the hint numbers */
        hClass   = "_hintElem",      /* marks hinted elements */
        fClass   = "_hintFocus",     /* marks focused element and focued hint */
        config;

    function create(inputText) {
        var topWinH = window.innerHeight,
            topWinW = window.innerWidth,
            count   = 0;

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
                labelTmpl = doc.createElement("span"),
                /* Bounds */
                minX = offsetX < 0 ? -offsetX : 0,
                minY = offsetY < 0 ? -offsetY : 0,
                maxX = offsetX + win.width > topWinW ? topWinW - offsetX : topWinW,
                maxY = offsetY + win.height > topWinH ? topWinH - offsetY : topWinH,
                rect, e, i;

            labelTmpl.className = lClass;

            createStyle(doc);

            /* due to the different XPath result type, we will need two counter variables */
            for (i = 0; i < res.snapshotLength; i++) {
                if (count >= config.maxHints) {
                    break;
                }

                e    = res.snapshotItem(i);
                rect = e.getBoundingClientRect();
                if (!rect || rect.left > maxX || rect.right < minX || rect.top > maxY || rect.bottom < minY) {
                    continue;
                }

                var cStyle = window.getComputedStyle(e, "");
                if (cStyle.display === "none" || cStyle.visibility !== "visible") {
                    continue;
                }

                /* create the hint label with number */
                var label        = labelTmpl.cloneNode(false);
                label.style.left = Math.max((rect.left + win.scrollX), win.scrollX) - 3 + "px";
                label.style.top  = Math.max((rect.top + win.scrollY), win.scrollY) - 3 + "px";
                label.innerText  = count + 1;

                fragment.appendChild(label);

                /* add the hint class to the hinted element */
                e.classList.add(hClass);

                count++;
                hints.push({
                    e:     e,
                    num:   count,
                    label: label
                });
            }

            var hDiv = doc.createElement("div");
            hDiv.id  = cId;
            hDiv.appendChild(fragment);
            doc.documentElement.appendChild(hDiv);

            hConts.push(hDiv);

            /* recurse into any iframe or frame element */
            var frameTags = ["frame", "iframe"],
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

        helper(window, 0, 0);

        if (count <= 1) {
            return fire(1);
        }
        return focusHint(1);
    }

    function createStyle(doc)
    {
        if (doc.hasStyle) {
            return;
        }
        var e = doc.createElement("style");
        e.innerHTML += "." + lClass + "{" +
            "position:absolute;" +
            "z-index:100000;" +
            "font-family:monospace;" +
            "font-weight:bold;" +
            "font-size:10px;" +
            "color:#000;" +
            "background-color:#fff;" +
            "margin:0;" +
            "padding:0px 1px;" +
            "border:1px solid #444;" +
            "opacity:0.7" +
            "}" +
            "." + hClass + "{" +
            "background-color:#ff0 !important;" +
            "color:#000 !important" +
            "}" +
            "." + hClass + "." + fClass + "{" +
            "background-color:#8f0 !important" +
            "}" +
            "." + lClass + "." + fClass + "{" +
            "opacity:1" +
            "}";

        doc.head.appendChild(e);
        /* prevent us from adding the style multiple times */
        doc.hasStyle = true;
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
                hint.e.classList.remove(fClass);
                hint.e.classList.remove(hClass);
                hint.label.parentNode.removeChild(hint.label);
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
            hint.e.classList.remove(fClass);
            hint.label.classList.remove(fClass);

            mouseEvent(hint.e, "mouseout");
        }

        focusNum = n;

        /* mark new hint as focused */
        hint = getHint(focusNum);
        if (hint) {
            hint.e.classList.add(fClass);
            hint.label.classList.add(fClass);

            mouseEvent(hint.e, "mouseover");

            var source = getSrc(hint.e);
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
        hint.e.classList.remove(fClass);
        hint.e.classList.remove(hClass);
        hint.label.parentNode.removeChild(hint.label);

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
        /* mode: l - links, i - images, e - editables */
        /* usage: O - open, T - open in new window, U - use source */
        init: function init(mode, usage, maxHints) {
            config = {
                mode:     mode,
                usage:    usage,
                maxHints: maxHints
            };
        },
        create: create,
        update: update,
        clear:  clear,
        fire:   fire,
        focus:  focus
    };
})();
Object.freeze(VbHint);
