#!/bin/ksh

export SHELL=$(which ksh 2> /dev/null)
[[ $SECONDS != *.* ]] && exec $SHELL $0 "$@"

usage=$'
[-1p1s1D?
@(#)$Id: ddsmkarray (AT&T Labs Research) 1999-01-23 $
]
'$USAGE_LICENSE$'
[+NAME?ddsmkarray - generate search code for DDS files]
[+DESCRIPTION?\bddsmkarray\b generates a C file that loads a DDS file, builds
an array based on the specified key, and then searches for records.
This tool defines the following items:]{
[+dt_<name>_t]
[+void dt_<name>load (file, size)]
[+dt_<name>_t *dt_<name>find (field)]
}
[+?\bdt_<name>_t\b is the data structure containing both the key and any
requested fields.
\bdt_<name>load\b is the function that loads the DDS file and builds the array.
The \bsize\b argument defines the size of the array.
\bdt_<name>find\b is the function that searches the array for a specific key.
The type of the field should match the type of the field in the key.
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
[103:key?specifies the single field that should be the key.
]:[field]
[104:fields?specifies the set of fields that should be included in the data
structure.
The default is to include all the fields.
]:["field1 ..."]
"[999:v?increases the verbosity level. May be specified multiple times.]"
[+SEE ALSO?\bdds\b(1), \bdds\b(3), \baggr\b(1)]
'

mode=unique
keyn=0

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
    getopts -a ddsmkarray "$usage" opt '-?'
}

if [[ $SWIFTWARNLEVEL != '' ]] then
    typeset -ft findschema readschema
    set -x
fi

while getopts -a ddsmkarray "$usage" opt; do
    case $opt in
    100) schema=$OPTARG ;;
    101) name=$OPTARG ;;
    102) mode=$OPTARG ;;
    103)
        keys[$((keyn++))]=$OPTARG
        set -A tstlst $OPTARG
        if [[ ${#tstlst[@]} != 1 ]] then
            print -u2 ERROR only one field allowed in key
            exit 1
        fi
        ;;
    104) fields=$OPTARG ;;
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

if [[ $keyn == 0 ]] then
    keys[0]=${names[0]}
    keyn=1
fi

for ((keyi = 0; keyi < keyn; keyi++ )) do
    for key in ${keys[$keyi]}; do
        if [[ ${inds[$key]} == '' ]] then
            print -u2 ERROR no such field $key
            exit
        fi
    done
done

if [[ $fields == '' ]] then
    for (( indi = 0; indi < indn; indi++ )) do
        fields="$fields ${names[$indi]}"
    done
    fields=${fields# }
else
    for field in $fields; do
        if [[ ${inds[$field]} == '' ]] then
            print -u2 ERROR no such field $field
            exit
        fi
    done
fi

print "
#include <ast.h>
#include <cdt.h>
#include <dds.h>

typedef struct ar_${name}_t {
"

if [[ $keyn != 1 ]] then
    print "    int arp_keyindex;"
fi

if [[ $mode == all ]] then
    print "    struct ar_${name}_t *arp_next;"
fi

for field in $fields; do
    n2i=${inds[$field]}
    if [[ ${sizes[$n2i]} == 1 ]] then
        print "    ${types[$n2i]} ar_$field;"
    else
        print "    ${types[$n2i]} ar_${field}[${sizes[$n2i]}];"
    fi
done

print "
} ar_${name}_t;

static ar_${name}_t **ar_${name}ps;
static int ar_${name}pn;

void ar_${name}load (char *ar_file, int ar_incr) {
    Sfio_t *ar_fp;
    DDSheader_t ar_hdr;
    DDSschema_t *ar_schemap;
    DDSfield_t *ar_fps[$indn];
    ar_${name}_t *ar_${name}mem;
    int ar_i, ar_index, ar_newpn;

    if (ar_incr == -1)
        ar_incr = 1000;
    if (!(ar_fp = sfopen (NULL, ar_file, \"r\")))
        SUerror (\"ar_${name}load\", \"cannot open file %s\", ar_file);
    if (!(ar_${name}ps = malloc (ar_incr * sizeof (ar_${name}_t *))))
        SUerror (\"ar_${name}load\", \"cannot allocate array\");
    ar_${name}pn = ar_incr;
    for (ar_i = 0; ar_i < ar_incr; ar_i++)
        ar_${name}ps[ar_i] = NULL;
    if (DDSreadheader (ar_fp, &ar_hdr) <= -1)
        SUerror (\"ar_${name}load\", \"readheader failed for %s\", ar_file);
    sfsetbuf (ar_fp, NULL, 1048576);
    ar_schemap = ar_hdr.schemap;
"

for (( indi = 0; indi < indn; indi++ )) do
    print "    ar_fps[$indi] = DDSfindfield (ar_schemap, \"${names[$indi]}\");"
done

print "
    while (DDSreadrecord (
        ar_fp, ar_schemap->recbuf, ar_schemap
    ) == ar_schemap->recsize) {
"

for ((keyi = 0; keyi < keyn; keyi++ )) do
    key=${keys[$keyi]}
    n2i=${inds[$key]}
    print "
        if (!(ar_${name}mem = calloc (1, sizeof (ar_${name}_t))))
            SUerror (\"ar_${name}load\", \"malloc failed\");
        if ((
            ar_index =
            (int) * (${types[$n2i]} *) &ar_schemap->recbuf[ar_fps[$n2i]->off]
        ) < 0)
            SUerror (\"dt_${name}load\", \"bad index %d\", ar_index);
        else if (ar_index >= ar_${name}pn) {
            ar_newpn = ((ar_index + ar_incr) / ar_incr) * ar_incr;
            if (!(ar_${name}ps = realloc (
                ar_${name}ps, ar_newpn * sizeof (ar_${name}_t *)
            )))
                SUerror (\"ar_${name}load\", \"realloc failed\");
            for (ar_i = ar_${name}pn; ar_i < ar_newpn; ar_i++)
                ar_${name}ps[ar_i] = NULL;
            ar_${name}pn = ar_newpn;
        }
    "

    if [[ $keyn != 1 ]] then
        print "        ar_${name}mem->arp_keyindex = $keyi;"
    fi
    for field in $fields; do
        n2i=${inds[$field]}
        print "        memcpy ("
        print "            &ar_${name}mem->ar_$field,"
        print "            &ar_schemap->recbuf[ar_fps[$n2i]->off],"
        print "            ${sizes[$n2i]} * sizeof (${types[$n2i]})"
        print "        );"
    done
    if [[ $mode == all ]] then
        print "        ar_${name}mem->arp_next = ar_${name}ps[ar_index];"
    fi
    print "        ar_${name}ps[ar_index] = ar_${name}mem;"
done

print "
    }
    sfclose (ar_fp);
}

ar_${name}_t *ar_${name}find (
"

key=${keys[0]}
n2i=${inds[$key]}
print "    ${types[$n2i]} ar_${key}\c"

print "
) {
    if (ar_${key} < 0 || ar_${key} >= ar_${name}pn) {
        SUwarning (1, \"ar_${name}find\", \"match failed\");
        return NULL;
    }
    return ar_${name}ps[ar_${key}];
}
"
