#!/bin/ksh

export SWMROOT=${DOCUMENT_ROOT%/htdocs}
. $SWMROOT/bin/swmenv
[[ $KSHREC != 1 && ${KSH_VERSION##*' '} < $SHELLVERSION ]] && \
KSHREC=1 exec $SHELL $0 "$@"
. $SWMROOT/bin/swmgetinfo

argv0=${0##*/}
instance=${argv0%%_*}
rest=${argv0#$instance}
rest=${rest#_}
if [[ $instance == '' || $rest == '' ]] then
    print -u2 SWIFT ERROR program name not in correct form
    exit 1
fi
. $SWMROOT/proj/$instance/bin/${instance}_env || exit 1

fm_data=(
    pid=''
    bannermenu='link|home||Home|reload this home page|link|back||Back|go back one page|list|favorites||Favorites|run a favorite query|list|tools|p:c|Tools|tools|list|help|userguide:homepage|Help|documentation'
)

function fm_init {
    fm_data.pid=$pid

    return 0
}

function fm_emit_header_begin {
    print '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 TRANSITIONAL//EN">'
    print "<html>"
    print "<head>"

    ph_emitheader page banner bdiv
    print "<title>${ph_data.title}</title>"

    return 0
}

function fm_emit_header_end {
    print "</head>"

    return 0
}

function fm_emit_body_begin {
    print "<body>"
    ph_emitbodyheader ${fm_data.pid} banner $qs_winw

    return 0
}

function fm_emit_body_end {
    ph_emitbodyfooter
    print "</body>"
    print "</html>"

    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    PS4='${.sh.fun}-$LINENO: '
    typeset -ft fm_init
    typeset -ft fm_emit_body_begin fm_emit_body_end
    typeset -ft fm_emit_header_begin fm_emit_header_end
fi

. vg_hdr
. vg_prefshelper

suireadcgikv

if [[ $HTTP_USER_AGENT == *MSIE* ]] then
    wname=width
    hname=height
else
    wname=innerWidth
    hname=innerHeight
fi
winattr="directories=no,hotkeys=no,status=yes,resizable=yes,menubar=no"
winattr="$winattr,personalbar=no,titlebar=no,toolbar=no,scrollbars=yes"
st="style='text-align:left'"

pid=$qs_pid
if [[ $pid == '' ]] then
    print -u2 SWIFT ERROR configuration not specified
    exit 1
fi
ph_init $pid "${fm_data.bannermenu}"

print "Content-type: text/html\n"

if ! swmuseringroups 'vg_att_admin|vg_fileedit'; then
    print "<html><body><b><font color=red>"
    print "You are not authorized to access this page"
    print "</font></b></body></html>"
fi

fm_init
fm_emit_header_begin
fm_emit_header_end

fm_emit_body_begin

print "<div class=pagemain>${ph_data.title}</div>"

cgiprefix="vg_filemanager.cgi?pid=$qs_pid&winw=$qs_winw"

for dir in ${PATH//:/ }; do
    if [[ -f $dir/../lib/vg/vg_filelist ]] then
        configfile=$dir/../lib/vg/vg_filelist
        break
    fi
done
. $configfile

if [[ ${files[@]} == '' ]] then
    print -u2 SWIFT ERROR missing configuration file
    exit 1
fi

case $qs_mode in
list)
    print "<table class=page>"
    for file in "${!files.@}"; do
        file=${file#files.}
        file=${file%%.*}
        print $file
    done | sort -u | while read -r file; do
        typeset -n fdata=files.$file
        if [[ ! -f ${fdata.location} ]] then
            continue
        fi
        if ! swmuseringroups "${fdata.group}"; then
            continue
        fi
        print "<tr class=page><td class=pageborder $st>"
        if [[
            ${fdata.accessmode} == byid
        ]] && swmuseringroups "${fdata.admingroup}"; then
            print "<a class=pageil href=${cgiprefix}&mode=show&file=$file>"
            print "  ${fdata.name} (own)"
            print "</a><br>"
            print "<a"
            print "  class=pageil"
            print "  href=${cgiprefix}&mode=show&file=$file&view=all"
            print ">"
            print "  ${fdata.name} (all)"
            print "</a>"
        else
            print "<a class=pageil href=${cgiprefix}&mode=show&file=$file>"
            print "  ${fdata.name}"
            print "</a>"
        fi
        print "</td>"
        print "<td class=pageborder $st>${fdata.summary}</td>"
        print "<td class=pageborder $st>${fdata.description}</td>"
        print "</tr>"
        typeset +n fdata
    done
    print "</table>"
    ;;
show)
    file=$qs_file
    typeset -n fdata=files.$file
    print "<script>"
    print "function launch (helper, file, field) {"
    print "  var ps = ''"
    for field in "${!fdata.fields.@}"; do
        field=${field#files.$file.fields.}
        field=${field%%.*}
        print $field
    done | sort -u | while read -r field; do
        typeset -n fielddata=files.$file.fields.$field
        print "  ps += '&' + getvalue_${fielddata.name} ()"
    done
    print "  window.open ("
    print "    'vg_valuehelper.cgi?pid=$qs_pid&winw=400&helper=' + helper +"
    print "    '&file=' + file + '&field=' + field + ps + '&winw=400',"
    print "    'vg_valuehelper', '$wname=400,$hname=400,$winattr'"
    print "  )"
    print "}"
    print "</script>"
    if ! swmuseringroups "${fdata.group}"; then
        print "<b><font color=red>"
        print "You are not authorized to modify this file"
        print "</font></b>"
        exit
    fi
    if [[
        ${fdata.accessmode} == byid && $qs_view == all
    ]] && swmuseringroups "${fdata.admingroup}"; then
        export VIEWALL=1
    fi

    print "<center><b>Editing ${fdata.summary}</b></center>"

    if [[ ! -f ${fdata.location} ]] then
        print -u2 SWIFT ERROR file not found ${fdata.location}
        print ERROR file not found ${fdata.location}
        exit 1
    fi
    print "<form"
    print "  action=vg_filemanager.cgi"
    print "  method=post enctype='multipart/form-data'"
    print ">"
    print "<table class=page>"
    print "<tr class=page><td class=pageborder $st>"
    print "  <input class=page type=submit name=_action value=Insert>"
    print "  <input class=page type=submit name=_action value=Modify>"
    print "  <input class=page type=submit name=_action value=Remove>"
    print "  <input class=page type=reset>"
    print "  <input type=hidden name=_mode value=edit>"
    print "  <input type=hidden name=_file value=$file>"
    print "  <input type=hidden name=_pid value=$qs_pid>"
    print "  <input type=hidden name=_winw value=$qs_winw>"
    print "</td></tr>"
    print "<tr class=page>"
    print "<td class=pageborder $st><b>Record Fields</b></td>"
    print "</tr>"
    print "<tr class=page><td class=pageborder $st><table class=page>"
    for field in "${!fdata.fields.@}"; do
        field=${field#files.$file.fields.}
        field=${field%%.*}
        print $field
    done | sort -u | while read -r field; do
        typeset -n fielddata=files.$file.fields.$field
        typeset -n qsv=qs_${fielddata.name}
        print "<tr class=page>"
        print "<td class=page>"
        print "<a class=pagelabel title='${fielddata.info}'>"
        print "${fielddata.text}"
        print "</a>"
        print "</td>"
        case ${fielddata.type} in
        choice)
            print "<td class=page><select class=page name=_${fielddata.name}>"
            for (( choicei = 1; ; choicei++ )) do
                typeset -n choice=files.$file.fields.$field.choice$choicei
                if [[ $choice == '' ]] then
                    break
                fi
                sel=
                if [[ $qsv == ${choice%%\|*} ]] then
                    sel=selected
                fi
                print "<option $sel value='${choice%%\|*}'>${choice#*\|}"
            done
            print "</select></td>"
            print "<script>"
            print "function getvalue_${fielddata.name} () {"
            print "  var f, s"
            print "  f = document.forms[0]._${fielddata.name}"
            print "  s = 'field_${fielddata.name}='"
            print "  s += escape (f.options[f.selectedIndex].value)"
            print "  return s"
            print "}"
            print "</script>"
            ;;
        *)
            if [[ ${fielddata.size} != '' ]] then
                size=${fielddata.size}
            else
                size=60
            fi
            val=
            if [[ $qsv != '' ]] then
                val="value='${qsv//__AMP__/\&}'"
            fi
            print "<td class=page>"
            print "<input"
            print "  class=page size=$size $val"
            print "  type=text name=_${fielddata.name}"
            print ">"
            print "</td>"
            print "<script>"
            print "function getvalue_${fielddata.name} () {"
            print "  var f, s"
            print "  f = document.forms[0]._${fielddata.name}"
            print "  s = 'field_${fielddata.name}=' + escape (f.value)"
            print "  return s"
            print "}"
            print "</script>"
            ;;
        esac
        if [[ ${fielddata.helper} != '' ]] then
            print "<td class=page><input"
            print "  type=button name=_${fielddata.name}_helper value=Values"
            print "  class=page onClick=\"launch("
            print "    '${fielddata.helper}', '$file', '$field'"
            print ")\""
            print "></td>"
        fi
        print "</tr>"
        typeset +n fielddata
    done
    print "</table></td></tr>"
    if [[ ${fdata.bulkinsertfield} != '' ]] then
        print "<tr class=page><td class=pageborder $st>"
        print "<b>Bulk Insert Option</b>"
        print "</td></tr>"
        typeset -n fielddata=files.$file.bulkinsertfield
        print "<tr class=page><td class=pageborder $st>"
        print "<a class=pagelabel title='${fielddata.info}'>"
        print "Client ${fielddata.text}"
        print "</a>"
        print "<input class=page type="file" name=cbulkfile value='file'><br>"
        print "<a class=pagelabel title='${fielddata.info}'>"
        print "Server ${fielddata.text}"
        print "</a>"
        print "<input class=page type="text" name=sbulkfile value='' size=80>"
        if [[ ${fielddata.format} != '' || ${fielddata.example} != '' ]] then
            print "<pre>"
            if [[ ${fielddata.format} != '' ]] then
                print "Format: ${fielddata.format}"
            fi
            if [[ ${fielddata.example} != '' ]] then
                print "Example: ${fielddata.example}"
            fi
            print "</pre>"
        fi
        print "</td></tr>"
        typeset +n fielddata
    fi
    print "<tr class=page>"
    print "<td class=pageborder $st><b>File Contents</b></td>"
    print "</tr>"
    print "<tr class=page><td class=pageborder $st>"
    print "<select class=page"
    print "name=_list size=16 multiple"
    print "onChange='selectrec (this.options[this.selectedIndex].value)'"
    print ">"
    $SHELL ${fdata.fileprinter} $configfile $file | while read -u3 -r line; do
        ${fdata.recordsplitter} $configfile $file barsep "$line"
    done 3<&0
    print "</select>"
    print "</td></tr>"
    print "</table>"
    print "</form>"
    print "<script language='JavaScript1.2'>"
    print "function selectrec (value) {"
    print "  v = unescape (value)"
    print "  t = v.split ('|++|')"
    print "  t = t[1].split ('|+|')"
    i=0
    for field in "${!fdata.fields.@}"; do
        field=${field#files.$file.fields.}
        field=${field%%.*}
        print $field
    done | sort -u | while read -r field; do
        typeset -n fielddata=files.$file.fields.$field
        print "  if (typeof (t[$i]) == 'undefined')"
        print "    t[$i] = ''"
        print "  document.forms[0]._${fielddata.name}.value = t[$i]"
        ((i++))
        typeset +n fielddata
    done
    print "}"
    print "</script>"
    print "<b>${fdata.summary}</b>"
    print "<p>${fdata.description}"
    print "<br><br><b>Fields</b>"
    for field in "${!fdata.fields.@}"; do
        field=${field#files.$file.fields.}
        field=${field%%.*}
        print $field
    done | sort -u | while read -r field; do
        typeset -n fielddata=files.$file.fields.$field
        print "<p><b>${fielddata.text}</b><br>${fielddata.info}<br>"
        typeset +n fielddata
    done
    if [[ ${fdata.bulkinsertfield} != '' ]] then
        print "<b>Bulk Insert Option</b>"
        typeset -n fielddata=files.$file.bulkinsertfield
        print "<p><b>${fielddata.text}</b><br>${fielddata.info}<br>"
        typeset +n fielddata
    fi
    print "<br><br><b>Buttons</b>"
    print "<p><b>Insert</b><br>Insert a new record."
    print "Fill out all fields and then click on Insert.<br>"
    print "<p><b>Modify</b><br>Modify a record."
    print "Select a record from the list,"
    print "modify its values and click Modify.<br>"
    print "<p><b>Remove</b><br>Remove records."
    print "Select one or more records from the list"
    print "and then click on Remove.<br>"
    print "<p><b>Reset</b><br>Reset this form.<br>"
    ;;
edit)
    file=$qs_file
    typeset -n fdata=files.$file
    if ! swmuseringroups "${fdata.group}"; then
        print "<b><font color=red>"
        print "You are not authorized to modify this file"
        print "</font></b>"
        exit
    fi
    lpat=
    if [[ ${fdata.accessmode} == byid ]] then
        if [[ $qs_view == all ]] && swmuseringroups "${fdata.admingroup}"; then
            export VIEWALL=1
        fi
        ${fdata.recordchecker} -legalpat | read -r lpat
        [[ $lpat == '' ]] && lpat="___NOPAT___"
    fi

    ext=$(printf '%(%Y%m%d.%H%M%S)T').$$

    print "<center><b>Editing ${fdata.summary}</b></center><br>"

    terrn=
    twarnn=
    typeset -l action=$qs_action
    case $action in
    remove)
        if [[ $qs_cbulkfile != '' || $qs_sbulkfile != '' ]] then
            print "<font color=red>"
            print "WARNING: bulk file is only used for inserting new records"
            print "</font><br>"
            ((twarnn++))
        fi
        for v in "${qs_list[@]}"; do
            rec=${v%%\|++\|*}
            rec=${rec//%27/\'}
            print -r "<b>Record: <pre>$rec</pre></b><br>"
            if [[ $lpat != '' && $rec != $lpat ]] then
                print "<b><font color=red>"
                print "You are not authorized to remove record $rec"
                print "</font></b>"
                break
            fi
            $SHELL ${fdata.fileeditor} $configfile $file $action $ext "" "$rec"
            if [[ $? != 0 ]] then
                print -u2 SWIFT ERROR ${fdata.fileeditor} failed
                print "<font color=red><b>"
                print "ERROR ${fdata.fileeditor} failed to complete"
                print "</b></font><br>"
                ((terrn++))
            fi
        done
        ;;
    insert|modify)
        if [[ $action == modify ]] then
            if [[ $qs_cbulkfile != '' || $qs_sbulkfile != '' ]] then
                print "<font color=red>"
                print "WARNING: bulk file is only used for inserting records"
                print "</font><br>"
                ((twarnn++))
                qs_cbulkfile='' qs_sbulkfile=''
            fi
            if (( ${#qs_list[@]} > 1 )) then
                print "<font color=red><b>"
                print "ERROR multiple records selected for a modify action"
                print "</b></font><br>"
                ((terrn++))
            fi
            orec=${qs_list%%\|++\|*}
            if [[ $lpat != '' && $orec != $lpat ]] then
                print "<b><font color=red>"
                print "ERROR You are not authorized to modify record $orec"
                print "</font></b>"
                ((terrn++))
                qs_list=
            fi
        fi
        if [[ $qs_cbulkfile != '' || $qs_sbulkfile != '' ]] then
            (
                if [[ $qs_cbulkfile != '' ]] then
                    for v in "${qs_cbulkfile[@]}"; do
                        rec=${v%%\|++\|*}
                        print -r -- "$rec"
                    done
                fi
                if [[ $qs_sbulkfile != '' ]] then
                    cat ${qs_sbulkfile}
                fi
            ) | while read -u3 -r rec; do
                rec=${rec//%27/\'}
                print -r "<b>Record: <pre>$rec</pre></b><br>"
                ${fdata.recordchecker} $configfile $file "$rec" \
                | while read -u3 -r line; do
                    case $line in
                    ERROR*)
                        print "<font color=red><b>$line</b></font><br>"
                        ;;
                    WARNING*)
                        print "<font color=red>$line</font><br>"
                        ;;
                    MESSAGE*)
                        print "${line#MESSAGE\ }<br>"
                        ;;
                    rec=*)
                        rec=${line#rec=}
                        ;;
                    errors=*)
                        errn=${line#errors=}
                        ;;
                    warnings=*)
                        warnn=${line#warnings=}
                        ;;
                    esac
                done 3<&0
                if [[ $errn == '' || $warnn == '' ]] then
                    print -u2 SWIFT ERROR ${fdata.recordchecker} failed
                    print "<font color=red><b>"
                    print "ERROR ${fdata.recordchecker} failed to complete"
                    print "</b></font><br>"
                    ((terrn++))
                elif (( errn == 0 )) then
                    qs_list=${qs_list//%27/\'}
                    $SHELL ${fdata.fileeditor} \
                        $configfile $file $action $ext "$rec" \
                    "${qs_list%%\|++\|*}"
                    if [[ $? != 0 ]] then
                        print -u2 SWIFT ERROR ${fdata.fileeditor} failed
                        print "<font color=red><b>"
                        print "ERROR ${fdata.fileeditor} failed to complete"
                        print "</b></font><br>"
                        ((terrn++))
                    fi
                fi
                if [[ $errn != '' ]] then
                    ((terrn += errn))
                fi
                if [[ $warnn != '' ]] then
                    ((twarnn += warnn))
                fi
            done 3<&0
        else
            for field in "${!fdata.fields.@}"; do
                field=${field#files.$file.fields.}
                field=${field%%.*}
                print $field
            done | sort -u | while read -r field; do
                typeset -n fielddata=files.$file.fields.$field
                typeset -n fieldvalue=qs_${fielddata.name}
                rec="$rec|+|$fieldvalue"
                typeset +n fieldvalue
                typeset +n fielddata
            done
            rec=${rec#\|+\|}
            rec=${rec//%27/\'}
            print -r "<b>Record: <pre>$rec</pre></b><br>"
            ${fdata.recordchecker} $configfile $file "$rec" \
            | while read -r line; do
                case $line in
                ERROR*)
                    print "<font color=red><b>$line</b></font><br>"
                    ;;
                WARNING*)
                    print "<font color=red>$line</font><br>"
                    ;;
                MESSAGE*)
                    print "${line#MESSAGE\ }<br>"
                    ;;
                rec=*)
                    rec=${line#rec=}
                    ;;
                errors=*)
                    errn=${line#errors=}
                    ;;
                warnings=*)
                    warnn=${line#warnings=}
                    ;;
                esac
            done
            if [[ $errn == '' || $warnn == '' ]] then
                print -u2 SWIFT ERROR ${fdata.recordchecker} failed
                print "<font color=red><b>"
                print "ERROR ${fdata.recordchecker} failed to complete"
                print "</b></font><br>"
                ((terrn++))
            elif (( errn == 0 )) then
                qs_list=${qs_list//%27/\'}
                $SHELL ${fdata.fileeditor} \
                    $configfile $file $action $ext "$rec" \
                "${qs_list%%\|++\|*}"
                if [[ $? != 0 ]] then
                    print -u2 SWIFT ERROR ${fdata.fileeditor} failed
                    print "<font color=red><b>"
                    print "ERROR ${fdata.fileeditor} failed to complete"
                    print "</b></font><br>"
                    ((terrn++))
                fi
            fi
            if [[ $errn != '' ]] then
                ((terrn += errn))
            fi
            if [[ $warnn != '' ]] then
                ((twarnn += warnn))
            fi
        fi
        ;;
    esac
    if (( twarnn > 0 )) then
        print "<font color=red>"
        print "Warnings: $twarnn"
        print "</font><br>"
    fi
    if (( terrn > 0 )) then
        print "<font color=red><b>"
        print "Errors: $terrn"
        print "</b></font><br>"
        print "Please correct errors and try again"
    else
        print "Click on Commit to complete the update"
    fi
    print "<form"
    print "  action=vg_filemanager.cgi"
    print "  method=post enctype="multipart/form-data""
    print ">"
    print "<table class=page><tr class=page>"
    if (( terrn == 0 )) then
        print "  <td class=page>"
        print "  <input class=page type=submit name=_action value=Commit>"
        print "  </td>"
        print "  <td class=page>"
        print "  <input class=page type=submit name=_action value=Cancel>"
        print "  </td>"
    else
        print "  <td class=page><input"
        print "    class=page type=button name=back"
        print "    value=Back onClick='history.back()'"
        print "  ></td>"
    fi
    print "</tr></table>"
    print "<input type=hidden name=_mode value=finalize>"
    print "<input type=hidden name=_file value=$file>"
    print "<input type=hidden name=_ext value=$ext>"
    print "<input type=hidden name=_pid value=$qs_pid>"
    print "<input type=hidden name=_winw value=$qs_winw>"
    print "</form>"
    ;;
finalize)
    file=$qs_file
    typeset -n fdata=files.$file
    if ! swmuseringroups "${fdata.group}"; then
        print "<b><font color=red>"
        print "You are not authorized to modify this file"
        print "</font></b>"
        exit
    fi
    ext=$qs_ext

    print "<center><b>Editing ${fdata.summary}</b></center>"

    case $qs_action in
    Commit)
        trap 'rm -f ${fdata.location}.lock' HUP QUIT TERM KILL EXIT
        set -o noclobber
        while ! command exec 6> ${fdata.location}.lock; do
            if [[ $(fuser ${fdata.location}.lock 2> /dev/null) != '' ]] then
                sleep 1
            elif kill -0 $(< ${fdata.location}.lock); then
                sleep 1
            else
                rm ${fdata.location}.lock
            fi
        done 2> /dev/null
        print -u6 $$
        set +o noclobber
        if [[ ${fdata.location} -nt ${fdata.location}.$ext ]] then
            print -u2 SWIFT ERROR file ${fdata.location} was modified
            print "<font color=red><b>"
            print ERROR file ${fdata.location} was modified
            print "</b></font><br>"
        elif ! cp ${fdata.location} ${fdata.location}.old; then
            print -u2 SWIFT ERROR backup of file ${fdata.location} failed
            print "<font color=red><b>"
            print ERROR backup of file ${fdata.location} failed
            print "</b></font><br>"
        elif ! cp ${fdata.location}.$ext ${fdata.location}.new; then
            print -u2 SWIFT ERROR cannot create file ${fdata.location}.new
            print "<font color=red><b>"
            print ERROR cannot create file ${fdata.location}.new
            print "</b></font><br>"
        elif ! mv -f ${fdata.location}.new ${fdata.location}; then
            print -u2 SWIFT ERROR cannot update file ${fdata.location}
            print "<font color=red><b>"
            print ERROR cannot update file ${fdata.location}
            print "</b></font><br>"
        elif \
            [[ ${fdata.postprocess} != '' ]] && ! ${fdata.postprocess} 1>&2 \
        ; then
            print -u2 SWIFT ERROR post processor ${fdata.postprocess} failed
            print "<font color=red><b>"
            print ERROR post processor ${fdata.postprocess} failed
            print "</b></font><br>"
        else
            (
                print "VGMSG|$(printf '%(%#)T')|FILE INFO\c"
                print "|$SWMID updated ${fdata.location}, ext=$ext"
            ) >> ${fdata.logfile}
            print "<b>"
            print file $file updated
            print "</b>"
        fi
        ;;
    Cancel)
        print "<font color=red><b>"
        print Update Cancelled
        print "</b></font><br>"
        ;;
    esac
    print "<form"
    print "  action=vg_filemanager.cgi"
    print "  method=post enctype="multipart/form-data""
    print ">"
    print "<table class=page><tr class=page>"
    print "  <td class=page>"
    print "  <input class=page type=submit name=_action value=Continue>"
    print "  </td>"
    print "</tr></table>"
    print "<input type=hidden name=_mode value=show>"
    print "<input type=hidden name=_file value=$file>"
    print "<input type=hidden name=_pid value=$qs_pid>"
    print "<input type=hidden name=_winw value=$qs_winw>"
    print "</form>"
    print "<a class=pageil href=${cgiprefix}&mode=list>Main File List</a>"
    print "<a class=pageil href=vg_home.cgi>Home</a>"
    ;;
esac

fm_emit_body_end
