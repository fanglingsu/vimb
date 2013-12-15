var VbHint = (function(){
    'use strict';

    var hints      = [],               /* holds all hint data (hinted element, label, number) in view port */
        docs       = [],               /* hold the affected document with the start and end index of the hints */
        validHints = [],               /* holds the valid hinted elements matching the filter condition */
        activeHint = 1,                /* number of current focused hint in valid hints array */
        filterText = "",               /* holds the typed filter text */
        filterNum  = 0,                /* holds the numeric filter */
        cId      = "_hintContainer",   /* id of the conteiner holding the hint lables */
        lClass   = "_hintLabel",       /* class used on the hint labels with the hint numbers */
        hClass   = "_hintElem",        /* marks hinted elements */
        fClass   = "_hintFocus",       /* marks focused element and focued hint */
        config,
        style    = "." + lClass + "{" +
            "-webkit-transform:translate(-4px,-4px);" +
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

    function clear() {
        var i, j, hint, doc, e;
        for (i = 0; i < docs.length; i++) {
            doc = docs[i];
            /* find all hinted elements vimbhint 'hint' */
            var res = xpath(doc.doc, "//*[contains(@vimbhint, 'hint')]");
            for (j = 0; j < res.snapshotLength; j++) {
                e = res.snapshotItem(j);
                e.removeAttribute("vimbhint");
                e.classList.remove(fClass);
                e.classList.remove(hClass);
            }
            doc.div.parentNode.removeChild(doc.div);
        }
        docs       = [];
        hints      = [];
        validHints = [];
        filterText = "";
        filterNum  = 0;
        activeHint = 1;
    }

    function create() {
        var count = 0;

        function helper(win, offsets) {
            /* document may be undefined for frames out of the same origin */
            /* policy and will break the whole code - so we check this before */
            if (win.document === undefined) {
                return;
            }

            offsets        = offsets || {left: 0, right: 0, top: 0, bottom: 0};
            offsets.right  = win.innerWidth  - offsets.right;
            offsets.bottom = win.innerHeight - offsets.bottom;

            function isVisible(e) {
                if (e === undefined) {
                    return false;
                }
                var rect = e.getBoundingClientRect();
                if (!rect ||
                    rect.top > offsets.bottom || rect.bottom < offsets.top ||
                    rect.left > offsets.right || rect.right < offsets.left ||
                    !rect.width || !rect.height
                ) {
                    return false;
                }

                var s = win.getComputedStyle(e, null);
                return s.display !== "none" && s.visibility == "visible";
            }

            var doc       = win.document,
                res       = xpath(doc, getXpath()),
                /* generate basic hint element which will be cloned and updated later */
                labelTmpl = doc.createElement("span"),
                e, i;

            labelTmpl.className = lClass;
            labelTmpl.setAttribute("vimbhint", "label");

            var containerOffsets = getOffsets(doc),
                offsetX = containerOffsets[0],
                offsetY = containerOffsets[1],
                fragment = doc.createDocumentFragment(),
                rect, label, text, showText, start = hints.length;

            /* collect all visible elements in hints array */
            for (i = 0; i < res.snapshotLength; i++) {
                e = res.snapshotItem(i);
                if (!isVisible(e)) {
                    continue;
                }

                count++;

                /* create the hint label with number */
                rect  = e.getBoundingClientRect();
                label = labelTmpl.cloneNode(false);
                label.style.left = Math.max((rect.left + offsetX), offsetX) + "px";
                label.style.top  = Math.max((rect.top  + offsetY), offsetY) + "px";
                label.style.display = "none";

                /* if hinted element is an image - show title or alt of the image in hint label */
                /* this allows to see how to filter for the image */
                text = "";
                showText = false;
                if (e instanceof HTMLImageElement) {
                    text     = e.title || e.alt;
                    showText = true;
                } else if (e.firstElementChild instanceof HTMLImageElement && /^\s*$/.test(e.textContent)) {
                    text     = e.firstElementChild.title || e.firstElementChild.alt;
                    showText = true;
                } else if (e instanceof HTMLInputElement) {
                    var type = e.type;
                    if (type === "image") {
                        text = e.alt || "";
                    } else if (e.value && type !== "password") {
                        text     = e.value;
                        showText = (type === "radio" || type === "checkbox");
                    }
                } else if (e instanceof HTMLSelectElement) {
                    if (e.selectedIndex >= 0) {
                        text = e.item(e.selectedIndex).text;
                    }
                } else {
                    text = e.textContent;
                }
                /* add the hint class to the hinted element */
                fragment.appendChild(label);
                e.setAttribute("vimbhint", "hint");

                hints.push({
                    e:        e,
                    label:    label,
                    text:     text,
                    showText: showText
                });

                if (count >= config.maxHints) {
                    break;
                }
            }

            /* append the fragment to the document */
            var hDiv = doc.createElement("div");
            hDiv.id  = cId;
            hDiv.appendChild(fragment);
            hDiv.setAttribute("vimbhint", "container");
            if (doc.body) {
                doc.body.appendChild(hDiv);
            }
            /* create the default style sheet */
            createStyle(doc);

            docs.push({
                doc:   doc,
                start: start,
                end:   hints.length - 1,
                div:   hDiv
            });

            /* recurse into any iframe or frame element */
            for (var f in win.frames) {
                var e = f.frameElement;
                if (isVisible(e)) {
                    var rect = e.getBoundingClientRect();
                    helper(f, {
                        left:   Math.max(offsets.left - rect.left, 0),
                        right:  Math.max(rect.right   - offsets.right, 0),
                        top:    Math.max(offsets.top  - rect.top, 0),
                        bottom: Math.max(rect.bottom  - offsets.bottom, 0)
                    });
                }
            }
        }

        helper(window);
    }

    function show() {
        var i, hint, doc, num = 1, activeHint = filterNum || 1,
            matcher = getMatcher(filterText);
        /* clear the array of valid hints */
        validHints = [];
        for (i = 0; i < hints.length; i++) {
            hint = hints[i];
            /* collect only hints matching the filter text */
            if (matcher(hint.text)) {
                /* assigne the new hint number to the hint */
                hint.num = num++;
                setFocus(hint, activeHint === hint.num);
                /* check for number filter */
                if (!filterNum || 0 === hint.num.toString().indexOf(filterNum.toString())) {
                    hint.label.style.display = "";
                    hint.e.classList.add(hClass);

                    /* create the label with the hint number */
                    hint.label.innerText = hint.num;
                    if (hint.showText && hint.text) {
                        /* use \x20 instead of ' ' to keep this space during */
                        /* js2h.sh processing */
                        hint.label.innerText += ":\x20" + hint.text.substr(0, 20);
                    }
                    validHints.push(hint);
                    continue;
                }
            }
            /* remove hint labels from no more visible hints */
            hint.label.style.display = "none";
            hint.label.classList.remove(fClass);
            hint.e.classList.remove(fClass);
            hint.e.classList.remove(hClass);
        }

        if (validHints.length === 1) {
            return fire();
        }
        return focusHint(1, activeHint);
    }

    /* Retruns a vlidator method to check if the hint elemens text matches */
    /* the given filter text. */
    function getMatcher(text) {
        var tokens = text.toLowerCase().split(/\s+/);
        return function (itemText) {
            itemText = itemText.toLowerCase();
            return tokens.every(function (token) {
                return 0 <= itemText.indexOf(token);
            });
        };
    }

    function getOffsets(doc) {
        var body  = doc.body || doc.documentElement,
            style = body.style,
            rect;

        if (style && /^(absolute|fixed|relative)$/.test(style.position)) {
            rect = body.getClientRects()[0];
            return [-rect.left, -rect.top];
        }
        return [doc.defaultView.scrollX, doc.defaultView.scrollY];
    }

    function createStyle(doc) {
        if (doc.hasStyle) {
            return;
        }
        var e = doc.createElement("style");
        e.innerHTML = style;
        doc.head.appendChild(e);
        /* prevent us from adding the style multiple times */
        doc.hasStyle = true;
    }

    function focus(back) {
        var old = activeHint;
        if (back) {
            if (--activeHint < 1) {
                activeHint = validHints.length;
            }
        } else {
            if (++activeHint > validHints.length) {
                activeHint = 1;
            }
        }
        return focusHint(activeHint, old);
    }

    function fire(num) {
        var hint = validHints[(num || activeHint) - 1];
        if (!hint) {
            return "DONE:";
        }

        var e    = hint.e,
            tag  = e.nodeName.toLowerCase(),
            type = e.type || "";

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

        switch (config.usage) {
            case "T": open(e, true); return "DONE:";
            case "O": open(e, false); return "DONE:";
            default: return "DATA:" + getSrc(e);
        }
    }

    /* internal used methods */
    function open(e, newWin) {
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
    function focusHint(newNum, oldNum) {
        /* reset previous focused hint */
        var hint;
        if ((hint = validHints[oldNum - 1])) {
            setFocus(hint, false);

            mouseEvent(hint.e, "mouseout");
        }

        /* mark new hint as focused */
        activeHint = newNum;
        if ((hint = validHints[newNum - 1])) {
            setFocus(hint, true);

            mouseEvent(hint.e, "mouseover");

            var src = getSrc(hint.e);
            return "OVER:" + (src || "");
        }
    }

    /* Toggles the focus highlight of a hint */
    function setFocus(hint, active) {
        if (active) {
            /* TODO place and remove the active value independent from 'hint' */
            hint.e.setAttribute("vimbhint", "hint active");
            hint.e.classList.add(fClass);
            hint.label.setAttribute("vimbhint", "label active");
            hint.label.classList.add(fClass);
        } else {
            hint.e.setAttribute("vimbhint", "hint");
            hint.e.classList.remove(fClass);
            hint.label.setAttribute("vimbhint", "label");
            hint.label.classList.remove(fClass);
        }
    }

    function click(e) {
        /* for sites that are interested in mouseover before click */
        mouseEvent(e, "mouseover");
        /* this is the w3.org definition for DOM-Level 2 events */
        mouseEvent(e, "mousedown");
        mouseEvent(e, "mouseup");
        mouseEvent(e, "click");
    }

    function mouseEvent(e, name) {
        var evObj = e.ownerDocument.createEvent("MouseEvents");
        evObj.initMouseEvent(name, true, true, e.contentWindow, 0, 0, 0, 0, 0, false, false, false, false, 0, null);
        e.dispatchEvent(evObj);
    }

    /* retrieves the url of given element */
    function getSrc(e) {
        return e.href || e.src;
    }

    /* retrieves the xpath expression according to mode */
    function getXpath() {
        switch (config.mode) {
            case "l":
                return "//*[@href] | //*[@onclick or @tabindex or @class='lk' or @role='link' or @role='button'] | //input[not(@type='hidden' or @disabled or @readonly)] | //textarea[not(@disabled or @readonly)] | //button | //select";
            case "e":
                return "//input[not(@type) or @type='text'] | //textarea";
            case "i":
                return "//img[@src]";
        }
    }

    function xpath(doc, expr) {
        return doc.evaluate(
            expr, doc, function (p) {return "http://www.w3.org/1999/xhtml";},
            XPathResult.ORDERED_NODE_SNAPSHOT_TYPE, null
        );
    }

    /* follow the count last link on pagematching the given pattern */
    function followLink(rel, pattern, count) {
        /* returns array of matching elements */
        function followFrame(frame) {
            var i, p, reg, res = [],
                doc = frame.document,
                elems = [],
                all = doc.getElementsByTagName("a");

            /* first match links by rel attribute */
            for (i = all.length - 1; i >= 0; i--) {
                /* collect visible elements */
                var s = doc.defaultView.getComputedStyle(all[i], null);
                if (s.display !== "none" && s.visibility === "visible") {
                    /* if there are rel attributes alaments, put them in the result */
                    if (all[i].rel.toLowerCase() === rel) {
                        res.push(all[i]);
                    } else {
                        /* save to match them later */
                        elems.push(all[i]);
                    }
                }
            }
            /* match each pattern successively against each link in the page */
            for (p = 0; p < pattern.length; p++) {
                reg = pattern[p];
                /* begin with the last link on page */
                for (i = elems.length - 1; i >= 0; i--) {
                    if (elems[i].innerText.match(reg)) {
                        res.push(elems[i]);
                    }
                }
            }
            return res;
        }
        var i, j, elems, frames = allFrames(window);
        for (i = 0; i < frames.length; i++) {
            elems = followFrame(frames[i]);
            for (j = 0; j < elems.length; j++) {
                if (--count == 0) {
                    open(elems[j], false);
                    return "DONE:";
                }
            }
        }
        return "NONE:";
    }

    function allFrames(win) {
        var i, f, frames = [win];
        for (i = 0; i < win.frames.length; i++) {
            frames.push(win.frames[i].frameElement);
        }
        return frames;
    }

    /* the api */
    return {
        init: function init(prefix, maxHints) {
            /* mode: l - links, i - images, e - editables */
            /* usage: O - open, T - open in new window, U - use source */
            var map = {
                /* prompt : [mode, usage] */
                ";e": ['e', 'U'],
                ";i": ['i', 'U'],
                ";I": ['i', 'U'],
                ";o": ['l', 'O'],
                ";O": ['l', 'U'],
                ";p": ['l', 'U'],
                ";P": ['l', 'U'],
                ";s": ['l', 'U'],
                ";t": ['l', 'T'],
                ";T": ['l', 'U'],
                ";y": ['l', 'U']
            };
            /* default config */
            config = {
                mode:     'l',
                usage:    'O',
                maxHints: maxHints
            };
            /* overwrite with mapped config if found */
            if (map.hasOwnProperty(prefix)) {
                config.mode  = map[prefix][0];
                config.usage = map[prefix][1];
            }
            create();
            return show();
        },
        filter: function filter(text) {
            filterText = text || "";
            return show();
        },
        update: function update(n) {
            filterNum = n;
            return show();
        },
        clear:      clear,
        fire:       fire,
        focus:      focus,
        /* not really hintings but uses similar logic */
        followLink: followLink
    };
})();
Object.freeze(VbHint);
