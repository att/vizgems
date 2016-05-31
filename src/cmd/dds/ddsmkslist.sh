#!/bin/ksh

export SHELL=$(which ksh 2> /dev/null)
[[ $SECONDS != *.* ]] && exec $SHELL $0 "$@"

usage=$'
[-1p1s1D?
@(#)$Id: ddsmkslist (AT&T Labs Research) 1999-01-23 $
]
'$USAGE_LICENSE$'
[+NAME?ddsmkslist - generate search code for DDS files]
[+DESCRIPTION?\bddsmkslist\b generates a C file that loads a DDS file and then
searches for records.
The file must be sorted by the specified keys.
\bddsmkslist\b performs binary searches.
This tool defines the following items:]{
[+dt_<name>_t]
[+void dt_<name>open (file)]
[+dt_<name>_t *dt_<name>find (field1p, ...)]
[+dt_<name>_t *dt_<name>findfirst (field1p, ...)]
[+dt_<name>_t *dt_<name>findnext (field1p, ...)]
}
[+?\bdt_<name>_t\b is the data structure containing both the key and any
requested fields.
\bdt_<name>open\b is the function that opens the DDS file.
\bdt_<name>find\b is the function that searches the file for a specific key.
The types of the field pointers should match the types of the fields in the key.
\bdt_<name>findfirst\b and \bdt_<name>findnext\b are used when the \bmode\b
flag is set to \ball\b.
]
[100:schema?specifies the schema of the input DDS file.
]:[path]
[101:name?specifies the name to use in the generated file.
The default is to use the name of the schema.
]:[name]
[102:mode?specifies how to handle records with duplicate keys.
The default is to only keep the first record.
]:[(unique|all):oneof]{
[+unique?only one record per unique key is kept][+all?all records are kept]
}
[104:warnonopen?specifies that the open function should return 1 if the file
cannot be opened.
The default is to exist immediately.
]
[103:key?specifies the set of fields that should make up the key.
]:["field1 ..."]
[105:run?specifies a shell command to run (by sourcing) before exiting.
This script will have access to all the variables such as the schema info.
]:[script]
"[999:v?increases the verbosity level. May be specified multiple times.]"
[+SEE ALSO?\bdds\b(1), \bdds\b(3), \baggr\b(1)]
'

mode=unique
postrun=

function findschema { # $1 = file name or full path
    if [[ -r $1 ]] then
        print $1
        return
    fi
    for dir in ${PATH//:/ }; do
        dir=${dir%%/}
        dir=${dir%/bin}
        if [[ -r $dir/lib/dds/$schema ]] then
            print $dir/lib/dds/$schema
            return
        fi
    done
}

function readschema { # $1 = full path of schema
    while read line; do
        set -A list $line
        case ${list[0]} in
        name)
            schemaname=${list[1]}
            ;;
        field)
            inds[${list[1]}]=$indn
            names[$indn]=${list[1]}
            types[$indn]=${list[2]}
            if [[ ${list[3]} != '' ]] then
                sizes[$indn]=${list[3]}
            else
                sizes[$indn]=1
            fi
            (( indn++ ))
            ;;
        esac
    done < $1
}

function showusage {
    OPTIND=0
    getopts -a ddsmkslist "$usage" opt '-?'
}

if [[ $SWIFTWARNLEVEL != '' ]] then
    typeset -ft findschema readschema
    set -x
fi

while getopts -a ddsmkslist "$usage" opt; do
    case $opt in
    100) schema=$OPTARG ;;
    101) name=$OPTARG ;;
    102) mode=$OPTARG ;;
    103) keys[$((keyn++))]=$OPTARG ;;
    104) warnonopen=y ;;
    105) postrun=$OPTARG ;;
    999) (( SWIFTWARNLEVEL++ )) ;;
    *) showusage; exit 1 ;;
    esac
done

if [[ $schema == '' && $name != '' ]] then
    schema=$name.schema
fi

schemafile=$(findschema $schema)
if [[ $schemafile == '' ]] then
    print -u2 ERROR cannot find schema file for $schema
    exit 1
