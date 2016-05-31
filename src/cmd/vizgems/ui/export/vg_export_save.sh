#!/bin/ksh

type=$1
name=$2
notes=$3
notes=${notes//[$',\r\n']/' '}
date=$(printf '%(%Y%m%d_%H%M%S)T')

typeset -A suffixes
suffixes[csv]=csv
suffixes[xml]=xml
suffixes[html]=zip
suffixes[pdf]=pdf

xml_ns="ss:Type='Number'"
xml_ss="ss:Type='String'"
function xml_file_begin {
    typeset t

    print "<?xml version='1.0'?>"
    print "<?mso-application progid='Excel.Sheet'?>"
    print "<Workbook"
    print " xmlns='urn:schemas-microsoft-com:office:spreadsheet'"
    print " xmlns:o='urn:schemas-microsoft-com:office:office'"
    print " xmlns:x='urn:schemas-microsoft-com:office:excel'"
    print " xmlns:ss='urn:schemas-microsoft-com:office:spreadsheet'"
    print " xmlns:html='http://www.w3.org/TR/REC-html40'"
    print ">"
    print "<DocumentProperties"
    print " xmlns='urn:schemas-microsoft-com:office:office'"
    print ">"
    print " <LastAuthor>AT&amp;T Visualizer</LastAuthor>"
    t=$(TZ=GMT printf '%(%Y-%m-%dT%H:%M:%SZ)T')
    print " <Created>$t</Created>"
    print " <Version>11.8036</Version>"
    print "</DocumentProperties>"
    print "<ExcelWorkbook xmlns='urn:schemas-microsoft-com:office:excel'>"
    print " <WindowHeight>8445</WindowHeight>"
    print " <WindowWidth>10395</WindowWidth>"
    print " <WindowTopX>480</WindowTopX>"
    print " <WindowTopY>60</WindowTopY>"
    print " <ProtectStructure>False</ProtectStructure>"
    print " <ProtectWindows>False</ProtectWindows>"
    print "</ExcelWorkbook>"
    print "<Styles>"
    print " <Style ss:ID='Default' ss:Name='Normal'>"
    print "  <Alignment ss:Vertical='Bottom'/>"
    print "  <Borders/>"
    print "  <Font/>"
    print "  <Interior/>"
    print "  <NumberFormat/>"
    print "  <Protection/>"
    print " </Style>"
    print " <Style ss:ID='s01'>"
    print "  <Alignment ss:WrapText='1'/>"
    print "  <NumberFormat ss:Format='@'/>"
    print " </Style>"
    print " <Style ss:ID='s21'>"
    print "  <NumberFormat ss:Format='[\$-F400]h:mm:ss\\ AM/PM'/>"
    print " </Style>"
    print " <Style ss:ID='s22'>"
    print "  <NumberFormat ss:Format='Fixed'/>"
    print " </Style>"
    print " <Style ss:ID='s23'>"
    print "  <NumberFormat ss:Format='@'/>"
    print " </Style>"
    print " <Style ss:ID='s24'>"
    print "  <Alignment ss:Vertical='Bottom' ss:WrapText='1'/>"
    print "  <NumberFormat ss:Format='@'/>"
    print " </Style>"
    print "</Styles>"
}
function xml_file_end {
    print "</Workbook>"
}
function xml_page_begin { # $1=rown $2=coln $3=pageid $4=pagecaption
    typeset pagerown=$1 pagecoln=$2 pageid=$3 pagecaption=$4
    typeset coli

    print "<Worksheet ss:Name='page $pageid'>"
    print "<Table"
    print " ss:ExpandedColumnCount='$pagecoln'"
    if [[ $notes != '' ]] then
        print " ss:ExpandedRowCount='$(( $pagerown + 2 ))'"
    else
        print " ss:ExpandedRowCount='$(( $pagerown + 1 ))'"
    fi
    print " x:FullColumns='1' x:FullRows='1'"
    print ">"
    for (( coli = 0; coli < coln; coli++ )) do
        print -- "<Column ss:StyleID='s01'/>"
    done
    print "<Row>"
    print "<Cell><Data $xml_ss>$pagecaption</Data></Cell>"
    print "</Row>"
    if [[ $notes != '' ]] then
        print "<Row>"
        print "<Cell><Data $xml_ss>$notes</Data></Cell>"
        print "</Row>"
    fi
}
function xml_page_end {
    print "</Table>"
    print "<WorksheetOptions"
    print " xmlns='urn:schemas-microsoft-com:office:excel'"
    print ">"
    print "<Print>"
    print "<ValidPrinterInfo/>"
    print "<HorizontalResolution>600</HorizontalResolution>"
    print "<VerticalResolution>0</VerticalResolution>"
    print "</Print>"
    print "<ProtectObjects>False</ProtectObjects>"
    print "<ProtectScenarios>False</ProtectScenarios>"
    print "</WorksheetOptions>"
    print "</Worksheet>"
}
function xml_row_begin {
    print "<Row>"
}
function xml_row_end {
    print "</Row>"
}
function xml_cell { # $1=coli $2=v
    typeset i=$coli v=$2

    print -n "<Cell>"
    case $v in
    ?(-)@(+([0-9])|+([0-9]).+([0-9])))
        print -rn "<Data $xml_ns>$v</Data>"
        ;;
    *)
        print -rn "<Data $xml_ss>$v</Data>"
        ;;
    esac
    print "</Cell>"
}

