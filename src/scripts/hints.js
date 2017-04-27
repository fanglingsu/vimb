var hints = Object.freeze((function(){
    'use strict';

    var hints      = [],               /* holds all hint data (hinted element, label, number) in view port */
        docs       = [],               /* hold the affected document with the start and end index of the hints */
        validHints = [],               /* holds the valid hinted elements matching the filter condition */
        activeHint,                    /* holds the active hint object */
        filterText = "",               /* holds the typed filter text */
        filterNum  = 0,                /* holds the numeric filter */
        /* TODO remove these classes and use the 'vimbhint' attribute for */
        /* styling the hints and labels - but this might break user */
        /* stylesheets that use the classes for styling */
        cId      = "_hintContainer",   /* id of the container holding the hint labels */
        lClass   = "_hintLabel",       /* class used on the hint labels with the hint numbers */
        hClass   = "_hintElem",        /* marks hinted elements */
        fClass   = "_hintFocus",       /* marks focused element and focussed hint */
        config;
    /* the hint class used to maintain hinted element and labels */
    function Hint() {
        /* hide hint label and remove coloring from hinted element */
        this.hide = function() {
            /* remove hint labels from no more visible hints */
            this.label.style.display = "none";
            this.e.classList.remove(fClass);
            this.e.classList.remove(hClass);
        };

        /* show the hint element colored with the hint label */
        this.show = function() {
            this.label.style.display = "";
            this.e.classList.add(hClass);

            /* create the label with the hint number */
            var text = [];
            if (this.e instanceof HTMLInputElement) {
                var type = this.e.type;
                if (type === "checkbox") {
                    text.push(this.e.checked ? "☑" : "☐");
                } else if (type === "radio") {
                    text.push(this.e.checked ? "⊙" : "○");
                }
            }
            if (this.showText && this.text) {
                text.push(this.text.substr(0, 20));
            }
            /* use \x20 instead of ' ' to keep this space during js2h.sh processing */
            this.label.innerText = this.num + (text.length ? ":\x20" + text.join("\x20") : "");
        };
    }

    function clear() {
        var i, j, doc, e;
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
    }

    function create() {
        var count = 0;

        function helper(win, offsets) {
            /* document may be undefined for frames out of the same origin */
            /* policy and will break the whole code - so we check this before */
            if (typeof win.document == "undefined") {
                return;
            }

            offsets        = offsets || {left: 0, right: 0, top: 0, bottom: 0};
            offsets.right  = win.innerWidth  - offsets.right;
            offsets.bottom = win.innerHeight - offsets.bottom;

            /* checks if given elemente is in viewport and visible */
            function isVisible(e) {
                if (typeof e == "undefined") {
                    return false;
                }
                var rect = e.getBoundingClientRect();
                if (!rect ||
                    rect.top >= offsets.bottom || rect.bottom <= offsets.top ||
                    rect.left >= offsets.right || rect.right <= offsets.left
                ) {
                    return false;
                }

                if ((!rect.width || !rect.height) && (e.textContent || !e.name)) {
                    var arr   = Array.prototype.slice.call(e.childNodes);
                    var check = function(e) {
                        return e instanceof Element
                            && e.style.float != "none"
                            && isVisible(e);
                    };
                    if (!arr.some(check)) {
                        return false;
                    }
                }

                var s = win.getComputedStyle(e, null);
                return s.display !== "none" && s.visibility == "visible";
            }

            var doc       = win.document,
                res       = xpath(doc, config.xpath),
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
                rect  = e.getClientRects()[0];
                label = labelTmpl.cloneNode(false);
                label.setAttribute(
                    "style", [
                        "display:none;",
                        "left:", Math.max((rect.left + offsetX), offsetX), "px;",
                        "top:", Math.max((rect.top + offsetY), offsetY), "px;"
                    ].join("")
                );

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
                    e:         e,
                    label:     label,
                    text:      text,
                    showText:  showText,
                    __proto__: new Hint
                });

                if (count >= config.maxHints) {
                    break;
                }
            }

            /* append the fragment to the document */
            var hDiv = doc.createElement("div");
            hDiv.id  = cId;
            hDiv.setAttribute("vimbhint", "container");
            hDiv.appendChild(fragment);
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
            for (i = 0; i < win.frames.length; i++) {
                var rect,
                    f = win.frames[i],
                    e = f.frameElement;

                if (isVisible(e)) {
                    rect = e.getBoundingClientRect();
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

    function show(fireLast) {
        var i, hint, newIdx,
            n       = 1,
            matcher = getMatcher(filterText),
            str     = getHintString(filterNum);

        if (config.hintNumSameLength) {
            /* get number of hints to be shown */
            var hintCount = 0;
            for (i = 0; i < hints.length; i++) {
                if (matcher(hints[i].text)) {
                    hintCount++;
                }
            }
            /* increase starting point of hint numbers until there are */
            /* enough available numbers */
            var len = config.hintKeys.length;
            while (n * (len - 1) < hintCount) {
                n *= len;
            }
        }

        /* clear the array of valid hints */
        validHints = [];
        for (i = 0; i < hints.length; i++) {
            hint = hints[i];
            /* hide hints not matching the filter text */
            if (!matcher(hint.text)) {
                hint.hide();
            } else {
                /* assign the new hint number/letters as label to the hint */
                hint.num = getHintString(n++);
                /* check for number filter */
                if (!filterNum || 0 === hint.num.indexOf(str)) {
                    hint.show();
                    validHints.push(hint);
                } else {
                    hint.hide();
                }
            }
        }
        if (fireLast && config.followLast && validHints.length <= 1) {
            focusHint(0);
            return fire();
        }

        /* if the previous active hint isn't valid set focus to first */
        if (!activeHint || validHints.indexOf(activeHint) < 0) {
            return focusHint(0);
        }
    }

    /* Returns a validator method to check if the hint elements text matches */
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

    /* Retrun the hint string for a given number based on configured hintkeys */
    function getHintString(n) {
        var res = [],
            len = config.hintKeys.length;
        do {
            res.push(config.hintKeys[n % len]);
            n = Math.floor(n / len);
        } while (n > 0);

        return res.reverse().join("");
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
        /* HINT_CSS is replaces by the contents of the HINT_CSS constant from config.h */
        e.innerHTML = "HINT_CSS";
        doc.head.appendChild(e);
        /* prevent us from adding the style multiple times */
        doc.hasStyle = true;
    }

    function focus(back) {
        var idx = validHints.indexOf(activeHint);
        /* previous active hint not found */
        if (idx < 0) {
            idx = 0;
        }

        if (back) {
            if (--idx < 0) {
                idx = validHints.length - 1;
            }
        } else {
            if (++idx >= validHints.length) {
                idx = 0;
            }
        }
        return focusHint(idx);
    }

    function fire() {
        if (!activeHint) {
            return "ERROR:";
        }

        var e = activeHint.e,
            res;

        /* process form actions like focus toggling inputs */
        if (config.handleForm) {
            res = handleForm(e);
        }

        if (config.keepOpen) {
            /* reset the filter number */
            filterNum = 0;
            show(false);
        } else {
            clear();
        }

        return res || config.action(e);
    }

    /* focus or toggle form fields */
    function handleForm(e) {
        var tag  = e.nodeName.toLowerCase(),
            type = e.type || "";

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
        /* to open links in new window the mouse events are fired with ctrl */
        /* key - otherwise some ugly pages will ignore this attribute in their */
        /* mouse event observers like duckduckgo */
        click(e, newWin);
        e.target = oldTarget;
    }

    /* set focus on hint with given index valid hints array */
    function focusHint(newIdx) {
        /* reset previous focused hint */
        if (activeHint) {
            activeHint.e.classList.remove(fClass);
            activeHint.label.classList.remove(fClass);
            mouseEvent(activeHint.e, "mouseout");
        }
        /* get the new active hint */
        if ((activeHint = validHints[newIdx])) {
            activeHint.e.classList.add(fClass);
            activeHint.label.classList.add(fClass);
            mouseEvent(activeHint.e, "mouseover");

            return "OVER:" + getSrc(activeHint.e);;
        }
    }

    function click(e, ctrl) {
        mouseEvent(e, "mouseover", ctrl);
        mouseEvent(e, "mousedown", ctrl);
        mouseEvent(e, "mouseup", ctrl);
        mouseEvent(e, "click", ctrl);
    }

    function mouseEvent(e, name, ctrl) {
        var evObj = e.ownerDocument.createEvent("MouseEvents");
        evObj.initMouseEvent(
            name, true, true, e.ownerDocument.defaultView,
            0, 0, 0, 0, 0,
            (typeof ctrl != "undefined") ? ctrl : false, false, false, false, 0, null
        );
        e.dispatchEvent(evObj);
    }

    /* retrieves the url of given element */
    function getSrc(e) {
        return e.href || e.src || "";
    }

    function xpath(doc, expr) {
        return doc.evaluate(
            expr, doc, function (p) {return "http://www.w3.org/1999/xhtml";},
            XPathResult.ORDERED_NODE_SNAPSHOT_TYPE, null
        );
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
        init: function init(mode, keepOpen, maxHints, hintKeys, followLast, hintNumSameLength) {
            var prop,
                /* holds the xpaths for the different modes */
                xpathmap = {
                    otY:     "//*[@href] | //*[@onclick or @tabindex or @class='lk' or @role='link' or @role='button'] | //input[not(@type='hidden' or @disabled or @readonly)] | //textarea[not(@disabled or @readonly)] | //button | //select",
                    e:       "//input[not(@type) or @type='text'] | //textarea",
                    iI:      "//img[@src]",
                    OpPsTxy: "//*[@href] | //img[@src and not(ancestor::a)] | //iframe[@src]"
                },
                /* holds the actions to perform on hint fire */
                actionmap = {
                    o:          function(e) {open(e, false); return "DONE:";},
                    t:          function(e) {open(e, true); return "DONE:";},
                    eiIOpPsTxy: function(e) {return "DATA:" + getSrc(e);},
                    Y:          function(e) {return "DATA:" + (e.textContent || "");}
                };

            config = {
                maxHints:   maxHints,
                keepOpen:   keepOpen,
                /* handle forms only useful when there are form fields in xpath */
                /* don't handle form for Y to allow to yank form filed content */
                /* instead of switching to input mode */
                handleForm:        ("eot".indexOf(mode) >= 0),
                hintKeys:          hintKeys,
                followLast:        followLast,
                hintNumSameLength: hintNumSameLength,
            };
            for (prop in xpathmap) {
                if (prop.indexOf(mode) >= 0) {
                    config["xpath"] = xpathmap[prop];
                    break;
                }
            }
            for (prop in actionmap) {
                if (prop.indexOf(mode) >= 0) {
                    config["action"] = actionmap[prop];
                    break;
                }
            }

            create();
            return show(true);
        },
        filter: function filter(text) {
            /* remove previously set number filters to make the filter */
            /* easier to understand for the users */
            filterNum  = 0;
            filterText = text || "";
            return show(true);
        },
        update: function update(n) {
            var pos,
                keys = config.hintKeys;
            /* delete last filter number digit */
            if (null === n && filterNum) {
                filterNum = Math.floor(filterNum / keys.length);
                return show(false);
            }
            if ((pos = keys.indexOf(n)) >= 0) {
                filterNum = filterNum * keys.length + pos;
                return show(true);
            }
            return "ERROR:";
        },
        clear:        clear,
        fire:         fire,
        focus:        focus,
    };
})());
