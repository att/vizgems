
function leftybegin {
    print "$instance.instance = \"$instance\";"
    print "$instance.tools = \"$tools\";"
}

function leftyentry {
    typeset -n var=$1.$2.entries.$3.var
    typeset -n val=$1.$2.entries.$3.val
    typeset -n type=$1.$2.entries.$3.type
    typeset prefix

    prefix=$instance.${1#sui}
    print "$prefix$var = \c"
    case $type in
    coord)
        print "['x' = ${val%%\ *}; 'y' = ${val##*\ };];"
        ;;
    2colors)
        print "[0 = \"${val%%\ *}\"; 1 = \"${val##*\ }\";];"
        ;;
    yesno)
        if [[ $val == yes ]] then
            print "1;"
        else
            print "0;"
        fi
        ;;
    text|fontname|bgcolor|fgcolor)
        print "\"$val\";"
        ;;
    fontsize)
        print "${val%pt};"
        ;;
    *)
        print "\"$val\";"
        ;;
    esac
}

function leftyentrylist {
    :
}

function leftyend {
    :
}

function javabegin {
    print "<param name=instance value=\"$instance\">"
    print "<param name=tools value=\"$tools\">"
}

function javaentry {
    typeset -n var=$1.$2.entries.$3.var
    typeset -n val=$1.$2.entries.$3.val
    typeset -n type=$1.$2.entries.$3.type
    typeset prefix

    prefix=${1#sui}
    print "<param name=$prefix$var value=\c"
    case $type in
    coord)
        print "\"${val%%\ *},${val##*\ }\">"
        ;;
    2colors)
        print "\"${val%%\ *},${val##*\ }\">"
        ;;
    yesno)
        if [[ $val == yes ]] then
            print "1>"
        else
            print "0>"
        fi
        ;;
    text|fontname|bgcolor|fgcolor)
        print "\"$val\">"
        ;;
    fontsize)
        print "${val%pt}>"
        ;;
    *)
        print "\"$val\">"
        ;;
    esac
}

function javaentrylist {
    :
}

function javaend {
    :
}

function kshbegin {
    print "instance=$instance"
    print "tools=\"$tools\""
}

