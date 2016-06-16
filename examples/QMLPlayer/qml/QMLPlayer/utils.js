
var kItemWidth = scaled(60)
var kItemHeight = scaled(30)
var kMargin = scaled(8)
var kFontSize = scaled(16)
var kSpacing = scaled(4)

// "/xxx" will be resolved as qrc:///xxx. while "xxx" is "qrc:///QMLDIR/xxx
var resprefix = Qt.resolvedUrl(" ").substring(0, 4) == "qrc:" ? "/" : ""

function resurl(s) { //why called twice if in qrc?
    return resprefix + s
}

String.prototype.startsWith = function(s) {
    return this.indexOf(s) === 0;
};

String.prototype.endsWith = function(suffix) {
    return this.indexOf(suffix, this.length - suffix.length) !== -1;
};

function fileName(path) {
    return path.substring(path.lastIndexOf("/") + 1)
}

function msec2string(t) {
    t = Math.floor(t/1000)
    var ss = t%60
    t = (t-ss)/60
    var mm = t%60
    var hh = (t-mm)/60
    if (ss < 10)
        ss = "0" + ss
    if (mm < 10)
        mm = "0" + mm
    if (hh < 10)
        hh = "0" + hh
    return hh + ":" + mm +":" + ss
}

function scaled(x) {
    //console.log("scaleRatio: " + scaleRatio + "; " + x + ">>>" + x*scaleRatio);
    return x * scaleRatio;
}

function htmlEscaped(s) {
    if (!s) {
        return '';
    }
    var escaped = '';
    var namedHtml = {
        '38': '&amp;',
        '60': '&lt;',
        '62': '&gt;',
        '34': '&quot;',
        '160': '&nbsp;',
        '162': '&cent;',
        '163': '&pound;',
        '164': '&curren;',
        '169': '&copy;',
        '174': '&reg;',
    };
    var wasNewLine = 0;
    for (var i = 0, il = s.length; i < il; ++i) {
        var c = s.charCodeAt(i);
        var es = namedHtml[c];
        if (typeof es !== 'undefined') {
            wasNewLine = 0;
            escaped += es;
        } else {
            if (c === 13 || c === 10) {
                if (wasNewLine == 0)
                    escaped += '<br>';
                wasNewLine++;
            } else {
                wasNewLine = 0;
                escaped += String.fromCharCode(c);
            }
        }
    }
    return escaped;
}
