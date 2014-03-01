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
