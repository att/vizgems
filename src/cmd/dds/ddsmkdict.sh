#!/bin/ksh

export SHELL=$(which ksh 2> /dev/null)
[[ $SECONDS != *.* ]] && exec $SHELL $0 "$@"

usage=$'
[-1p1s1D?
@(#)$Id: ddsmkdict (AT&T Labs Research) 1999-01-23 $
]
'$USAGE_LICENSE$'
[+NAME?ddsmkdict - generate search code for DDS files]
[+DESCRIPTION?\bddsmkdict\b generates a C file that loads a DDS file, builds a
dictionary based on the specified key, and then searches for records.
This tool defines the following items:]{
[+dt_<name>_t]
[+void dt_<name>load (file)]
[+dt_<name>_t *dt_<name>find (field1p, ...)]
}
[+?\bdt_<name>_t\b is the data structure containing both the key and any
requested fields.
\bdt_<name>load\b is the function that loads the DDS file and builds the
dictionary.
\bdt_<name>find\b is the function that searches the dictionary for a specific
key.
The types of the field pointers should match the types of the fields in the key.
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
[103:key?specifies the set of fields that should make up the key.
Can be specified multiple times.
In this case, each record is entered in the dictionary under each key.
All keys must have the same length.
]:["field1 ..."]
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
    getopts -a ddsmkdict "$usage" opt '-?'
}

if [[ $SWIFTWARNLEVEL != '' ]] then
    typeset -ft findschema readschema
    set -x
fi

while getopts -a ddsmkdict "$usage" opt; do
    case $opt in
    100) schema=$OPTARG ;;
    101) name=$OPTARG ;;
    102) mode=$OPTARG ;;
    103) keys[$((keyn++))]=$OPTARG ;;
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

typedef struct dt_${name}_key_t {
"

for key in ${keys[0]}; do
    n2i=${inds[$key]}
    if [[ ${sizes[$n2i]} == 1 ]] then
        print "    ${types[$n2i]} dt_$key;"
    else
        print "    ${types[$n2i]} dt_${key}[${sizes[$n2i]}];"
    fi
done

print "
} dt_${name}_key_t;

typedef struct dt_${name}_t {
    Dtlink_t link;
    /* begin key */
    dt_${name}_key_t dtp_key;
    /* end key */
"

if [[ $keyn != 1 ]] then
    print "    struct dt_${name}_t *dtp_mainkeyp;"
fi

if [[ $mode == all ]] then
    print "    struct dt_${name}_t *dtp_next, *dtp_last;"
fi

for field in $fields; do
    n2i=${inds[$field]}
    if [[ ${sizes[$n2i]} == 1 ]] then
        print "    ${types[$n2i]} dt_$field;"
    else
        print "    ${types[$n2i]} dt_${field}[${sizes[$n2i]}];"
    fi
done

print "
} dt_${name}_t;

static Dt_t *dt_${name}dict;

static Dtdisc_t dt_${name}_disc = {
    sizeof (Dtlink_t), sizeof (dt_${name}_key_t), 0,
    NULL, NULL, NULL, NULL, NULL, NULL
};

void dt_${name}load (char *dt_file) {
    Sfio_t *dt_fp;
    DDSheader_t dt_hdr;
    DDSschema_t *dt_schemap;
    DDSfield_t *dt_fps[$indn];
    dt_${name}_t *dt_${name}mem, *dt_${name}p;

    if (!(dt_fp = sfopen (NULL, dt_file, \"r\")))
        SUerror (\"dt_${name}load\", \"cannot open file %s\", dt_file);
    if (!(dt_${name}dict = dtopen (&dt_${name}_disc, Dtset)))
        SUerror (\"dt_${name}load\", \"cannot open dictionary\");
    if (DDSreadheader (dt_fp, &dt_hdr) <= -1)
        SUerror (\"dt_${name}load\", \"readheader failed for %s\", dt_file);
    sfsetbuf (dt_fp, NULL, 1048576);
    dt_schemap = dt_hdr.schemap;
"

for (( indi = 0; indi < indn; indi++ )) do
    print "    dt_fps[$indi] = DDSfindfield (dt_schemap, \"${names[$indi]}\");"
done

print "
    while (DDSreadrecord (
        dt_fp, dt_schemap->recbuf, dt_schemap
    ) == dt_schemap->recsize) {
"

for ((keyi = 0; keyi < keyn; keyi++ )) do
    print "
        if (!(dt_${name}mem = calloc (1, sizeof (dt_${name}_t))))
            SUerror (\"dt_${name}load\", \"malloc failed\");
    "
    set -A list ${keys[0]}
    index=0
    for key in ${keys[$keyi]}; do
        n2i=${inds[$key]}
        print "        memcpy ("
        print "            &dt_${name}mem->dtp_key.dt_${list[$index]},"
        print "            &dt_schemap->recbuf[dt_fps[$n2i]->off],"
        print "            ${sizes[$n2i]} * sizeof (${types[$n2i]})"
        print "        );"
        (( index++ ))
    done
    if [[ $keyn != 1 ]] then
        if [[ $keyi == 0 ]] then
            print "        dt_${name}mem->dtp_mainkeyp = NULL;"
        else
            print "        dt_${name}mem->dtp_mainkeyp = dt_${name}p;"
        fi
    fi
    if [[ $mode == all ]] then
        print "        dt_${name}mem->dtp_next = NULL;"
        print "        dt_${name}mem->dtp_last = dt_${name}mem;"
    fi
    for field in $fields; do
        n2i=${inds[$field]}
        print "        memcpy ("
        print "            &dt_${name}mem->dt_$field,"
        print "            &dt_schemap->recbuf[dt_fps[$n2i]->off],"
        print "            ${sizes[$n2i]} * sizeof (${types[$n2i]})"
        print "        );"
    done
    print "
        if (!(dt_${name}p = dtinsert (dt_${name}dict, dt_${name}mem)))
            SUerror (\"dt_${name}load\", \"insert failed\");
        else if (dt_${name}p != dt_${name}mem) {
    "
    if [[ $mode == all ]] then
        print "            dt_${name}p->dtp_last->dtp_next = dt_${name}mem;"
        print "            dt_${name}p->dtp_last = dt_${name}mem;"
    else
        print "            SUwarning (1, \"dt_${name}load\", \"duplicate\");"
        print "            free (dt_${name}mem);"
    fi
    print "        }"
done

print "
    }
    sfclose (dt_fp);
}

dt_${name}_t *dt_${name}find (
"

flag=
for key in ${keys[0]}; do
    n2i=${inds[$key]}
    if [[ $flag == y ]] then
        print ","
    fi
    flag=y
    print "    ${types[$n2i]} *dt_${key}p\c"
done

print "
) {
    static dt_${name}_key_t dt_${name}key;
    static dt_${name}_t *dt_${name}p;
"

for key in ${keys[0]}; do
    n2i=${inds[$key]}
    print "    memcpy ("
    print "        &dt_${name}key.dt_$key, dt_${key}p,"
    print "        ${sizes[$n2i]} * sizeof (${types[$n2i]})"
    print "    );"
done

print "
    if (!(dt_${name}p = dtmatch (dt_${name}dict, &dt_${name}key))) {
        SUwarning (1, \"dt_${name}find\", \"match failed\");
        return NULL;
    }
    return dt_${name}p;
}
"
