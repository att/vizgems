#!/bin/ksh

from=$1
to=$2
subject=$3
notes=$4

tostr="${to//[ ,;]/, }"
toarg="${to//[,;]/ }"

if [[ -x /usr/lib/sendmail ]] then
    sm=/usr/lib/sendmail
elif [[ -x /usr/sbin/sendmail ]] then
    sm=/usr/sbin/sendmail
else
    print -u2 ERROR cannot locate sendmail program
    exit 1
fi

dt=$(printf '%(%F)T')

for ti in $toarg; do
    (
        print "From $from $dt"
        print "From: $from"
        print "X-Mailer: mailx"
        print "Mime-Version: 1.0"
        print "Content-Type: multipart/alternative; boundary=\"__VG_ALT_VG__\""
        print "Date: $(printf '%(%a, %d %b %Y %H:%M:%S %z)T')"
        print "To: $tostr"
        print "Subject: $subject"
        print ""
        print "This is a multi-part message in MIME format."
        print -- "--__VG_ALT_VG__"
        print "Content-Type: text/plain; charset=ISO-8859-1; format=flowed"
        print "Content-Transfer-Encoding: 7bit"
        print ""
        print "Email from $SWMNAME ($SWMID)"
        print "Date $dt"
        print "Notes $notes"
        print ""
        print "If you see this message, your mail reader does not"
        print "handle HTML."
        print ""
        print ""
        print -- "--__VG_ALT_VG__"
        print "Content-Type: multipart/related; boundary=\"__VG__HTML__VG__\""
        print ""
        print ""
        print -- "--__VG__HTML__VG__"
        print "Content-Type: text/html; charset=ISO-8859-1"
        print "Content-Transfer-Encoding: 7bit"
        print ""

        cidn=0
        skip=n
        while read -r line; do
            case $line in
            '<!--BEGIN SKIP-->'*)
                skip=y
                ;;
            '<!--END SKIP-->'*)
                skip=n
                continue
                ;;
            esac
            [[ $skip == y ]] && continue

            case $line in
            '<!--EMAIL TABLE-->'*)
                print "<div align=center><table>"
                print "<tr>"
                print "<td align=right><b>From</b></td>"
                print "<td>$SWMNAME ($SWMID)</td>"
                print "</tr>"
                print "<tr>"
                print "<td align=right><b>Date</b></td><td>$dt</td>"
                print "</tr>"
                print "<tr>"
                print "<td align=right><b>Notes</b></td><td>$notes</td>"
                print "</tr>"
                print "</table></div>"
                ;;
            '<'img*vg_dq_imghelper*'>')
                file=${line#*name=}
                file=${file%%'&'*}
                file=${file%%[\ \'\"]*}
                file=${file##*/}
                cids[$cidn]=$file
                paths[$cidn]=$file
                print "<img src=cid:$file>"
                (( cidn++ ))
                ;;
            '<'img*'>')
                file=${line#*src=}
                file=${file%%['>' ]*}
                file=${file//[\'\"]/}
                paths[$cidn]=$SWMROOT/htdocs/${file#$SWIFTWEBPREFIX/}
                file=${file##*/}
                cids[$cidn]=$file
                print "<img src=cid:$file>"
                (( cidn++ ))
                ;;
            *href=[\'\"]*[\'\"]*)
                if [[ $line != *javascript:void* ]] then
                    href=${line#*href=}
                    href=${href%%[\ \>]*}
                    line=${line//"$href"/"'javascript:void(0)'"}
                fi
                print -r "$line"
                ;;
            *)
                print -r "$line"
                ;;
            esac
        done < rep.out

        print ""

        for (( cidi = 0; cidi < cidn; cidi++ )) do
            print -- "--__VG__HTML__VG__"
            print "Content-Type: image/gif; name=\"${cids[$cidi]}\""
            print "Content-Transfer-Encoding: base64"
            print "Content-ID: <${cids[$cidi]}>"
            print "Content-Disposition: inline; filename=\"${cids[$cidi]}\""
            print ""
            uuencode -x mime < ${paths[$cidi]} | egrep -v '^begin|^====$'
        done

        print -- "--__VG__HTML__VG__--"
        print ""
        print -- "--__VG_ALT_VG__--"
        print ""
        print ""
    ) | $sm "$ti"
done
