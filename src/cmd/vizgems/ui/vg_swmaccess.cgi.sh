#!/bin/ksh

export SWMROOT=${DOCUMENT_ROOT%/htdocs}
. $SWMROOT/bin/swmenv
[[ $KSHREC != 1 && ${KSH_VERSION##*' '} < $SHELLVERSION ]] && \
KSHREC=1 exec $SHELL $0 "$@"

argv0=${0##*/}
instance=${argv0%%_*}
rest=${argv0#$instance}
rest=${rest#_}
if [[ $instance == '' || $rest == '' ]] then
    print -u2 SWIFT ERROR program name not in correct form
    exit 1
fi
. $SWMROOT/proj/$instance/bin/${instance}_env || exit 1

auth_data=(
    pid=''
    bannermenu='list|help|userguide:authpage|Help|documentation'
)

function auth_init {
    auth_data.pid=$pid

    return 0
}

function auth_emit_header_begin {
    print '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 TRANSITIONAL//EN">'
    print "<html>"
    print "<head>"

    ph_emitheader page banner bdiv
    print "<title>${ph_data.title}</title>"

    return 0
}

function auth_emit_header_end {
    print "</head>"

    return 0
}

function auth_emit_body_begin {
    typeset vt st img

    print "<body>"
    ph_emitbodyheader ${auth_data.pid} banner 1600

    return 0
}

function auth_emit_body_end {
    ph_emitbodyfooter
    print "</body>"
    print "</html>"

    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    PS4='${.sh.fun}-$LINENO: '
    typeset -ft auth_init
    typeset -ft auth_emit_body_begin auth_emit_body_end
    typeset -ft auth_emit_header_begin auth_emit_header_end
fi

. vg_hdr
. vg_prefshelper
. $SWIFTDATADIR/dpfiles/config.sh

suireadcgikv

typeset ill='+(@(\<|%3c)@([a-z][a-z0-9]|a)*|\`*\`|\$*\(*\)|\$*\{*\})'

if [[ $qs_mode == logout ]] then
    url=${REQUEST_URI%%[?&]*}
    if [[ $url == *$ill* ]] then
        print -r -u2 "SWIFT ERROR swmaccess: illegal characters in REQUEST_URI"
        exit 1
    fi
    print "Content-type: text/html"
    print "Cache-Control: no-cache, no-store, must-revalidate"
    print "Pragma: no-cache"
    print "Expires: 0"
    print "Set-Cookie: attSWMAUTH=; path=/; SameSite=Strict\n"
    print '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 TRANSITIONAL//EN">'
    print "<html>"
    print "<head>"
    print "<meta http-equiv='refresh' content='0; URL=$url' />"
    print "</head>"
    print "</html>"
    exit 0
fi

if [[ $SWMWSWMACCESS != y ]] then
    exit 0
fi

remoteid=$REMOTE_ADDR
if [[ $HTTP_X_FORWARDED_FOR != '' ]] then
    remoteid=$HTTP_X_FORWARDED_FOR
fi

authfailed=n
if [[ $qs_mode == @(auth|showcookie) ]] then
    if [[ $qs_user != +([a-zA-Z0-9_-]) || $qs_pass == '' ]] then
        exit 1
    fi
    line=$(egrep "^$qs_user:" $SWMROOT/.passwd)
    user=${line%%':'*}
    pass=${line#*':'}
    if [[ $pass == '' ]] then
        authfailed=y
    else
        ok=n
        if [[ $pass == '$apr1$'* ]] then
            salt=${pass#'$apr1$'}; salt=${salt%%'$'*}
            if [[ $(
                openssl passwd -apr1 -salt "$salt" "$qs_pass"
            ) == $pass ]] then
                ok=y
            fi
        else
            if swmcheckpasswd "$qs_pass" "$pass"; then
                ok=y
            fi
        fi
        if [[ $ok == y ]] then
            code=$(uuidgen)
            suffix=${code//[!A-Za-z0-9_]/''}
            cachefile=$SWMROOT/tmp/$REMOTE_ADDR.${suffix:0:32}
            if [[ $HTTP_X_FORWARDED_FOR != '' ]] then
                cachefile=$SWMROOT/tmp/$HTTP_X_FORWARDED_FOR.${suffix:0:32}
            fi
            if [[ $qs_globalcookie == y && -f $SWMROOT/globalcookieok ]] then
                line=$(egrep "^vg_att_admin:" $SWMROOT/.group)
                if [[ $line == *' '$user' '* || $line == *' '$user ]] then
                    cachefile=$SWMROOT/tmp/0.0.0.0.${suffix:0:32}
                fi
            fi
            now_ts=$(printf '%(%#)T')
            printf 'cached_ts=%q\ncached_code=%q\ncached_id=%q\n' \
                "$now_ts" "$code" "$user" \
            > $cachefile.$$ && mv $cachefile.$$ $cachefile
            if [[ $qs_mode == showcookie ]] then
                print "Content-type: text/plain"
                print "Cache-Control: no-cache, no-store, must-revalidate"
                print "Pragma: no-cache"
                print "Expires: 0"
                print "Set-Cookie: attSWMAUTH=$code; path=/; SameSite=Strict\n"
                print "attSWMAUTH=$code"
                exit 0
            fi
            print "Content-type: text/html"
            print "Cache-Control: no-cache, no-store, must-revalidate"
            print "Pragma: no-cache"
            print "Expires: 0"
            print "Set-Cookie: attSWMAUTH=$code; path=/; SameSite=Strict\n"
            print '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 TRANSITIONAL//EN">'
            print "<html>"
            if [[ $QUERY_STRING == *after_auth=* ]] then
                url=${QUERY_STRING##*after_auth=}
                url=${url%%'&'*}
            elif [[ $REDIRECT_URL != '' ]] then
                url=$REDIRECT_URL
            elif [[ $qs_url != '' ]] then
                url=$qs_url
            else
                url=/cgi-bin-vg-members/vg_home.cgi
            fi
            if [[ $url == *$ill* ]] then
                print -r -u2 "SWIFT ERROR swmaccess: illegal characters in url"
                exit 1
            fi
            if [[ $url != /cgi-* || $url == *$'\n'* ]] then
                print -u2 ERROR illegal url $url
                exit 1
            fi
            print "<head>"
            print "<meta http-equiv='refresh' content='0; URL=$url' />"
            print "</head>"
            print "</html>"
            rm -f $SWMROOT/tmp/badauth.$user.*
            rm -f $SWMROOT/tmp/badauth.$remoteid.*
            exit 0
        else
            authfailed=y
        fi
    fi
fi

if [[ $authfailed == n ]] then
    rm -f $SWMROOT/tmp/badauth.$user.*
    rm -f $SWMROOT/tmp/badauth.$remoteid.*
else
    if [[ $user != '' ]] then
        badie=$user
    else
        badie=$remoteid
    fi
    fail=toomany
    for (( i = 0; i < 5; i++ )); do
        if [[ ! -f $SWMROOT/tmp/badauth.$badie.$i ]] then
            touch $SWMROOT/tmp/badauth.$badie.$i
            fail=some
            break
        fi
    done
    if [[ $fail == toomany ]] then
        print -u2 more than 5 login failures by ${user:-$remoteid}
        sleep 60
        rm $SWMROOT/tmp/badauth.$badie.*
    fi
fi

tabind=1

print "Content-type: text/html"
print "Status: 404 Not Found"
print "Cache-Control: no-cache, no-store, must-revalidate"
print "Pragma: no-cache"
print "Expires: 0\n"

export SWMID=__no_user__
pid=default
ph_init $pid "${auth_data.bannermenu}"

auth_init
auth_emit_header_begin

if [[ $QUERY_STRING == *after_auth=* ]] then
    url=${QUERY_STRING##*after_auth=}
    url=${url%%'&'*}
elif [[ $REDIRECT_URL != '' ]] then
    url="$REDIRECT_URL?$REDIRECT_QUERY_STRING"
elif [[ $qs_url != '' ]] then
    url=$qs_url
else
    url=/cgi-bin-vg-members/vg_home.cgi
fi
if [[ $url == *$ill* ]] then
    print -r -u2 "SWIFT ERROR swmaccess: illegal characters in url"
    exit 1
fi

helpercgi="/cgi-bin/vg_swmaccess.cgi"
if [[ 1 == 1 ]] then
    print "<form"
    print "  id=doauth method=post enctype='multipart/form-data'"
    print "  action='$helpercgi'"
    print ">"
    print "<input type=hidden name=mode value=auth>"
    print "<input type=hidden id=auth_user name=user value=''>"
    print "<input type=hidden id=auth_pass name=pass value=''>"
    print "<input type=hidden name=url value='$url'>"
    print "</form>"
    print "<script>"
    print "var auth_helper = '$helpercgi'"
    print "function auth_login () {"
    print "  var userel = document.getElementById ('auth_user')"
    print "  var passel = document.getElementById ('auth_pass')"
    print "  var formel = document.getElementById ('doauth')"
    print "  userel.value = auth_userel.value"
    print "  passel.value = auth_passel.value"
    print "  formel.submit ()"
    print "}"
    print "</script>"
else
    print "<script>"
    print "var auth_helper = '$helpercgi'"
    print "function auth_login () {"
    print "  var url"
    print "  if ("
    print "    auth_userel.value.length == 0 || auth_passel.value.length == 0"
    print "  ) {"
    print "    alert ('please fill out both fields first')"
    print "    return"
    print "  }"
    print "  url = auth_helper + '?mode=auth'"
    print "  url += '&user=' + auth_userel.value"
    print "  url += '&pass=' + auth_passel.value"
    print "  url += '&url=$url'"
    print "  location = url"
    print "}"
    print "</script>"
fi
print "<script>"
print "function auth_checkandrun (e) {"
print "  if (e.keyCode != 13)"
print "    return false"
print "  if (auth_userel.value.length > 0 && auth_passel.value.length > 0)"
print "    auth_login ()"
print "}"
print "function auth_clearfields () {"
print "  auth_userel.value = ''"
print "  auth_passel.value = ''"
print "}"
print "function auth_gotoatteaccess (u) {"
print "  var s = 'https://www.e-access.att.com/empsvcs/hrpinmgt/pagLogin'"
print "  s += '?retURL=' + escape (u) + '&sysName=VizGEMS'"
print "  document.cookie = "
print "    'attSWMAUTH=; path=/; expires=Sat, 1 Jan 2000 00:00:00'"
print "  location.href = s"
print "}"
print "</script>"
auth_emit_header_end

auth_emit_body_begin

print "<div class=page>${ph_data.title}</div>"
if [[ $authfailed == y ]] then
    print "<div class=page>"
    print "<font color=red>credentials not recognized, please try again</font>"
    print "</div>"
fi
print "<div class=page>Log in to access your account</div>"

print "<div class=page><table class=page>"

print "<td class=pageborder colspan=2>&nbsp;<a"
print "  class=pageil href='javascript:void(0)' onClick='auth_login()'"
print "  title='sign in using the credentials below'"
print ">&nbsp;Log in&nbsp;</a>&nbsp;<a"
print "  class=pageil href='javascript:void(0)' onClick='auth_clearfields()'"
print "  title='clear all fields'"
print ">&nbsp;Clear&nbsp;</a>&nbsp;"
if [[ $SHOWHELP == 1 ]] then
    print "<a"
    print "  class=pageil href='javascript:void(0)'"
    print "  onClick='vg_showhelp(\"quicktips/authform\")'"
    print "  title='click to read about using this form'"
    print ">&nbsp;?&nbsp;</a>"
fi
print "</td>"

print "<tr class=page>"
print "<td class=pageborder style='text-align:right'>"
print "<a"
print "  class=pagelabel href='javascript:void(0)'"
print "  title='user id, 3-12 characters'"
print ">&nbsp;User ID&nbsp;</a>"
if [[ $SHOWHELP == 1 ]] then
    print "<a"
    print "  class=pageil href='javascript:void(0)'"
    print "  onClick='vg_showhelp(\"quicktips/authuser\")'"
    print "  title='click to read about user ids'"
    print ">&nbsp;?&nbsp;</a>"
fi
print "</td>"
print "<td class=pageborder style='text-align:left'>"
print "&nbsp;<input"
print "  tabindex=$tabind class=page type=text id=mainuser size=20"
print "  title='user id, 3-12 characters' onKeyUp='auth_checkandrun(event)'"
print ">"
(( tabind++ ))
print "</td>"
print "</tr>"
print "<tr class=page>"
print "<td class=pageborder style='text-align:right'>"
print "<a"
print "  class=pagelabel href='javascript:void(0)'"
print "  title='password, 4-20 characters'"
print ">&nbsp;Password&nbsp;</a>"
if [[ $SHOWHELP == 1 ]] then
    print "<a"
    print "  class=pageil href='javascript:void(0)'"
    print "  onClick='vg_showhelp(\"quicktips/authpass\")'"
    print "  title='click to read about passwords'"
    print ">&nbsp;?&nbsp;</a>"
fi
print "</td>"
print "<td class=pageborder style='text-align:left'>"
print "&nbsp;<input"
print "  tabindex=$tabind class=page type=password id=mainpass size=20"
print "  title='password, 4-20 characters' onKeyUp='auth_checkandrun(event)'"
print ">"
(( tabind++ ))
print "</td>"
print "</tr>"

if [[ $SWMWATTEACCESS == y ]] then
    if [[ $HTTPS == on ]] then
        s+="https://$HTTP_HOST$url"
    else
        s+="http://$HTTP_HOST$url"
    fi
    print "<tr class=page>"
    print "<td colspan=2 class=pageborder style='text-align:center'>"
    print "<a class=pageil href='javascript:auth_gotoatteaccess(\"$s\")'>"
    print "  or, click here to use the AT&T Global Sign-on"
    print "</a>"
    print "</td>"
    print "</tr>"
fi

print "</table></div>"

print "<script>"
print "var auth_userel = document.getElementById ('mainuser')"
print "var auth_passel = document.getElementById ('mainpass')"
print "auth_clearfields ()"
print "auth_userel.focus ()"
print "</script>"

auth_emit_body_end
