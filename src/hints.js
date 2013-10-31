var VbHint = (function(){
    'use strict';

    var hConts   = [],               /* holds the hintcontainers of the different documents */
        hints    = [],               /* holds all hint data (hinted element, label, number) */
        ixdFocus = 0,                /* index of current focused hint */
        cId      = "_hintContainer", /* id of the conteiner holding the hint lables */
        lClass   = "_hintLabel",     /* class used on the hint labels with the hint numbers */
        hClass   = "_hintElem",      /* marks hinted elements */
        fClass   = "_hintFocus",     /* marks focused element and focued hint */
        config;

    function create(inputText) {
        clear();

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

            var doc   = win.document,
                xpath = getXpath(inputText),
                res   = doc.evaluate(
                    xpath, doc,
                    function (p) {return "http://www.w3.org/1999/xhtml";},
                    XPathResult.ORDERED_NODE_SNAPSHOT_TYPE, null
                ),
                /* generate basic hint element which will be cloned and updated later */
                labelTmpl = doc.createElement("span"),
                e, i;

            labelTmpl.className = lClass;

            var containerOffsets = getOffsets(doc),
                offsetX = containerOffsets[0],
                offsetY = containerOffsets[1],
                fragment = doc.createDocumentFragment(),
                rect, label, text;

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
                label.innerText  = count;

                /* if hinted element is an image - show title or alt of the image in hint label */
                /* this allows to see how to filter for the image */
                text = "";
                if (e instanceof HTMLImageElement) {
                    text = e.alt || e.title;
                } else if (e.firstElementChild instanceof HTMLImageElement && /^\s*$/.test(e.textContent)) {
                    text = e.firstElementChild.title || e.firstElementChild.alt;
                }
                if (text) {
                    label.innerText += ": " + text.substr(0, 20);
                }

                /* add the hint class to the hinted element */
                e.classList.add(hClass);
                fragment.appendChild(label);

                hints.push({
                    e:     e,
                    num:   count,
                    label: label
                });

                if (count >= config.maxHints) {
                    break;
                }
            }

            /* append the fragment to the document */
            var hDiv = doc.createElement("div");
            hDiv.id  = cId;
            hDiv.appendChild(fragment);
            doc.documentElement.appendChild(hDiv);
            /* create the default style sheet */
            createStyle(doc);

            hConts.push(hDiv);

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

        if (count <= 1) {
            return fire(0);
        }
        return focusHint(0);
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
        e.innerHTML += "." + lClass + "{" +
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

        doc.head.appendChild(e);
        /* prevent us from adding the style multiple times */
        doc.hasStyle = true;
    }

    function focus(back) {
        var n, i = ixdFocus;
        if (back) {
            n = (i >= 1) ? i - 1 : hints.length - 1;
        } else {
            n = (i + 1 < hints.length) ? i + 1 : 0;
        }
        return focusHint(n);
    }

    function update(n) {
        var remove = [], idx, r, i, hint;
        if (n === 0) {
            return create();
        }
        /* remove none matching hints */
        for (i = 0; i < hints.length; ++i) {
            hint = hints[i];
            /* collect the hints to be removed */
            if (0 !== hint.num.toString().indexOf(n.toString())) {
                remove.push(hint);
            }
        }

        /* now remove the hints */
        for (i = 0; i < remove.length; ++i) {
            r = remove[i];
            r.e.classList.remove(fClass);
            r.e.classList.remove(hClass);
            r.label.parentNode.removeChild(r.label);

            /* remove hints from all hints */
            if ((idx = hints.indexOf(r)) !== -1) {
                hints.splice(idx, 1);
            }
        }


        if (hints.length === 1) {
            return fire(0);
        }
        return focusHint(0);
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

    function fire(i) {
        var hint = getHint(i || ixdFocus);
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
    function focusHint(i) {
        /* reset previous focused hint */
        var hint;
        if ((hint = getHint(ixdFocus))) {
            hint.e.classList.remove(fClass);
            hint.label.classList.remove(fClass);

            mouseEvent(hint.e, "mouseout");
        }

        /* mark new hint as focused */
        ixdFocus = i;
        if ((hint = getHint(i))) {
            hint.e.classList.add(fClass);
            hint.label.classList.add(fClass);

            mouseEvent(hint.e, "mouseover");

            var src = getSrc(hint.e);
            return "OVER:" + (src || "");
        }
    }

    /* retrieves the hint for given hint number */
    function getHint(i) {
        return hints[i] || null;
    }

    function click(e) {
        mouseEvent(e, "mouseover");
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
    function getXpath(s) {
        if (s === undefined) {
            s = "";
        }
        /* replace $WHAT in xpath to contains(translate(WHAT, 'SEARCH', 'search'), 'search') */
        function buildQuery(what, x, s) {
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
                    return "//a[@href] | //*[@onclick or @tabindex or @class='lk' or @role='link' or @role='button'] | //input[not(@type='hidden' or @disabled or @readonly)] | //textarea[not(@disabled or @readonly)] | //button | //select";
                }
                return buildQuery(
                    ["@value", ".", "@placeholder", "@title", "@alt"],
                    "//a[@href and ($. or child::img[$@title or $@alt])] | //*[(@onclick or @class='lk' or @role='link' or role='button') and $.] | //input[not(@type='hidden' or @disabled or @readonly) and ($@value or $@placeholder)] | //textarea[not(@disabled or @readonly) and $.] | //button[$.] | //select[$.]",
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
        },
        create:     create,
        update:     update,
        clear:      clear,
        fire:       fire,
        focus:      focus,
        /* not really hintings but uses similar logic */
        followLink: followLink
    };
})();
Object.freeze(VbHint);