fi
schemaname=${schema%.schema}
schemaname=${schemaname##*/}

typeset -A inds names types sizes
integer indn=0
readschema $schemafile
if [[ $indn == 0 ]] then
    print -u2 ERROR cannot read fields from $schemafile
    exit 1
fi
if [[ $name == '' ]] then
    name=${schemaname%.schema}
    name=${name##*/}
fi

if [[ $keys == "" ]] then
    keys=${names[0]}
fi
set -A keylist $keys
keyn=${#keylist[@]}

for key in $keys; do
    if [[ ${inds[$key]} == '' ]] then
        print -u2 ERROR no such field $key
        exit
    fi
done

print "
#include <ast.h>
#include <dds.h>

typedef struct sl_${name}_t {
"

for (( indi = 0; indi < indn; indi++ )) do
    if [[ ${sizes[$indi]} == 1 ]] then
        print "    ${types[$indi]} sl_${names[$indi]};"
    else
        print "    ${types[$indi]} sl_${names[$indi]}[${sizes[$indi]}];"
    fi
done

print "
} sl_${name}_t;

static Sfio_t *sl_${name}fp;
static DDSschema_t *sl_${name}schemap;
static off_t sl_${name}off1, sl_${name}off2;
static long long sl_${name}recn, sl_${name}reci;
static char *sl_${name}datap;
static sl_${name}_t *sl_${name}p;
static DDSheader_t sl_${name}_hdr;

#define OFF${name}(reci) (\\
    sl_${name}off1 + (off_t) sl_${name}schemap->recsize * reci\\
)
"

print "#define GETRECORD${name}(reci) {\\"
if [[ $(sutendian) == 0 ]] then
    print "    if (sl_${name}_hdr.vczp) {\\"
    print "        if (DDSvczread (\\"
    print "            &sl_${name}_hdr, sl_${name}datap,\\"
    print "            sl_${name}schemap->recbuf,\\"
    print "            OFF${name} (reci), sl_${name}schemap->recsize\\"
    print "        ) != sl_${name}schemap->recsize)\\"
    print "            SUerror (\\"
    print "                \"sl_${name}find\",\\"
    print "                \"cannot read record %lld\", reci\\"
    print "            );\\"
    print "    } else {\\"
    print "        memcpy (\\"
    print "            sl_${name}schemap->recbuf,\\"
    print "            sl_${name}datap + OFF${name} (reci),\\"
    print "            sl_${name}schemap->recsize\\"
    print "        );\\"
    print "    }\\"
    print "    DDSswaprecord (sl_${name}schemap->recbuf, sl_${name}schemap);\\"
    print "    sl_${name}p = (sl_${name}_t *) sl_${name}schemap->recbuf;\\"
else
    print "    if (sl_${name}_hdr.vczp) {\\"
    print "        if (DDSvczread (\\"
    print "            &sl_${name}_hdr, sl_${name}datap,\\"
    print "            sl_${name}schemap->recbuf,\\"
    print "            OFF${name} (reci), sl_${name}schemap->recsize\\"
    print "        ) != sl_${name}schemap->recsize)\\"
    print "            SUerror (\\"
    print "                \"sl_${name}find\",\\"
    print "                \"cannot read record %lld\", reci\\"
    print "            );\\"
    print "        sl_${name}p = (sl_${name}_t *) sl_${name}schemap->recbuf;\\"
    print "    } else {\\"
    print "        sl_${name}p = (sl_${name}_t *) (\\"
    print "            sl_${name}datap + OFF${name} (reci)\\"
    print "        );\\"
    print "    }\\"
fi
print "}"
print ""

print "#define CMP${name}(\c"

flag=
for key in $keys; do
    n2i=${inds[$key]}
    if [[ $flag == y ]] then
        print ", \c"
    fi
    flag=y
    print "sl_${key}p\c"
done

print ") {\\"

t=$keys
for (( keyi = 0; keyi < keyn; keyi++ )) do
    key=${t%% *}
    t=${t#* }
    n2i=${inds[$key]}
    if [[ ${sizes[$n2i]} == 1 ]] then
        print "    c = (*sl_${key}p - sl_${name}p->sl_${key});\\"
    else
        print "    c = memcmp (\\"
        print "        sl_${key}p, sl_${name}p->sl_${key},\\"
        print "        ${sizes[$n2i]} * sizeof (${types[$n2i]})\\"
        print "    );\\"
    fi
    if (( keyi < keyn - 1 )) then
        print "    if (c == 0)\\"
    fi
done

print "}"

print "
static int sl_${name}open (char *sl_file) {
"

if [[ $warnonopen == y ]] then
    print "
        if (!(sl_${name}fp = sfopen (NULL, sl_file, \"r\"))) {
            SUwarning (1, \"sl_${name}open\", \"cannot open file %s\", sl_file);
            return 1;
        }
    "
else
    print "
        if (!(sl_${name}fp = sfopen (NULL, sl_file, \"r\")))
            SUerror (\"sl_${name}open\", \"cannot open file %s\", sl_file);
    "
fi

print "
    if (DDSreadheader (sl_${name}fp, &sl_${name}_hdr) <= -1)
        SUerror (\"sl_${name}open\", \"readheader failed for %s\", sl_file);
    if (
        (sl_${name}off1 = sfseek (sl_${name}fp, 0, SEEK_CUR)) == -1 ||
        (sl_${name}off2 = sfseek (sl_${name}fp, 0, SEEK_END)) == -1
    )
        SUerror (\"sl_${name}open\", \"size check failed for %s\", sl_file);
    sl_${name}schemap = sl_${name}_hdr.schemap;
    sl_${name}recn = (
        sl_${name}off2 - sl_${name}off1
    ) / sl_${name}schemap->recsize;
    if (sl_${name}_hdr.vczp && (
        sl_${name}off2 = DDSvczsize (&sl_${name}_hdr)
    ) == -1)
        SUerror (\"sl_${name}open\", \"vcz size check failed for %s\", sl_file);
    if (!(sl_${name}datap = mmap (
        NULL, sl_${name}off2, PROT_READ, MAP_PRIVATE, sffileno (sl_${name}fp), 0
    )))
        SUerror (\"sl_${name}open\", \"mmap failed for %s\", sl_file);
    return 0;
}

static void sl_${name}close (void) {
    if (sl_${name}datap)
        munmap (sl_${name}datap, sl_${name}off2);
    sfclose (sl_${name}fp);
}

static sl_${name}_t *sl_${name}find (
"

keylist=
flag=
for key in $keys; do
    n2i=${inds[$key]}
    if [[ $flag == y ]] then
        print ","
        keylist="${keylist}, "
    fi
    flag=y
    print "    ${types[$n2i]} *sl_${key}p\c"
    keylist="${keylist}sl_${key}p"
done

print "
) {
    long long minreci, maxreci;
    int c;

    minreci = 0, maxreci = sl_${name}recn - 1;
    if (maxreci == -1)
        return NULL;
    sl_${name}reci = (maxreci + minreci) / 2;
    GETRECORD${name} (sl_${name}reci);
    CMP${name} ($keylist);
    while (c != 0 && maxreci != minreci) {
        if (c > 0)
            if (sl_${name}reci == maxreci)
                break;
            else
                minreci = sl_${name}reci + 1;
        else
            if (sl_${name}reci == minreci)
                break;
            else
                maxreci = sl_${name}reci - 1;
        sl_${name}reci = (maxreci + minreci) / 2;
        GETRECORD${name} (sl_${name}reci);
        CMP${name} ($keylist);
    }
    if (c != 0)
        return NULL;
"

if [[ $mode == all ]] then
    print "    {"
    print "        long long currreci;"
    print "        for ("
    print "            currreci = sl_${name}reci - 1;"
    print "            currreci >= 0; currreci--"
    print "        ) {"
    print "            GETRECORD${name} (currreci);"
    print "            CMP${name} ($keylist);"
    print "            if (c != 0) {"
    print "                sl_${name}reci = currreci + 1;"
    print "                GETRECORD${name} (sl_${name}reci);"
    print "                break;"
    print "            }"
    print "            sl_${name}reci = currreci;"
    print "        }"
    print "    }"
fi

print "
    return sl_${name}p;
}
"

if [[ $mode == all ]] then
    print "#define sl_${name}findfirst sl_${name}find"
    print "static sl_${name}_t *sl_${name}findnext ("
    flag=
    for key in $keys; do
        n2i=${inds[$key]}
        if [[ $flag == y ]] then
            print ","
        fi
        flag=y
        print "    ${types[$n2i]} *sl_${key}p\c"
    done
    print ") {"
    print "    int c;"
    print ""
    print "    if (++sl_${name}reci >= sl_${name}recn)"
    print "        return NULL;"
    print "    GETRECORD${name} (sl_${name}reci);"
    print "    CMP${name} ($keylist);"
    print "    if (c != 0)"
    print "        return NULL;"
    print "    return sl_${name}p;"
    print "}"
fi

if [[ $postrun != '' ]] then
    . $postrun
fi
