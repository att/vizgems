#!/bin/ksh

export SHELL=$(which ksh 2> /dev/null)
[[ $SECONDS != *.* ]] && exec $SHELL $0 "$@"

usage=$'
[-1p1s1D?
@(#)$Id: ddspp (AT&T Labs Research) 1999-01-23 $
]
[+NAME?ddspp - pretty printer for DDS files]
[+DESCRIPTION?\bddspp\b prints each of its file arguments in one of two styles:]{
[+table]
[+xml]
}
[+?Style \btable\b is a pipe-delimited CSV style.
Style \bxml\b generates an XML file.
The top tag is based on the name of the input file.
The inner tags are based on the field names.
]
[100:is?specifies the schema of the input DDS files.
This is only needed when the files are raw data files
without the standard DDS header.
]:[path]
[101:style?specifies the style to use.
The default is the table style.
]:[(table|xml)]
[998:q?specified once omits the file name from the output.
Specified twice also omits the header record.
]
[999:v?increases the verbosity level. May be specified multiple times.]

 [file1] ...

[+SEE ALSO?\bdds\b(1), \bdds\b(3)]
'

function showusage {
    OPTIND=0
    getopts -a ddspp "$usage" opt '-?'
}

style=table
sarg=''
quiet=0
while getopts -a ddspp "$usage" opt; do
    case $opt in
    100) sarg="-is $OPTARG" ;;
    101) style=$OPTARG ;;
    998) (( quiet++ )) ;;
    999) (( SWIFTWARNLEVEL++ )) ;;
    *) showusage; exit 1 ;;
    esac
done
shift $OPTIND-1

typeset -A fmtmap
fmtmap[char]='%c'
fmtmap[short]='%d'
fmtmap[int]='%d'
fmtmap[long]='%ld'
fmtmap['long long']='%lld'
fmtmap[float]='%f'
fmtmap[double]='%lf'

spec=(
    fmt=''
    hdr=''
    arg=''
    prefix=''
    suffix=''
    vars=''
)

xmlenc="
char *xmlenc (char *istr, char *ostr) {
    char *is, *os;
    static char hm[16] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };
    for (is = istr, os = ostr; *is; is++) {
        switch (*is) {
        case ' ': *os++ = '%', *os++ = '2', *os++ = '0'; break;
        case '+': *os++ = '%', *os++ = '2', *os++ = 'B'; break;
        case '%': *os++ = '%', *os++ = '2', *os++ = '5'; break;
        case ';': *os++ = '%', *os++ = '3', *os++ = 'B'; break;
        case '&': *os++ = '%', *os++ = '2', *os++ = '6'; break;
        case '|': *os++ = '%', *os++ = '7', *os++ = 'C'; break;
        case '\'': *os++ = '%', *os++ = '2', *os++ = '7'; break;
        case '\\\"': *os++ = '%', *os++ = '2', *os++ = '2'; break;
        case '<': *os++ = '%', *os++ = '3', *os++ = 'C'; break;
        case '>': *os++ = '%', *os++ = '3', *os++ = 'E'; break;
        case '\\\\': *os++ = '%', *os++ = '5', *os++ = 'C'; break;
        case '\n':
        case '\r':
            *os++ = ' ';
            break;
        default:
            if (*is < 32 || *is > 126)
                *os++ = '%', *os++ = hm[(*is / 16)], *os++ = hm[(*is % 16)];
            else
                *os++ = *is;
            break;
        }
    }
    *os = 0;
    return ostr;
}
"

function genstr { # $1 = file
    typeset line schema name type len xmlname sep1 sep2 fn i ind xmlind fmt
    typeset xmlvars bufsize

    typeset -n fr=spec.fmt
    typeset -n hr=spec.hdr
    typeset -n ar=spec.arg
    fr='' hr='' ar=''

    sep1='' sep2=''
    fn=0
    ddsinfo -V $sarg $1 | while read line; do
        case $line in
        schema:*)
            schema=${line#'schema: '}
            xmlschema=${schema##*/}; xmlschema=${xmlschema%.schema}
            xmlschema=${xmlschema//[!a-zA-Z0-9]/_}
            ;;
        recsize:*)
            bufsize=${line#'recsize: '}
            (( bufsize *= 3 ))
            ;;
        field:*)
            rest=${line#'field: '}
            name=${rest%%' '*}; rest=${rest#"$name "}
            len=${rest##*' '}; rest=${rest%' '*}
            type=$rest
            xmlname=${name//[!a-zA-Z0-9]/_}
            if [[ $type == char ]] then
                case $style in
                table)
                    hr+="$sep1$name"
                    fr+="$sep1%s"
                    ar+="$sep2$name"
                    ;;
                xml)
                    hr+="<f$fn>$xmlname</f$fn>"
                    fr+="<f$fn>%s</f$fn>"
                    ar+="${sep2}xmlenc($name,buf_$name)"
                    spec.vars+="static char buf_$name[.SIZE.]; "
                    ;;
                esac
                sep1='|' sep2=', '
            else
                for (( i = 0; i < len; i++ )) do
                    if (( $len > 1 )) then
                        xmlind="$i"
                        ind="[$xmlind]"
                    else
                        xmlind=""
                        ind=""
                    fi
                    fmt=${fmtmap[$type]}
                    case $style in
                    table)
                        hr+="$sep1$name$ind"
                        fr+="$sep1$fmt"
                        ar+="$sep2$name$ind"
                        ;;
                    xml)
                        hr+="<f$fn$xmlind>$xmlname</f$fn$xmlind>"
                        fr+="<f$fn$xmlind>$fmt</f$fn$xmlind>"
                        ar+="$sep2$name$ind"
                        ;;
                    esac
                sep1='|' sep2=', '
                done
            fi
            (( fn++ ))
            ;;
        esac
    done
    case $style in
    xml)
        hr="<hdr>$hr</hdr>"
        fr="<rec>$fr</rec>"
        spec.prefix="<$xmlschema>\n"
        spec.suffix="</$xmlschema>\n"
        spec.vars=${spec.vars//.SIZE./$bufsize}
        ;;
    esac
}

for i in "$@"; do
    genstr "$i"
    if (( quiet > 1 )) then
        spec.hdr=''
    else
        spec.hdr+='\n'
    fi
    spec.fmt+='\n'
    s="BEGIN{ sfprintf (sfstdout, \"${spec.prefix}${spec.hdr}\"); }"
    s+="{ sfprintf (sfstdout, \"${spec.fmt}\", ${spec.arg}); }"
    s+="END{ sfprintf (sfstdout, \"${spec.suffix}\"); }"
    case $style in
    xml)
        s+="VARS{ ${spec.vars} $xmlenc }"
        ;;
    esac
    (( quiet == 0 )) && print "\n$i"
    ddsprint -pe "$s" $i
done