function csv_file_begin {
    return
}
function csv_file_end {
    return
}
function csv_page_begin { # $1=rown $2=coln $3=pageid $4=pagecaption
    typeset pagerown=$1 pagecoln=$2 pageid=$3 pagecaption=$4

    if [[ $notes != '' ]] then
        print -r -- "\"$notes\","
        print -r ""
    fi
    print -r -- "\"$pagecaption\","
    print -r ""
}
function csv_page_end {
    print -r ""
}
function csv_row_begin {
    return
}
function csv_row_end {
    print -r ""
}
function csv_cell { # $1=coli $2=v
    typeset i=$coli v=$2

    [[ $i != 0 ]] && print -rn ","
    v=${v//'"'/}
    print -rn "\"$v\""
}

function runinxvfb {
    typeset mcookie authfile proto i j num ret pid

    mcookie=$(mcookie)
    authfile=$PWD/Xauthority
    export XAUTHORITY=$authfile
    proto=.

    num=bad
    for (( i = 0; i < 10; i++ )) do
        (( num = 100 + i ))
        rm $authfile
        xauth source - <<#EOF
        add :$num $proto $mcookie
        EOF
        Xvfb :$num -screen 0 1280x1024x24 &
        pid=$!
        ret=0
        for (( j = 0; j < 4; j++ )) do
            if kill -0 $pid; then
                DISPLAY=:$num xdpyinfo 2>&1
                ret=$?
                if [[ $ret == 0 ]] then
                    break
                fi
                sleep 1
            fi
        done
        [[ $ret == 0 ]] && break
        num=bad
    done

    if [[ $num != bad ]] then
        DISPLAY=:$num $@
        ret=$?
    fi

    kill $pid
    for (( j = 0; j < 4; j++ )) do
        if ! kill -0 $pid; then
            break
        fi
        sleep 1
    done
    kill -9 $pid

    return $ret
}

case $type in
csv|xml)
    exec 4>&1
    exec > "$name.${suffixes[$type]}"

    ${type}_file_begin

    filei=0
    for file in *tab.*.html; do
        [[ ! -f $file ]] && continue
        (( filei++ ))
        {
            caption='Data'
            unset cols; typeset -A cols
            while read -r line; do
                case $line in
                "<caption"*)
                    caption=${line#*'>'}
                    caption=${caption%'</caption>'}
                    caption=${caption//'<'*([!<>])'>'/}
                    break
                    ;;
                esac
            done
            coli=0 coln=0
            rowi=0
            unset vals; typeset -A vals
            reci=0
            unset recs; typeset -A recs
            unset recvals; typeset -A recvals
            recs[0]=0
            while read -r line; do
                case $line in
                "<tr"*)
                    (( coli = ${recs[$reci]} ))
                    for (( colj = 0; colj < coli; colj++ )) do
                        vals[$rowi:$colj]=${recvals[$colj]}
                    done
                    ;;
                "</tr"*)
                    (( rowi++ ))
                    if (( coln < coli )) then
                        (( coln = coli ))
                    fi
                    (( coli = ${recs[$reci]} ))
                    ;;
                "<td"*"><table"*)
                    (( reci++ ))
                    recs[$reci]=$coli
                    for (( colj = 0; colj < coli; colj++ )) do
                        recvals[$colj]=${vals[$rowi:$colj]}
                    done
                    ;;
                "</table></td>")
                    (( reci-- ))
                    ;;
                "<td"*)
                    v=${line#*'>'}
                    v=${v%'</td>'}
                    v=${v//'<'*([!'>'])'>'/}
                    vals[$rowi:$coli]="$v"
                    (( coli++ ))
                    if [[ $line == *colspan=+([0-9])* ]] then
                        (( colm = ${.sh.match[1]} - 1 ))
                        for (( colj = 0; colj < colm; colj++ )) do
                            vals[$rowi:$coli]=""
                            (( coli++ ))
                        done
                    fi
                    ;;
                esac
            done
            rown=$rowi
            ${type}_page_begin $rown $coln "$filei" "$caption"
            for (( rowi = 0; rowi < rown; rowi++ )) do
                ${type}_row_begin
                for (( coli = 0; coli < coln; coli++ )) do
                    ${type}_cell $coli "${vals[$rowi:$coli]}"
                done
                ${type}_row_end
            done
            ${type}_page_end
        } < $file
    done

    ${type}_file_end

    exec >&-
    exec 1>&4
    ;;
html)
    mkdir -p export.dir || { exit 1; }
    rm -f export.dir/* ${suffixes[$type]}

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
            print "<td align=right><b>From</b></td><td>$SWMNAME ($SWMID)</td>"
            print "</tr>"
            print "<tr>"
            print "<td align=right><b>Date</b></td><td>$(date)</td>"
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
            print "<img src=\"./$file\">"
            (( cidn++ ))
            ;;
        '<'img*'>')
            file=${line#*src=}
            file=${file%%['>' ]*}
            file=${file//[\'\"]/}
            paths[$cidn]=$SWMROOT/htdocs/${file#$SWIFTWEBPREFIX/}
            file=${file##*/}
            cids[$cidn]=$file
            print "<img src=\"./$file\">"
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
    done < rep.out > export.dir/index.htm

    for (( cidi = 0; cidi < cidn; cidi++ )) do
        cp ${paths[$cidi]} export.dir/${cids[$cidi]}
    done

    zip -r "$name" export.dir > /dev/null
    ;;
