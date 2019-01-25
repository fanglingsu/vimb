var Vimb = {};

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
