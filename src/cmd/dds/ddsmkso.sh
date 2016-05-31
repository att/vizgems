#!/bin/ksh

export SHELL=$(which ksh 2> /dev/null)
[[ $SECONDS != *.* ]] && exec $SHELL $0 "$@"

# given a c file, generate a shared object

#usage=$'
#[-1p1s1D?
#@(#)$Id: ddsmkso (AT&T Labs Research) 1999-01-23 $
#]
#'$USAGE_LICENSE$'
#[+NAME?ddsmkso - generate a shared object out of a C file]
#[+DESCRIPTION?\bddsmkso\b compiles a C file into a shared object.
#The shared object implements a set of methods for processing DDS data.
#]
#[200:o?specifies the name of the generated shared object file.
#]:[file]
#[999:v?increases the verbosity level. May be specified multiple times.]
#
#file
#
#[+SEE ALSO?\bdds\b(1), \bdds\b(3)]
#'

function showusage {
    OPTIND=0
    getopts -a ddsmkso "$usage" opt '-?'
}

if [[ $SWIFTWARNLEVEL != '' ]] then
    print -u2 $PS4 executing ddsmkso
    set -x
fi

buildflag=-O
if [[ $SWIFTDEBUGMODE != '' ]] then
    buildflag=-g
fi

if [[ $SWIFTPIXIEMODE != '' ]] then
    pixieflag=y
fi

#while getopts -a ddsmkso "$usage" opt; do
#    case $opt in
#    200) sofile=$OPTARG ;;
#    999) (( SWIFTWARNLEVEL++ )) ;;
#    *) showusage; exit 1 ;;
#    esac
#done
#shift $OPTIND-1

while (( $# > 0 )) do
    if [[ $1 == '-o' ]] then
        sofile=$2
        shift; shift
    elif [[ $1 == '-libs' ]] then
        libs=$2
        shift; shift
    elif [[ $1 == '-ccflags' ]] then
        ccflags=$2
        shift; shift
    elif [[ $1 == '-ldflags' ]] then
        ldflags=$2
        shift; shift
    else
        break
    fi
done

cfile=$1

if [[ $cfile == '' || $sofile == '' ]] then
    showusage
    exit 1
fi

function mktmp {
    if [[ $SWIFTWARNLEVEL != '' ]] then
        print -u2 $PS4 in function mktmp
        set -x
    fi
    typeset fname
    fname=${TMPDIR:-/tmp}/dds.$$.$RANDOM.$1
    while [[ -f $fname ]] do
        fname=${TMPDIR:-/tmp}/$1.$$.$RANDOM.$1
    done
    > $fname
    print $fname
}

ofile=${cfile%.c}.o
regfile=${cfile%.c}.locs

CCFLAGS="$CCFLAGS $buildflag $DDS_CCFLAGS $DDS_ARCHCCFLAGS"
LDFLAGS="$LDFLAGS ${DDS_LDFLAGS//DDS_REGFILE/$regfile} $DDS_ARCHLDFLAGS"

if [[ $SWIFTWARNLEVEL == '' ]] then
    trap 'rm -f $cfile $ofile $regfile' HUP QUIT TERM KILL EXIT
else
    trap 'rm -f $ofile $regfile' HUP QUIT TERM KILL EXIT
    print -u2 $cfile
fi

prefix=$DDS_APREFIX
suffix=$DDS_ASUFFIX
incflags="$DDSEXTRACCFLAGS -I."
libflags="$DDSEXTRALDFLAGS -L."
for i in ${PATH//:/ }; do
    j=${i%bin}
    if [[ -f $j/include/swift/dds.h && -f $j/lib/${prefix}dds${suffix} ]] then
        incflags="$incflags -I$j/include/swift -I$j/include"
        libflags="$libflags -L$j/lib"
        libs="$libs -ldds -laggr -lswift"
        break
    fi
done
for i in ${PATH//:/ }; do
    j=${i%bin}
    if [[ -f $j/include/ast/ast.h && -f $j/lib/${prefix}ast${suffix} ]] then
        incflags="$incflags -I$j/include/ast"
        libflags="$libflags -L$j/lib"
        libs="$libs -lrecsort -last"
        break
    fi
done
incflags="$incflags -I."
libflags="$libflags -L."

$DDS_CC $CCFLAGS $incflags $ccflags -D_PACKAGE_ast -c $cfile -o $ofile
if [[ $? != 0 ]] then
    print -u2 "$0: compilation failed"
    exit 1
fi
$DDS_LD $LDFLAGS $libflags $ldflags -o $sofile $ofile $libs

if [[ $? != 0 ]] then
    print -u2 "$0: linking failed"
    exit 1
fi
if [[ $pixieflag == y ]] then
    pixie -dso32 -pixie_file $sofile.pixn32 $sofile
    cp -f $sofile.pixn32 .
fi

if [[ $DDS_RENAMEFROMDLL == 1 ]] then
    mv $sofile.dll $sofile
    rm -f $sofile.*
fi

exit 0
