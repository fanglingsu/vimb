var hints = Object.freeze((function(){
    'use strict';

    var hints      = [],   /* holds all hint data (hinted element, label, number) in view port */
        docs       = [],   /* hold the affected document with the start and end index of the hints */
        validHints = [],   /* holds the valid hinted elements matching the filter condition */
        activeHint,        /* holds the active hint object */
        filterText = "",   /* holds the typed text filter */
        filterKeys = "",   /* holds the typed hint-keys filter */
        attr = "vimbhint",
        config;
    /* the hint class used to maintain hinted element and labels */
    function Hint() {
        var state = "";
        /* hide hint label and remove coloring from hinted element */
        this.hide = function() {
            var l = this.label,
                e = this.e;
            /* remove hint labels from no more visible hints */
            l.style.display = "none";
            l.setAttribute(attr, "");
            e.setAttribute(attr, "");
            state = "hidden";
        };

        /* marks the element and label of a hint as focused */
        this.focus = function() {
            this.label.setAttribute(attr, "label focus");
            this.e.setAttribute(attr, "hint focus");
            state = "focus";
        };

        /* remove focus mark from hint and label */
        this.unfocus = function() {
            /* do not unfocus hidden hints */
            if (state != "hidden") {
                this.label.setAttribute(attr, "label");
                this.e.setAttribute(attr, "hint");
                state = "visible";
            }
        };

        /* show the hint element colored with the hint label */
        this.show = function() {
            var e = this.e,
                l = this.label,
                text = [];
            if (state != "focus") {
                l.style.display = "";
                l.setAttribute(attr, "label");
                e.setAttribute(attr, "hint");
                state = "visible";
            }

            /* create the label with the hint number/letters */
            if (e instanceof HTMLInputElement) {
                var type = e.type;
                if (type === "checkbox") {
                    text.push(e.checked ? "☑" : "☐");
                } else if (type === "radio") {
                    text.push(e.checked ? "⊙" : "○");
                }
            }
            if (this.showText && this.text) {
                text.push(this.text.substr(0, 20));
            }
            /* use \x20 instead of ' ' to keep this space during js2h.sh processing */
            l.innerText = this.num + (text.length ? ":\x20" + text.join("\x20") : "");
        };
    }

    function onresize() {
        clear(false);
        create();
        show(false);
    }

    function clear(removeListener) {
        var i, j, doc, e, w = window;
        if (removeListener && w) {
            w.removeEventListener("resize", onresize, true);
            w.removeEventListener("scroll", onresize, false);
            for (i = 0; i < w.frames.length; i++) {
                try {
                    w.frames[i].frameElement.contentDocument.removeEventListener("scroll", onresize, false);
                } catch (ex) {
                }
            }
        }
        for (i = 0; i < docs.length; i++) {
            doc = docs[i];
            /* find all hinted elements vimbhint 'hint' */
            var res = xpath(doc.doc, "//*[@vimbhint]");
            for (j = 0; j < res.snapshotLength; j++) {
                e = res.snapshotItem(j);
                e.removeAttribute(attr);
            }
            doc.div.parentNode.removeChild(doc.div);
        }
        docs       = [];
        hints      = [];
        validHints = [];
        filterText = "";
        filterKeys = "";
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

            labelTmpl.setAttribute(attr, "label");

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

                /* create the hint label with number/letters */
                rect  = e.getClientRects()[0];
                label = labelTmpl.cloneNode(false);

                label.style.display = "none";
                label.style.left    = Math.max(rect.left - 4, 0) + "px";
                label.style.top     = Math.max(rect.top - 4, 0) + "px";

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
                e.setAttribute(attr, "hint");

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
            hDiv.setAttribute(attr, "container");
            hDiv.style.position = "fixed";
            hDiv.style.top      = "0";
            hDiv.style.left     = "0";
            hDiv.style.zIndex   = "225000";
            hDiv.appendChild(fragment);
            if (doc.body) {
                doc.body.appendChild(hDiv);
            }
            docs.push({
                doc:   doc,
                start: start,
                end:   hints.length - 1,
                div:   hDiv
            });

            /* recurse into any iframe or frame element */
            for (i = 0; i < win.frames.length; i++) {
                try {
                    var rect, f = win.frames[i], e = f.frameElement;
                } catch (ex) {
                    continue;
                }

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
            matcher = getMatcher(filterText);

        var hintCount  = 0,
            candidates = [];

        /* Check which hints match to the filter. */
        for (i = 0; i < hints.length; i++) {
            hint = hints[i];
            /* hide hints not matching the text filter */
            if (!matcher(hint.text)) {
                hint.hide();
            } else {
                hintCount++;
                candidates.push(hint);
            }
        }

        /* clear the array of valid hints */
        validHints = [];
        /* Now we can assigne the hint labels and check if hose match. */
        var labeler = config.getHintLabeler(hintCount);
        for (i = 0; i < candidates.length; i++) {
            hint = candidates[i];
            /* assign the new hint number/letters as label to the hint */
            hint.num = labeler();
            /* check for hint-keys filter */
            if (!filterKeys.length || hint.num.indexOf(filterKeys) == 0) {
                hint.show();
                validHints.push(hint);
            } else {
                hint.hide();
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
    /* the given text filter. */
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
            /* reset the hint-keys filter */
            filterKeys = "";
            show(false);
        } else {
            clear(true);
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
                e.click();
                return "DONE:";
            }
            if (type === "submit" || type === "reset" || type  === "button" || type === "image") {
                e.click();
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
    function open(e) {
        /* We call click() in async mode to return as fast as possible. If
         * we don't return immediately, the EvalJS dbus call will probably
         * timeout and cause errors. */
        window.setTimeout(function() {
            var href;
            if ((href = e.getAttribute('href')) && href != '#') {
                window.location.href = href;
            } else {
                e.click();
            }
        }, 0);
    }

    /* set focus on hint with given index valid hints array */
    function focusHint(newIdx) {
        /* reset previous focused hint */
        if (activeHint) {
            activeHint.unfocus();
            mouseEvent(activeHint.e, "mouseout");
        }
        /* get the new active hint */
        if ((activeHint = validHints[newIdx])) {
            var e = activeHint.e;
            activeHint.focus();
            mouseEvent(e, "mouseover");

            return "OVER:" + (e instanceof HTMLImageElement ? "I:" : "A:") + getSrc(e);
        }
    }

    function mouseEvent(e, name) {
        var evObj = e.ownerDocument.createEvent("MouseEvents");
        evObj.initMouseEvent(
            name, true, true, e.ownerDocument.defaultView,
            0, 0, 0, 0, 0,
            false, false, false, false, 0, null
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
    function _labeler(keys, sameLength) {
        var kl  = keys.length,
            /* Avoid don't consider the hint keys to be numeric in case the */
            /* hintKeys='0' to avoid endless loop by attempt to use next */
            /* hintKey char later. */
            num = (keys[0] == '0' && kl > 1) ? 1 : 0,
            sl  = sameLength;

        return function (count) {
            /* if hint keys starts with '0' count from 1 instead of 0 */
            var hcount = num,
                offset = 0;

            function getMaxHintOfLen(len) {
                return (len > 0) ? ((kl - num) ** len) : 0;
            }

            /* We can generate same length label if there is only one hint key */
            /* except in case there is only one hint. But we don't need to */
            /* handle this. */
            if (sl && kl > 1) {
                if (num) {
                    offset = 1;
                    /* increase starting point of hint numbers until there are */
                    /* enough available numbers */
                    while (offset * (kl - 1) < count) {
                        offset *= kl;
                    }
                    offset--;
                } else {
                    var val, labellen = 0, res = 0;
                    /* Find hint string length to describe all hints with same length. */
                    while ((val = getMaxHintOfLen(labellen)) < count) {
                        labellen++;
                        res += val;
                    }
                    /* the offset-th hint string is the first one to use */
                    offset = res;
                }
            }
            return function () {
                /* Start on second hint key in incase of numeric hints. */
                var res = [],
                    n   = hcount + offset;
                do {
                    res.push(keys[n % kl]);
                    n  = ~~(n / kl);
                    if (!num) {
                        n--;
                    }
                } while (n - num >= 0);
                hcount++;

                return res.reverse().join("");
            };
        };
    }

    /* the api */
    return {
        init: function(mode, keepOpen, maxHints, hintKeys, followLast, keysSameLength) {
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
                    ot:         function(e) {open(e); return "DONE:";},
                    eiIOpPsTxy: function(e) {return "DATA:" + getSrc(e);},
                    Y:          function(e) {return "DATA:" + (e.textContent || "");}
                };

            config = {
                maxHints:   maxHints,
                keepOpen:   keepOpen,
                /* handle forms only useful when there are form fields in xpath */
                /* don't handle form for Y to allow to yank form filed content */
                /* instead of switching to input mode */
                handleForm:     ("eot".indexOf(mode) >= 0),
                hintKeys:       hintKeys,
                followLast:     followLast,
                keysSameLength: keysSameLength,
                getHintLabeler: _labeler(hintKeys, keysSameLength)
            };

            for (prop in xpathmap) {
                if (prop.indexOf(mode) >= 0) {
                    config.xpath = xpathmap[prop];
                    break;
                }
            }
            for (prop in actionmap) {
                if (prop.indexOf(mode) >= 0) {
                    config.action = actionmap[prop];
                    break;
                }
            }

            window.addEventListener("resize", onresize, true);
            window.addEventListener("scroll", onresize, false);
            for (var i = 0; i < window.frames.length; i++) {
                try {
                    window.frames[i].frameElement.contentDocument.addEventListener("scroll", onresize, false);
                } catch (ex) {
                }
            }

            create();
            return show(true);
        },
        filter: function(text) {
            /* remove previously set hint-keys filters to make the filter */
            /* easier to understand for the users */
            filterKeys = "";
            filterText = text || "";
            return show(true);
        },
        update: function(n) {
            var pos;
            /* delete last hint-keys filter digit */
            if (null === n && filterKeys.length) {
                filterKeys = filterKeys.slice(0, -1);
                return show(false);
            }
            if ((pos = config.hintKeys.indexOf(n)) >= 0) {
                filterKeys = filterKeys + n;
                return show(true);
            }
            return "ERROR:";
        },
        clear: clear,
        fire:  fire,
        focus: focus,
    };
})());
