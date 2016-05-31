
function dq_vt_autoinvtab_tool_mainopenstack {
    dq_vt_autoinvtab_tool_mainopenstack_begin
    dq_vt_autoinvtab_tool_mainopenstack_body
    dq_vt_autoinvtab_tool_mainopenstack_end
}

function dq_vt_autoinvtab_tool_mainopenstack_begin {
    typeset tbli name

    dq_vt_autoinvtab_tool_default_begin
}

function dq_vt_autoinvtab_tool_mainopenstack_end {
    dq_vt_autoinvtab_tool_default_end
}

function dq_vt_autoinvtab_tool_mainopenstack_body {
    typeset tbli tblj ik v pi li si vi ui ni
    typeset -A projs

    dq_vt_autoinvtab_tool_default_body
    tbli=${dq_vt_autoinvtab_tool_default_data.tbli}

    for ik in "${!dq_vt_autoinvtab_data.is.@}"; do
        [[ $ik != *.os_pinfo* ]] && continue
        typeset -n ip=$ik
        projs[${ik##*os_pinfo}]=(
            pinfo=$ip
            typeset -A limits images servers volumes networks users
            typeset -A p2s2i p2s2u p2s2v p2s2n
        )
    done

    for ik in "${!dq_vt_autoinvtab_data.is.@}"; do
        [[ $ik != *.os_* ]] && continue
        typeset -n ip=$ik

        case ${ik##*.} in
        os_plim*)
            pi=${ik#*os_plim}; li=${pi##*_}; pi=${pi%_*}
            projs[$pi].limits[$li]=( key=${ip%%=*} val=${ip#*=} )
            ;;
        os_sinfo*)
            pi=${ik#*os_sinfo}; si=${pi##*_}; pi=${pi%_*}
            projs[$pi].servers[$si]=( info=$ip )
            ;;
        os_iinfo*)
            pi=${ik#*os_iinfo}; ii=${pi##*_}; pi=${pi%_*}
            projs[$pi].images[$ii]=( info=$ip )
            ;;
        os_uinfo*)
            pi=${ik#*os_uinfo}; ui=${pi##*_}; pi=${pi%_*}
            projs[$pi].users[$ui]=( info=$ip )
            ;;
        os_vinfo*)
            pi=${ik#*os_vinfo}; vi=${pi##*_}; pi=${pi%_*}
            projs[$pi].volumes[$vi]=( info=$ip )
            ;;
        os_ninfo*)
            pi=${ik#*os_ninfo}; ni=${pi##*_}; pi=${pi%_*}
            projs[$pi].networks[$ni]=( info=$ip )
            ;;
        os_p2s2i*)
            pi=${ik#*os_p2s2i}; si=${pi#*_}; ii=${si#*_}
            si=${si%_*}; pi=${pi%%_*}
            projs[$pi].p2s2i[${ik#*os_p2s2i}]=( pi=$pi si=$si ii=$ii )
            ;;
        os_p2s2u*)
            pi=${ik#*os_p2s2u}; si=${pi#*_}; ui=${si#*_}
            si=${si%_*}; pi=${pi%%_*}
            projs[$pi].p2s2u[${ik#*os_p2s2u}]=( pi=$pi si=$si ui=$ui )
            ;;
        os_p2s2v*)
            pi=${ik#*os_p2s2v}; si=${pi#*_}; vi=${si#*_}
            si=${si%_*}; pi=${pi%%_*}
            projs[$pi].p2s2v[${ik#*os_p2s2v}]=( pi=$pi si=$si vi=$vi )
            ;;
        os_p2s2n*)
            pi=${ik#*os_p2s2n}; si=${pi#*_}; ni=${si#*_}
            si=${si%_*}; pi=${pi%%_*}
            projs[$pi].p2s2n[${ik#*os_p2s2n}]=( pi=$pi si=$si ni=$ni )
            ;;
        esac
    done

    if (( ${#projs[@]} > 0 )) then
        for pi in "${!projs[@]}"; do
            typeset -n pr=projs[$pi]
            dq_vt_autoinvtab_adddatarow $tbli "project|${pr.pinfo}"
            dq_vt_autoinvtab_beginsubtable $tbli "limits" ''
            tblj=${dq_vt_autoinvtab_data.tbli}
            dq_vt_autoinvtab_addheaderrow $tblj "name|value"
            for li in "${!pr.limits[@]}"; do
                typeset -n lr=projs[$pi].limits[$li]
                print -r -- "${lr.key}|right:${lr.val}"
            done | sort -t'|' -k1,1 | while read v; do
                dq_vt_autoinvtab_adddatarow $tblj "$v"
            done
            dq_vt_autoinvtab_endsubtable $tblj

            dq_vt_autoinvtab_beginsubtable $tbli "servers" ''
            tblj=${dq_vt_autoinvtab_data.tbli}
            dq_vt_autoinvtab_addheaderrow $tblj "name|image|user|volumes|nets"
            for si in "${!pr.servers[@]}"; do
                typeset -n sr=projs[$pi].servers[$si]
                v="${sr.info}|"
                for p2s2ii in "${!pr.p2s2i[@]}"; do
                    typeset -n p2s2ir=projs[$pi].p2s2i[$p2s2ii]
                    if [[ ${p2s2ir.si} == $si ]] then
                        v+=${pr.images[${p2s2ir.ii}].info}
                        break
                    fi
                done
                v+="|"
                for p2s2ui in "${!pr.p2s2u[@]}"; do
                    typeset -n p2s2ur=projs[$pi].p2s2u[$p2s2ui]
                    if [[ ${p2s2ur.si} == $si ]] then
                        v+=${pr.users[${p2s2ur.ui}].info}
                        break
                    fi
                done
                v+="|"
                for p2s2vi in "${!pr.p2s2v[@]}"; do
                    typeset -n p2s2vr=projs[$pi].p2s2v[$p2s2vi]
                    if [[ ${p2s2vr.si} == $si ]] then
                        [[ $v != *'|' ]] && v+=' '
                        v+=${pr.volumes[${p2s2vr.vi}].info}
                    fi
                done
                v+="|"
                for p2s2ni in "${!pr.p2s2n[@]}"; do
                    typeset -n p2s2nr=projs[$pi].p2s2n[$p2s2ni]
                    if [[ ${p2s2nr.si} == $si ]] then
                        [[ $v != *'|' ]] && v+=' '
                        v+=${pr.networks[${p2s2nr.ni}].info}
                    fi
                done
                print -r -- "$v"
            done | sort -t'|' -k1,1 | while read v; do
                dq_vt_autoinvtab_adddatarow $tblj "$v"
            done
            dq_vt_autoinvtab_endsubtable $tblj

            dq_vt_autoinvtab_beginsubtable $tbli "volumes" ''
            tblj=${dq_vt_autoinvtab_data.tbli}
            dq_vt_autoinvtab_addheaderrow $tblj "name"
            for vi in "${!pr.volumes[@]}"; do
                typeset -n vr=projs[$pi].volumes[$vi]
                print -r -- "${vr.info}"
            done | sort -t'|' -k1,1 | while read v; do
                dq_vt_autoinvtab_adddatarow $tblj "$v"
            done
            dq_vt_autoinvtab_endsubtable $tblj

            dq_vt_autoinvtab_beginsubtable $tbli "images" ''
            tblj=${dq_vt_autoinvtab_data.tbli}
            dq_vt_autoinvtab_addheaderrow $tblj "name"
            for ii in "${!pr.images[@]}"; do
                typeset -n ir=projs[$pi].images[$ii]
                print -r -- "${ir.info}"
            done | sort -t'|' -k1,1 | while read v; do
                dq_vt_autoinvtab_adddatarow $tblj "$v"
            done
            dq_vt_autoinvtab_endsubtable $tblj

            dq_vt_autoinvtab_beginsubtable $tbli "networks" ''
            tblj=${dq_vt_autoinvtab_data.tbli}
            dq_vt_autoinvtab_addheaderrow $tblj "name"
            for ni in "${!pr.networks[@]}"; do
                typeset -n nr=projs[$pi].networks[$ni]
                print -r -- "${nr.info}"
            done | sort -t'|' -k1,1 | while read v; do
                dq_vt_autoinvtab_adddatarow $tblj "$v"
            done
            dq_vt_autoinvtab_endsubtable $tblj

            dq_vt_autoinvtab_beginsubtable $tbli "users" ''
            tblj=${dq_vt_autoinvtab_data.tbli}
            dq_vt_autoinvtab_addheaderrow $tblj "name"
            for ui in "${!pr.users[@]}"; do
                typeset -n ur=projs[$pi].users[$ui]
                print -r -- "${ur.info}"
            done | sort -t'|' -k1,1 | while read v; do
                dq_vt_autoinvtab_adddatarow $tblj "$v"
            done
            dq_vt_autoinvtab_endsubtable $tblj
        done
    fi
}
