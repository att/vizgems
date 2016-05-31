
file=$1
field=$2
shift 2

for dir in ${PATH//:/ }; do
    if [[ -f $dir/../lib/vg/vg_filelist ]] then
        configfile=$dir/../lib/vg/vg_filelist
        break
    fi
done
. $configfile
assetdbfile=${files.assets.location}

if [[ ${files[@]} == '' ]] then
    print -u2 SWIFT ERROR missing configuration file
    exit 1
fi

typeset -n fdata=files.$file
typeset -n fielddata=files.$file.fields.$field

print "<script>"
print "function set (t) {"
print "  opener.document.forms[0]._${fielddata.name}.value = t"
print "}"
print "</script>"

print "<form>"
print "<input type=button name=_close value=Close onClick='top.close ()'>"

case ${fielddata.name} in
customer)
    print "<br><select"
    print "name=_list size=10"
    print "onChange='set (this.options[this.selectedIndex].value)'"
    print ">"
    ddsfilter -osm none -fe '{
        DROP;
        if (strcmp (level, "c") == 0 && strcmp (key, "name") == 0)
            sfprintf (sfstdout, "%s %s\n", id, val);
    }' $SWIFTDATADIR/data/main/latest/processed/total/inv-nodes.dds \
    | while read -r id name; do
        print -r "<option value='$id - $name'>$id - $name"
    done
    print "</select>"
    ;;
object)
    print "<br><select"
    print "name=_list size=10"
    print "onChange='set (this.options[this.selectedIndex].value)'"
    print ">"
    while read -u3 -r asset; do
        print "<option value='$asset'>$asset"
    done 3< $assetdbfile
    print "</select>"
    ;;
vgasset)
    print "<br><select"
    print "name=_list size=10"
    print "onChange='set (this.options[this.selectedIndex].value)'"
    print ">"
    for i in $SWIFTDATADIR/dpfiles/inv/view/*-inv.txt; do
        cat $i
    done | egrep ^node.o | sed -e 's/^node.o.//' -e 's/|.*$//' | sort -u \
    | while read -r asset; do
        print "<option value='$asset'>$asset"
    done
    print "</select>"
    ;;
vgswport)
    print "<br><select"
    print "name=_list size=10"
    print "onChange='set (this.options[this.selectedIndex].value)'"
    print ">"
    id=
    for i in "$@"; do
        if [[ $i == field_vgasset=* ]] then
            id=${i#*=}
            break
        fi
    done
    [[ $id == '' ]] && id=___NOID___
    for i in $SWIFTDATADIR/dpfiles/inv/scope/auto-*-inv.txt; do
        cat $i
    done | egrep "^node\|o\|$id.*\|(si|scopeinv)_(i|atm|frame)descr" \
    | sed \
        -e 's/^.*si_idescr//' -e 's/^.*si_atmdescr//' \
        -e 's/^.*si_framedescr//' \
        -e 's/^.*scopeinv_idescr//' -e 's/^.*scopeinv_atmdescr//' \
        -e 's/^.*scopeinv_framedescr//' -e 's/|aid=.*$//' \
    | sort -u \
    | while read -r idnname; do
        print "<option value='${idnname#*'|'} (${idnname%'|'*})'>${idnname#*'|'}"
    done
    print "</select>"
    ;;
vgswspeed)
    print "<br><select"
    print "name=_list size=10"
    print "onChange='set (this.options[this.selectedIndex].value)'"
    print ">"
    id=
    for i in "$@"; do
        if [[ $i == field_vgasset=* ]] then
            id=${i#*=}
            break
        fi
    done
    port=
    for i in "$@"; do
        if [[ $i == field_vgswport=* ]] then
            port=${i#*=}
            break
        fi
    done
    port=${port/*'('/}
    port=${port/')'*/}
    [[ $id == '' ]] && id=___NOID___
    [[ $port == '' ]] && port=___NOID___
    for i in $SWIFTDATADIR/dpfiles/inv/scope/auto-*-inv.txt; do
        cat $i
    done | egrep "^node\|o\|$id.*\|(si|scopeinv)_(i|atm|frame)speed$port" \
    | sed \
        -e 's/^.*si_ispeed//' -e 's/^.*si_atmspeed//' \
        -e 's/^.*si_framespeed//' \
        -e 's/^.*scopeinv_ispeed//' -e 's/^.*scopeinv_atmspeed//' \
        -e 's/^.*scopeinv_framespeed//' -e 's/|aid=.*$//' \
    | sort -u \
    | while read -r speed; do
        print "<option value='${speed#*'|'}'>${speed#*'|'}"
    done
    print "</select>"
    ;;
starttime)
    print "<br><select"
    print "name=_list size=12"
    print "onChange='set (this.options[this.selectedIndex].value)'"
    print ">"
    for t in now '1 hour' '2 hours' '3 hours'; do
        tt=$(printf '%(%H:%M)T' "$t")
        print "<option value='$tt'>$t"
    done
    for ((t = 0; t < 24; t++)) do
        tt=$(printf '%02d:00' $t)
        print "<option value='$tt'>at $tt"
    done
    print "</select>"
    ;;
endtime)
    print "<br><select"
    print "name=_list size=12"
    print "onChange='set (this.options[this.selectedIndex].value)'"
    print ">"
    for t in '1 hour' '2 hours' '3 hours'; do
        tt=$(printf '%(%H:%M)T' "$t")
        print "<option value='$tt'>$t"
    done
    for ((t = 0; t < 24; t++)) do
        tt=$(printf '%02d:00' $t)
        print "<option value='$tt'>at $tt"
    done
    print "<option value='23:59'>at 23:59"
    print "</select>"
    ;;
startdate)
    print "<br><select"
    print "name=_list size=12"
    print "onChange='set (this.options[this.selectedIndex].value)'"
    print ">"
    for t in today '1 day' '2 days' '3 days' '7 days'; do
        tt=$(printf '%(%Y.%m.%d %a)T' "$t")
        print "<option value='$tt'>$t"
    done
    today=$(printf '%(%#)T' today)
    ((dsec = 24 * 60 * 60))
    for ((t = 0; t <= 30; t++)) do
        tt=$(printf '%(%Y.%m.%d %a)T' \#$((today + dsec * t + dsec / 12)))
        print "<option value='$tt'>$tt"
    done
    print "</select>"
    ;;
enddate)
    print "<br><select"
    print "name=_list size=12"
    print "onChange='set (this.options[this.selectedIndex].value)'"
    print ">"
    for t in today '1 day' '2 days' '3 days' '7 days'; do
        tt=$(printf '%(%Y.%m.%d %a)T' "$t")
        print "<option value='$tt'>$t"
    done
    today=$(printf '%(%#)T' today)
    ((dsec = 24 * 60 * 60))
    for ((t = 0; t <= 30; t++)) do
        tt=$(printf '%(%Y.%m.%d %a)T' \#$((today + dsec * t + dsec / 12)))
        print "<option value='$tt'>$tt"
    done
    print "<option value='indefinitely'>indefinitely"
    print "</select>"
    ;;
esac
print "</form>"
print "<b>Help</b>"
print "<p>Click on an entry to transfer its value to the field in the main form"

print "</html>"
