
export SWMROOT=${DOCUMENT_ROOT%/htdocs}

export PATH=$PATH:$SWMROOT/bin

pfile=$SWMROOT/.passwd
gfile=$SWMROOT/.group
ifile=$SWMROOT/accounts/current
mdir=$SWMROOT/htdocs/members

lockfile=$SWMROOT/lock.accounts
trap 'rm -f ${lockfile} $$pfile.one' HUP QUIT TERM KILL EXIT

set -o noclobber
while ! command exec 3> ${lockfile}; do
    if kill -0 $(< ${lockfile}); then
        sleep 5
        continue
    elif [[ $(fuser ${lockfile}) != '' ]] then
        sleep 5
        continue
    else
        rm ${lockfile}
        continue
    fi
done 2> /dev/null
print -u3 $$
set +o noclobber

if [[ -f $pfile.nxt || -f $gfile.nxt || -f $ifile.nxt ]] then
    if [[ -f $pfile.new ]] then
        mv $pfile.new $pfile.nxt
    fi
    if [[ -f $gfile.new ]] then
        mv $gfile.new $gfile.nxt
    fi
    if [[ -f $ifile.new ]] then
        mv $ifile.new $ifile.nxt
    fi
    if [[ -f $pfile.nxt ]] then
        mv $pfile.nxt $pfile
    fi
    if [[ -f $gfile.nxt ]] then
        mv $gfile.nxt $gfile
    fi
    if [[ -f $ifile.nxt ]] then
        mv $ifile.nxt $ifile
    fi
fi

