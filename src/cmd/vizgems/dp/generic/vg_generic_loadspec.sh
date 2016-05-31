
spec=()

function genschemas { # $1 = dir
    for schemai in "${!spec.schemas[@]}"; do
        typeset -n schemar=spec.schemas[$schemai]
        {
            print "name ${schemar.name}"
            for (( fieldi = 0; fieldi < ${#schemar.fields[@]}; fieldi++ )) do
                typeset -n fieldr=schemar.fields[$fieldi]
                print "field ${fieldr.name} ${fieldr.type} ${fieldr.size}"
            done
        } > $1/lib/dds/${schemar.name}.schema
    done
}

function vg_generic_loadspec { # $1 = spec name
    typeset tmpdir

    if [[ $1 == '' ]] then
        print -u2 SWIFT ERROR no spec specified
        exit 1
    fi

    if [[ $GENERICPATH != '' ]] then
        if [[ \
            -f $GENERICPATH/bin/spec.sh && -f $VG_DSYSTEMDIR/generic/$1.sh && \
            $GENERICPATH/bin/spec.sh -nt $VG_DSYSTEMDIR/generic/$1.sh \
        ]] then
            if \
                cmp $GENERICPATH/bin/spec.sh $VG_DSYSTEMDIR/generic/$1.sh \
                > /dev/null 2>&1; \
            then
                tw -d $GENERICPATH touch
                return 0
            fi
        fi
    fi

    if ! command . $VG_DSYSTEMDIR/generic/$1.sh; then
        print -u2 SWIFT ERROR cannot load spec $1
        exit 1
    fi

    tmpdir=$VG_DSYSTEMDIR/tmp/generic/$1.$$
    mkdir -p $tmpdir/bin $tmpdir/lib/dds
    if [[ ! -d $tmpdir ]] then
        print -u2 SWIFT ERROR cannot create tmp dir $tmpdir
        exit 1
    fi

    genschemas $tmpdir

    cp $VG_DSYSTEMDIR/generic/$1.sh $tmpdir/bin/spec.sh
    export PATH=$tmpdir/bin:$PATH
    export GENERICPATH=$tmpdir
}
