typeset -A vg_units
vg_units[none]=1
vg_units['H%']=0.01
vg_units['%']=1
vg_units['FR']=1
vg_units[B]=1
vg_units[KB]=1024
vg_units[kB]=1024
vg_units[MB]=$(( 1024 * 1024 ))
vg_units[GB]=$(( 1024 * 1024 * 1024 ))
vg_units[bps]=1
vg_units[kbps]=1000
vg_units[mbps]=1000000
vg_units[Bps]=1
vg_units[KBps]=1024
vg_units[MBps]=$(( 1024 * 1024 ))
vg_units[ms]=1
vg_units[cs]=10
vg_units[ds]=100
vg_units[s]=1000

typeset vg_ucnum vg_ucunit

function vg_unitconv { # $1: value $2 in_units $3 out_units
    typeset -lF3 v=$1
    typeset iu=$2 ou=$3

    if [[ ${vg_units[$iu]} == '' || ${vg_units[$ou]} == '' ]] then
        print -u2 vg_unitconv: cannot convert from $iu to $ou
        return 1
    fi

    (( v = (v * ${vg_units[$iu]} * 1.0) / ${vg_units[$ou]} ))
    vg_ucnum=$v
    vg_ucunit=$ou
    return 0
}
