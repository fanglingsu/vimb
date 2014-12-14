/* Script to insert various data into form fields.
 * Given data is an array of "Selector:Value" tupel.
 * ["selector:value","...:..."]
 * Example call from within vimb:
 * ::e! _vbform.fill(["input[name='login']:daniel","input[name='password']:p45w0rD"]);
 *
 * Note the double ':' in front, this tells vimb not to write the command into
 * history or the last excmd register ":. */
"use strict"
var _vbform = {
    fill: function(data) {
        data = data||[];
        var e, i, k, s;

        // iterate over data array and try to find the elements
        for (i = 0; i < data.length; i++) {
            // split into selector and value part
            s = data[i].split(":");
            e = document.querySelectorAll(s[0]);
            for (k = 0; k < e.length; k++) {
                // fill the form fields
                this.set(e[k], s[1]);
            }
        }
    },

    // set the value for the form element
    set: function(e, v) {
        var i, t, n = e.nodeName.toLowerCase();

        if (n == "input") {
            t = e.type;
            if (!t || t == "text" || t == "password") {
                e.value = v;
            } else if (t == "checkbox" || t == "radio") {
                e.checked = ("1" == v) ? true : false;
            } else if (t == "submit") {
                e.click();
            }
        } else if (n == "textarea") {
            e.value = v;
        } else if (n == "select") {
            for (i = 0; i < e.options.length; i++) {
                if (e.options[i].value == v) {
                    e.options[i].selected = "selected";
                }
            }
        }
    }
};
