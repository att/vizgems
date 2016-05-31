
function suireadcmd { # $1 = fd $2 = cmd $3 = arguments
    typeset ifs fd _cmd _args line

    ifs="$IFS"
    IFS=''
    set -f
    fd=$1
    _cmd=$2
    _args=$3
    if ! read -r -u$fd line; then
        IFS="$ifs"
        return 1
    fi
    if [[ $line != begin ]] then
        eval $_cmd='$line'
        eval $_args='( foo=bar )'
        IFS="$ifs"
        return
    fi
    unset $_cmd
    eval $_cmd=
    unset $_args
    eval $_args='( cmd= )'
    suireadcmdrec $fd $_args
    eval $_cmd="\${$_args.cmd}"
    set +f
    IFS="$ifs"
}

function suireadcmdrec { # $1 = fd $2 = prefix
    typeset ifs line key val x fd=$1 _args=$2
    integer i=0

    ifs="$IFS"
    IFS=''
    set -f
    while read -r -u$fd line; do
        if [[ $line == ']' || $line == end ]] then
            IFS="$ifs"
            return
        fi
        key=${line%%=*}
        val=${line#"$key"}
        if [[ $val == "" ]] then
            typeset -n x=$_args._$i
            x="$key"
            ((i++))
            typeset +n x
        elif [[ $val != '=[' ]] then
            val=${val#=}
            if [[ $key == [0-9]* ]] then
                key=_$key
            fi
            typeset -n x=$_args.$key
            x="$val"
            typeset +n x
        else
            typeset -n x=$_args.$key
            typeset +n x
            suireadcmdrec $fd $_args.$key
        fi
    done
    set +f
    IFS="$ifs"
}

function suiwritecmd { # $1 = fd $2 = cmd $3 = arguments
    typeset -n x=$3

    print "${x}" | sed -e 's/^(/begin/' -e 's/^)/end/' \
        -e 's/^	*_[0-9][0-9]*=//' -e 's/(/[/' -e 's/)/]/' \
    -e 's/^	*//' | sed -E "s/='(.*)'$/=\1/" 1>&$1
    typeset +n x
}
