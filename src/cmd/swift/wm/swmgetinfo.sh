
export SWMROOT=${SWMROOT:-${DOCUMENT_ROOT%/htdocs}}

id=${REMOTE_USER:-$SWMIDOVERRIDE}

if [[ $id == '' && $SWMWSWMACCESS == y && $HTTP_COOKIE == *attSWMAUTH* ]] then
    code=${HTTP_COOKIE##*attSWMAUTH=}
    code=${code%%\;*}
    suffix=${code//[!A-Za-z0-9_]/''}
    cachefile=$SWMROOT/tmp/$REMOTE_ADDR.${suffix:0:32}
    now_ts=$(printf '%(%#)T')
    if [[ $code != '' && -f $cachefile ]] then
        command . $cachefile 2> /dev/null
        if [[ $cached_code == $code ]] then
            if (( cached_ts > now_ts - SWMWSWMIDLE || SWMWSWMIDLE == 0 )) then
                id=$cached_id
            else
                rm $cachefile
                unset cachefile
            fi
        else
            rm $cachefile
            unset cachefile
        fi
    fi
    if [[ $id == '' ]] then
        authurl="/cgi-bin/vg_swmaccess.cgi?url=$(printf '%#H' "$REQUEST_URI")"
        print "Content-type: text/html\n"
        print '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 TRANSITIONAL//EN">'
        print "<html>"
        print "<head></head>"
        print "<body>"
        print "<script>"
        print "location.href = '$authurl'"
        print "</script>"
        print "</body>"
        print "</html>"
        exit 0
    else
        if [[ $cachefile != '' && $SWMNOUPDATE != y ]] then
            printf 'cached_ts=%q\ncached_code=%q\ncached_id=%q\n' \
                "$now_ts" "$code" "$id" \
            > $cachefile.$$ && mv $cachefile.$$ $cachefile 2> /dev/null
        fi
    fi
fi

if [[ $id == '' && $SWMWEBIGACCESS == y && $HTTP_COOKIE == *ECCUser* ]] then
    code=${HTTP_COOKIE##*ECCUser=}
    code=${code%%\'*}
    code=${code%%\;*}
    code=${code##*:}
    curl "$SWMWEBIGURL/getuserprofile.jsp?userID=$(printf '%#H' "$code")" \
        2> /dev/null \
    | egrep '^dn:.*companyID=' | read cid
    cid=${cid##*companyID=}
    cid=${cid%%,*}
    id=$(egrep ":$cid\$" $SWMROOT/.cookie | tail -1 | sed 's/:.*$//')
    if [[ $id == '' ]] then
        print -u2 ERROR cannot find user id mapped to EBIG company id $cid
    fi
fi

if [[ $id == '' && $SWMWBDACCESS == y && $HTTP_COOKIE == *SWIFT_CODE* ]] then
    code=${HTTP_COOKIE##*SWIFT_CODE=}
    code=${code%%\'*}
    code=${code%%\;*}
    id=$(egrep ":$code\$" $SWMROOT/.cookie | tail -1 | sed 's/:.*$//')
    if [[ $id == '' ]] then
        print -u2 ERROR cannot find user id mapped to BD id $code
    fi
fi

if [[ $id == '' && $SWMWCLOUDACCESS == y && $HTTP_COOKIE == *iPlanetDirectoryPro* ]] then
    code=${HTTP_COOKIE##*iPlanetDirectoryPro=}
    code=${code%%\;*}
    code=${code//'"'/}
    suffix=${code//[!A-Za-z0-9_]/''}
    cachefile=$SWMROOT/tmp/$REMOTE_ADDR.${suffix:0:16}
    now_ts=$(printf '%(%#)T')
    if [[ $code != '' && -f $cachefile ]] then
        command . $cachefile 2> /dev/null
        if (( cached_ts > now_ts - 30 )) && [[ $cached_code == $code ]] then
            id=$cached_id
            touch $cachefile
            unset cachefile
        else
            rm $cachefile
        fi
    fi
    if [[ $id == '' && $code != '' ]] then
        code=${code//@([\'\\])/'\'\1}
        eval "code=\$'${code//'%'@(??)/'\x'\1"'\$'"}'"
        curl -u "$SWMWCLOUDUSER" -H 'Accept: application/json' \
            -k "$SWMWCLOUDURL?token=$(printf '%#H' "$code")" 2> /dev/null \
        | read line
        cid=${line##*companyId'":"'}
        cid=${cid%%'"'*}
        id=$(egrep "^cust_$cid:" $SWMROOT/.passwd | tail -1 | sed 's/:.*$//')
    fi
    if [[ $id != '' ]] then
        if [[ $cachefile != '' && $SWMNOUPDATE != y ]] then
            printf 'cached_ts=%q\ncached_code=%q\ncached_id=%q\n' \
                "$now_ts" "$code" "$id" \
            > $cachefile.$$ && mv $cachefile.$$ $cachefile 2> /dev/null
        fi
    fi
fi

if [[ $id == '' && $SWMWATTEACCESS == y && $HTTP_COOKIE == *attESHr* ]] then
    code=${HTTP_COOKIE##*attESSec=}
    code=${code%%\;*}
    suffix=${code//[!A-Za-z0-9_]/''}
    cachefile=$SWMROOT/tmp/$REMOTE_ADDR.${suffix:0:16}
    now_ts=$(printf '%(%#)T')
    if [[ $code != '' && -f $cachefile ]] then
        command . $cachefile 2> /dev/null
        if (( cached_ts > now_ts - 30 )) && [[ $cached_code == $code ]] then
            id=$cached_id
            touch $cachefile
            unset cachefile
        else
            rm $cachefile
        fi
    fi
    if [[ $id == '' && $code != '' ]] then
        code=${code//'+'/' '}
        code=${code//@([\'\\])/'\'\1}
        eval "code=\$'${code//'%'@(??)/'\x'\1"'\$'"}'"
        java \
            -classpath $SWMROOT/etc/gateKeeper.jar:$SWMROOT/etc/PSE_Lite.jar \
            esGateKeeper.CmdGateKeeper "$code" "CSP" "PROD" \
        | read cspid
        cspid=${cspid%%'|PROD|'*}
        cspid=${cspid##*'|'}
        id=$(egrep "^$cspid:" $SWMROOT/.passwd | tail -1 | sed 's/:.*$//')
    fi
    if [[ $id == '' ]] then
        authurl='https://www.e-access.att.com/empsvcs/hrpinmgt/pagLogin'
        if [[ $HTTPS == on ]] then
            desturl=https://$HTTP_HOST
        else
            desturl=http://$HTTP_HOST
        fi
        desturl+="/$REQUEST_URI"
        print "Content-type: text/html\n"
        print '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 TRANSITIONAL//EN">'
        print "<html>"
        print "<head></head>"
        print "<body>"
        print "<script>"
        print "s = '$authurl?retURL=' + escape('$desturl') + '&sysName=VizGEMS'"
        print "location.href = s"
        print "</script>"
        print "</body>"
        print "</html>"
        exit 0
    else
        if [[ $cachefile != '' && $SWMNOUPDATE != y ]] then
            printf 'cached_ts=%q\ncached_code=%q\ncached_id=%q\n' \
                "$now_ts" "$code" "$id" \
            > $cachefile.$$ && mv $cachefile.$$ $cachefile 2> /dev/null
        fi
    fi
fi

accline=$(egrep "^$id;" $SWMROOT/accounts/current)
if [[
    $id == '' || "$(egrep ^$id: $SWMROOT/.passwd)" == '' || $accline == ''
]] then
    print "Content-type: text/html\n"
    print '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 TRANSITIONAL//EN">'
    print "<html><head><title>SWIFT</title></head><body>"
    print "Sorry, your account name is not recognized"
    print "</body></html>"
    exit 1
fi

export SWMID=$id
rest=${accline#*';'}
name=${rest%%';'*}
rest=${rest#*';'}
export SWMNAME=$name
export SWMINFO=$rest

export SWMGROUPS
while read line; do
    set -A list $line
    for (( i = 1; i < ${#list[@]}; i++ )) do
        if [[ ${list[$i]} == $SWMID ]] then
            SWMGROUPS="$(print $SWMGROUPS ${list[0]%:})"
            eval SWMGROUP_${list[0]%:}=1
        fi
    done
done < $SWMROOT/.group

export SWMDIR=$SWMROOT/htdocs/members/$SWMID
export SWMDIR_REL=/members/$SWMID

if [[ ! -d $SWMDIR ]] then
    mkdir $SWMDIR
    if [[ ! -d $SWMDIR ]] then
        print "Content-type: text/html\n"
        print '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 TRANSITIONAL//EN">'
        print "<html><head><title>SWIFT</title></head><body>"
        print "Sorry, your private directory does not exist"
        print "and cannot be created"
        print "</body></html>"
        exit 1
    fi
fi