case $1 in
add)
    id=$2
    pass=$3
    if [[ $id == '' ]] then
        print -u2 ERROR empty account id
        exit 10
    fi
    if [[ $(egrep "^$id:" $pfile) != '' ]] then
        print -u2 ERROR account $id already exists
        mkdir -p $mdir/$id
        exit 10
    fi
    if [[ $pass == '' ]] then
        print -u2 ERROR empty password
        exit 10
    fi
    > $pfile.one
    htpasswd -d -b $pfile.one $id "$pass" 2>&1 \
    | egrep -v 'Adding|Updating|Deleting'
    pass=$(< $pfile.one)
    pass=${pass#*:}
    groups=:${4// /}:
    name=${5//\;/_}
    attr=${6//\;/_}
    (
        egrep -v "^$id:" $pfile
        print "$id:$pass"
    ) | sort -u > $pfile.new
    (
        while read gr list; do
            gr=${gr%:}
            list2=" $list "
            if [[ $list2 == *\ $id\ * ]] then
                list2=${list2// $id / }
                list2=${list2# }
                list2=${list2% }
                print $gr: $list2
            else
                print $gr: $list
            fi
        done < $gfile
    ) > $gfile.new2
    (
        typeset -A grmap
        while read gr list; do
            gr=${gr%:}
            grmap[$gr]=$gr
            if [[ $groups == *:$gr:* ]] then
                print $gr: $list $id
            else
                print $gr: $list
            fi
        done < $gfile.new2
        for gr in ${groups//:/ }; do
            if [[ ${grmap[$gr]} == "" ]] then
                print $gr: $id
            fi
        done
    ) | sort -u > $gfile.new
    rm $gfile.new2
    (
        egrep -v "^$id;" $ifile
        print "$id;$name;$attr"
    ) | sort -u > $ifile.new
    date=$(date -f %Y.%m.%d)
    cp $pfile $pfile.$date
    cp $gfile $gfile.$date
    cp $ifile $ifile.$date
    mv $pfile.new $pfile.nxt
    mv $gfile.new $gfile.nxt
    mv $ifile.new $ifile.nxt
    mv $pfile.nxt $pfile
    mv $gfile.nxt $gfile
    mv $ifile.nxt $ifile
    mkdir -p $mdir/$id
    ;;
del)
    id=$2
    if [[ $id == '' ]] then
        print -u2 ERROR empty account id
        exit 10
    fi
    if [[ $(egrep "^$id:" $pfile) == '' ]] then
        print -u2 WARNING account $id does not exist
    fi
    (
        egrep -v "^$id:" $pfile
    ) | sort -u > $pfile.new
    (
        while read gr list; do
            gr=${gr%:}
            list2=" $list "
            if [[ $list2 == *\ $id\ * ]] then
                list2=${list2// $id / }
                list2=${list2# }
                list2=${list2% }
                print $gr: $list2
            else
                print $gr: $list
            fi
        done < $gfile
    ) | sort -u > $gfile.new
    (
        egrep -v "^$id;" $ifile
    ) | sort -u > $ifile.new
    date=$(date -f %Y.%m.%d)
    [[ -d $mdir/$id ]] && rm -r $mdir/$id
    cp $pfile $pfile.$date
    cp $gfile $gfile.$date
    cp $ifile $ifile.$date
    mv $pfile.new $pfile.nxt
    mv $gfile.new $gfile.nxt
    mv $ifile.new $ifile.nxt
    mv $pfile.nxt $pfile
    mv $gfile.nxt $gfile
    mv $ifile.nxt $ifile
    ;;
mod)
    id=$2
    pass=$3
    if [[ $id == '' ]] then
        print -u2 ERROR empty account id
        exit 10
    fi
    if [[ $(egrep "^$id:" $pfile) == '' ]] then
        print -u2 ERROR account $id does not exist
        exit 10
    fi
    if [[ $4 == '' && $5 == "" && $6 == "" ]] then
        if [[ $pass != '' ]] then
            htpasswd -d -b $pfile $id "$pass" 2>&1 \
            | egrep -v 'Adding|Updating|Deleting'
        fi
        exit 0
    fi
    if [[ $pass == "" ]] then
        egrep "^$id:" $pfile > $pfile.one
    else
        > $pfile.one
        htpasswd -d -b $pfile.one $id "$pass" 2>&1 \
        | egrep -v 'Adding|Updating|Deleting'
    fi
    pass=$(< $pfile.one)
    pass=${pass#*:}
    groups=:${4// /}:
    name=${5//\;/_}
    attr=${6//\;/_}
    if ! egrep "^$id:" $pfile > /dev/null; then
        print -u2 ERROR account $id does not exist
        exit 10
    fi
    (
        egrep -v "^$id:" $pfile
        print "$id:$pass"
    ) | sort -u > $pfile.new
    (
        while read gr list; do
            gr=${gr%:}
            list2=" $list "
            if [[ $list2 == *\ $id\ * ]] then
                list2=${list2// $id / }
                list2=${list2# }
                list2=${list2% }
                print $gr: $list2
            else
                print $gr: $list
            fi
        done < $gfile
    ) > $gfile.new2
    (
        typeset -A grmap
        while read gr list; do
            gr=${gr%:}
            grmap[$gr]=$gr
            if [[ $groups == *:$gr:* ]] then
                print $gr: $list $id
            else
                print $gr: $list
            fi
        done < $gfile.new2
        for gr in ${groups//:/ }; do
            if [[ ${grmap[$gr]} == "" ]] then
                print $gr: $id
            fi
        done
    ) | sort -u > $gfile.new
    rm $gfile.new2
    (
        egrep -v "^$id;" $ifile
        print "$id;$name;$attr"
    ) | sort -u > $ifile.new
    date=$(date -f %Y.%m.%d)
    cp $pfile $pfile.$date
    cp $gfile $gfile.$date
    cp $ifile $ifile.$date
    mv $pfile.new $pfile.nxt
    mv $gfile.new $gfile.nxt
    mv $ifile.new $ifile.nxt
    mv $pfile.nxt $pfile
    mv $gfile.nxt $gfile
    mv $ifile.nxt $ifile
    ;;
esac
