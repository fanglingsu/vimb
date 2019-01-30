/* This script is injected into all frames. */
var Vimb = {};

Vimb.applyFocusChangeObservers = (pageId, serializer) => {
    let doc         = document;
    let changeFocus = () => {
        /* Do not concern about focus changes on body element. It seems that
         * this will fire some blur events on switching from one input to
         * another within iframes. */
        if (doc.activeElement && !(doc.activeElement instanceof HTMLBodyElement)) {
            window.webkit.messageHandlers.focus.postMessage(
                serializer(pageId, Vimb.isElementEditable(doc.activeElement))
            );
        }
    };

    /* Check for current active element. */
    changeFocus();
    doc.addEventListener('focus', changeFocus, true);
    doc.addEventListener('blur', changeFocus, true);
};

Vimb.isElementEditable = (e) => {
    let name = e.tagName.toLowerCase();
    return name === "textarea"
        || (name === "input" && /^(?:text|email|number|search|tel|url|password)$/i.test(e.type))
        || "true" === e.contentEditable;
};

Vimb.focusInput = () => {
    let i, e, expr = "//textarea|//input[not(@type)" +
        " or translate(@type,'ETX','etx')='text'" +
        " or translate(@type,'ADOPRSW','adoprsw')='password'" +
        " or translate(@type,'CLOR','clor')='color'" +
        " or translate(@type,'ADET','adet')='date'" +
        " or translate(@type,'ADEIMT','adeimt')='datetime'" +
        " or translate(@type,'ACDEILMOT','acdeilmot')='datetime-local'" +
        " or translate(@type,'AEILM','aeilm')='email'" +
        " or translate(@type,'HMNOT','hmnot')='month'" +
        " or translate(@type,'BEMNRU','bemnru')='number'" +
        " or translate(@type,'ACEHRS','acehrs')='search'" +
        " or translate(@type,'ELT','elt')='tel'" +
        " or translate(@type,'EIMT','eimt')='time'" +
        " or translate(@type,'LRU','lru')='url'" +
        " or translate(@type,'EKW','ekw')='week'" +
        "]";
    let res = document.evaluate(expr, document, null, XPathResult.ORDERED_NODE_SNAPSHOT_TYPE, null);
    for (i = 0; i < res.snapshotLength; i++) {
        e = res.snapshotItem(i);
        if (!!(e.offsetWidth || e.offsetHeight || e.getClientRects().length)) {
            e.focus();
            return;
        }
    }
};