function kshentry {
    typeset -n var=$1.$2.entries.$3.var
    typeset -n val=$1.$2.entries.$3.val
    typeset -n type=$1.$2.entries.$3.type

    if [[ ${prefixes[${instance}]} == '' ]] then
        print "${instance}=''"
        prefixes[${instance}]=1
    fi
    if [[ ${prefixes[${instance}_${1#sui}]} == '' ]] then
        print "${instance}.${1#sui}=''"
        prefixes[${instance}_${1#sui}]=1
    fi
    if [[ ${prefixes[${instance}_${1#sui}_$2]} == '' ]] then
        print "${instance}.${1#sui}.$2=''"
        prefixes[${instance}_${1#sui}_$2]=1
    fi
    print "${instance}.${1#sui}.$2.$var=\c"
    case $type in
    coord|2colors|text|fontname|bgcolor|fgcolor)
        printf "%q\n" "${val}"
        ;;
    yesno)
        if [[ $val == yes ]] then
            print "1"
        else
            print "0"
        fi
        ;;
    fontsize)
        printf "%q\n" "${val%pt}pt"
        ;;
    *)
        printf "%q\n" "$val"
        ;;
    esac
}

function kshentrylist {
    typeset tool=$1 group=$2 var=$3
    if [[ ${prefixes[${instance}_${1#sui}]} == '' ]] then
        print "${instance}.${1#sui}=''"
        prefixes[${instance}_${1#sui}]=1
    fi
    if [[ ${prefixes[${instance}_${1#sui}_$2]} == '' ]] then
        print "${instance}.${1#sui}.$2=''"
        prefixes[${instance}_${1#sui}_$2]=1
    fi
    shift 3
    typeset val="$@"
    print "${instance}.${tool#sui}.$group.$var=\"$val\""
}

function kshend {
    :
}

typeset -A suffixmap
suffixmap[lefty]=lefty
suffixmap[java]=java
suffixmap[ksh]=sh

force=n
while (( $# > 0 )) do
    case $1 in
    -f)
        force=y
        shift
        ;;
    -oprefix)
        oprefix=$2
        shift 2
        ;;
    -prefname)
        prefname=$2
        shift 2
        ;;
    *)
        break
        ;;
    esac
done

instance=$SWIFTINSTANCE

uptodate=y
for tool in $SWIFTTOOLS; do
    for dir in ${PATH//:/ }; do
        file=$dir/../lib/prefs/$tool.prefs
        if [[ -f $file ]] then
            for lang in ksh; do
                if [[ $file -nt $oprefix.${suffixmap[$lang]} ]] then
                    uptodate=n
                fi
            done
            break
        fi
    done
done
for dir in ${PATH//:/ }; do
    file=$dir/../lib/prefs/$instance.prefs
    if [[ -f $file ]] then
        for lang in ksh; do
            if [[ $file -nt $oprefix.${suffixmap[$lang]} ]] then
                uptodate=n
            fi
        done
        break
    fi
done
if [[ $SUIPREFGENSYSTEMFILE != '' ]] then
    if [[ -f $SUIPREFGENSYSTEMFILE ]] then
        file=$SUIPREFGENSYSTEMFILE
        for lang in ksh; do
            if [[ $file -nt $oprefix.${suffixmap[$lang]} ]] then
                uptodate=n
            fi
        done
    fi
fi
if [[ $SUIPREFGENUSERFILE != '' ]] then
    if [[ -f $SUIPREFGENUSERFILE ]] then
        file=$SUIPREFGENUSERFILE
        for lang in ksh; do
            if [[ $file -nt $oprefix.${suffixmap[$lang]} ]] then
                uptodate=n
            fi
        done
    fi
else
    if [[ -f $SWMDIR/$instance.$prefname.prefs ]] then
        file=$SWMDIR/$instance.$prefname.prefs
        for lang in ksh; do
            if [[ $file -nt $oprefix.${suffixmap[$lang]} ]] then
                uptodate=n
            fi
        done
    fi
fi

if [[ $force != y && $uptodate == y ]] then
    for file in $oprefix.*; do
        [[ ! -f $file ]] && continue
        touch $file
    done
    exit 0
fi

for tool in $SWIFTTOOLS; do
    for dir in ${PATH//:/ }; do
        if [[ -f $dir/../lib/prefs/$tool.prefs ]] then
            . $dir/../lib/prefs/$tool.prefs
            break
        fi
    done
done
for dir in ${PATH//:/ }; do
    if [[ -f $dir/../lib/prefs/$instance.prefs ]] then
        . $dir/../lib/prefs/$instance.prefs
        break
    fi
done
if [[ $SUIPREFGENSYSTEMFILE != '' ]] then
    if [[ -f $SUIPREFGENSYSTEMFILE ]] then
        . $SUIPREFGENSYSTEMFILE
    fi
fi
if [[ $SUIPREFGENUSERFILE != '' ]] then
    if [[ -f $SUIPREFGENUSERFILE ]] then
        . $SUIPREFGENUSERFILE
    fi
else
    if [[ -f $SWMDIR/$instance.$prefname.prefs ]] then
        . $SWMDIR/$instance.$prefname.prefs
    fi
fi

tools=
for tool in $SWIFTTOOLS; do
    typeset -n x=$tool
    yn=${x.launch}
    uselang=${x.uselang}
    case $yn in
    yes) tools="$tools $tool:$uselang" ;;
    *) tools="$tools $tool:nolaunch" ;;
    esac
done
typeset -A alllangs
for tool in $tools; do
    typeset -n x=${tool%%:*}
    if [[ "${!x.@}" == '' ]] then
        print -u2 "ERROR tool $tool has no preferences"
        exit 1
    fi
    for lang in ${x.langs}; do
        alllangs[$lang]=1
    done
done
tmp=tmp.$$
for lang in "${!alllangs[@]}"; do
    eval ${lang}begin > $oprefix.${suffixmap[$lang]}.$tmp
done

typeset -A prefixes
for tool in $tools; do
    typeset -n x=${tool%%:*}
    langs=${x.langs}
    for group in ${x.groups}; do
        typeset -n y=${tool%%:*}.$group
        for entry in ${y.entrylist}; do
            for lang in $langs; do
                ${lang}entry ${tool%%:*} $group $entry \
                >> $oprefix.${suffixmap[$lang]}.$tmp
            done
        done
        for lang in $langs; do
            ${lang}entrylist ${tool%%:*} $group \
                _entrylist ${y.entrylist} \
            >> $oprefix.${suffixmap[$lang]}.$tmp
        done
        for lang in $langs; do
            ${lang}entrylist ${tool%%:*} $group \
                _changedentrylist ${y.changedentrylist} \
            >> $oprefix.${suffixmap[$lang]}.$tmp
        done
    done
done

for lang in "${!alllangs[@]}"; do
    eval ${lang}end >> $oprefix.${suffixmap[$lang]}.$tmp
done

for lang in "${!alllangs[@]}"; do
    mv $oprefix.${suffixmap[$lang]}.$tmp $oprefix.${suffixmap[$lang]}
done
