
function vg_recgen_stat { # $1: rec, $2: stats $3: flags
    typeset rec=$1 sstr=$2 flags=$3

    typeset -n rref=$rec
    typeset -n ss=$sstr
    typeset sis si sn=${#ss[@]}

    typeset -A gs prevs nexts vals
    typeset gi nexti vname ti lv id alvid rti s vflag label date file
    typeset -lF3 dv

    if [[ -f ./stat.state.$VG_JOBID ]] then
        . ./stat.state.$VG_JOBID || { rm -f ./stat.state.$VG_JOBID; return 1; }
    fi

    if [[ " $flags " == *' 'havereals' '* ]] then
        for (( si = 0; si < sn; si++ )) do
            typeset -n sref=ss[$si]
            [[ $sref == '' ]] && continue
            [[ ${sref.nodata} != '' ]] && continue

            if [[ ${sref.alvids} != '' ]] then
                for alvid in "${sref.alvids}"; do
                    for rti in ${sref.rtis:-$VG_JOBTS}; do
                        gs[$rti:$alvid]+=" $si"
                    done
                done
            fi
            if [[ ${sref.rid} != '' ]] then
                for rti in ${sref.rtis:-$VG_JOBTS}; do
                    gs[$rti:${sref.rlevel}:${sref.rid}]+=" $si"
                done
            else
                for rti in ${sref.rtis:-$VG_JOBTS}; do
                    gs[$rti:${rref.target.level}:${rref.target.name}]+=" $si"
                done
            fi
        done
    else
        for (( si = 0; si < sn; si++ )) do
            typeset -n sref=ss[$si]
            [[ $sref == '' ]] && continue
            [[ ${sref.nodata} != '' ]] && continue
            gs[default]+=" $si"
        done
    fi

    date=$(printf '%(%Y.%m.%d.%H.%M.%S)T' \#$VG_JOBTS) || return 1
    file=$VG_DSCOPESDIR/outgoing/stat/stats.$date.$VG_JOBID.$VG_SCOPENAME.xml
    {
        for gi in "${!gs[@]}"; do
            sis=${gs[$gi]}
            if [[ $gi == default ]] then
                ti=$VG_JOBTS
                lv=${rref.target.level}
                id=${rref.target.name}
            else
                ti=${gi%%:*}
                gi=${gi#$ti:}
                lv=${gi%%:*}
                id=${gi#"$lv":}
            fi
            print -n "<stat>"
            print -n "<v>${rref.version}</v>"
            print -n "<jid>$VG_JOBID</jid>"
            print -n "<lv>$lv</lv><id>$id</id>"
            print -n "<sid>${rref.scope.name}</sid>"
            print -n "<ti>$ti</ti>"
            print -n "<vars>"
            for si in $sis; do
                typeset -n sref=ss[$si]
                vflag=n
                s="<var>"
                if [[ $gi != default && ${sref.name} == *._id_$id ]] then
                    s+="<k>${sref.name%"._id_$id"}</k>"
                else
                    s+="<k>${sref.name}</k>"
                fi
                if [[ ${sref.label} != '' ]] then
                    label="${sref.label//'<'/%3C}"
                    s+="<l>${label//'>'/%3E}</l>"
                fi
                if [[ $ti == $VG_JOBTS ]] then
                    typeset -n v=ss[$si].num
                    vname=${sref.name}
                else
                    typeset -n v=ss[$si].num_$ti
                    vname=${sref.name}:$ti
                fi
                [[ $v == '' ]] && continue
                s+="<t>${sref.type}</t>"
                case ${sref.type} in
                counter)
                    if [[ ${vals[$vname]} != '' ]] then
                        vflag=y
                    else
                        dv=0
                        if [[ ${prevs[${sref.name}]} != '' ]] then
                            (( dv = v - ${prevs[${sref.name}]} ))
                            if ((
                                dv >= 0.0 && VG_JOBTS - p_ts < 3 * VG_JOBIV
                            )) then
                                s+="<n>$dv</n>"
                                vflag=y
                            fi
                        fi
                        nexts[${sref.name}]=$v
                        prevs[${sref.name}]=$v
                        v=$dv
                        vals[$vname]=y
                    fi
                    ;;
                string)
                    ;;
                *)
                    s+="<n>$v</n>"
                    vflag=y
                    ;;
                esac
                s+="<u>${sref.unit}</u>"
                s+="</var>"
                [[ ${sref.norep} == '' && $vflag == y ]] && print -rn "$s"
            done
            print -n "</vars>"
            print "</stat>"
        done
        if (( ${#nexts[@]} > 0 )) then
            for nexti in "${!nexts[@]}"; do
                print -r -u3 -- "prevs['$nexti']=${nexts[$nexti]}"
            done
            print -r -u3 -- "p_ts=$VG_JOBTS"
        fi
    } > ./stat.file.$VG_JOBID 3> ./stat.state.$VG_JOBID || return 1
    cp ./stat.file.$VG_JOBID $file.tmp && mv $file.tmp $file
}