pdf)
    mkdir -p export.dir || { exit 1; }
    rm -f export.dir/* ${suffixes[$type]}

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
            print "<td align=right><b>From</b></td><td>$SWMNAME ($SWMID)</td>"
            print "</tr>"
            print "<tr>"
            print "<td align=right><b>Date</b></td><td>$(date)</td>"
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
            if [[ $file == inv* ]] then
                print "<img width=100% src=\"./$file\">"
            else
                print "<img src=\"./$file\">"
            fi
            (( cidn++ ))
            ;;
        '<'img*'>')
            file=${line#*src=}
            file=${file%%['>' ]*}
            file=${file//[\'\"]/}
            paths[$cidn]=$SWMROOT/htdocs/${file#$SWIFTWEBPREFIX/}
            file=${file##*/}
            cids[$cidn]=$file
            print "<img src=\"./$file\">"
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
    done < rep.out > export.dir/index.htm

    for (( cidi = 0; cidi < cidn; cidi++ )) do
        cp ${paths[$cidi]} export.dir/${cids[$cidi]}
    done

    if whence -q wkhtmltopdf; then
        (
            cd export.dir
            runinxvfb wkhtmltopdf index.htm ../$name.pdf
        ) > export.dir/log 2>&1
    elif whence -q htmldoc; then
        (
            cd export.dir
            HTMLDOC_NOCGI=1 htmldoc \
                -t pdf --webpage --color --outfile ../$name.pdf index.htm \
        ) > export.dir/log 2>&1
    else
        print -u2 SWIFT ERROR no html to pdf program found
        exit 1
    fi
    ;;
esac

print $name.${suffixes[$type]}

exit 0
