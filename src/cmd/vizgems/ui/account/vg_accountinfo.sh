
. vg_hdr

ifs="$IFS"
IFS=';'

if [[ $1 == -usernames ]] then
    while read -r id n rest; do
        print "$id|$n"
    done < $SWMROOT/accounts/current | sort -t'|'
elif [[ $1 == -dump ]] then
    while read -r id n rest; do
        id=$id
        if [[ $rest == *vg_hevopsview* ]] then
            uflag=y
        else
            uflag=n
        fi
        rest=${rest//\|/\;}
        type=$VG_ACCOUNT_TYPE_CUST
        for attr in $rest; do
            k=${attr%%=*}
            [[ $k != vg_groups ]] && continue
            v=${attr##"$k"?(=)}
            [[ :$v: == *:vg_att_*:* ]] && type=$VG_ACCOUNT_TYPE_ATT
        done
        for attr in $rest; do
            k=${attr%%=*}
            v=${attr##"$k"?(=)}
            if [[ $k == vg_level_* ]] then
                if [[ $v == *:* ]] then
                    v="${v//:/'|'}"
                    [[ $uflag == y ]] && v="$v|UNKNOWN"
                    v="($v)"
                elif [[ $uflag == y && $v != __ALL__ ]] then
                    v="($v|UNKNOWN)"
                fi
                print "$id|$type|${k#vg_level_}|$v"
            fi
        done
    done < $SWMROOT/accounts/current | sort -t'|'
else
    while read -r id n rest; do
        if [[ $id != $1 ]] then
            continue
        fi
        print id=$id
        for attr in ${rest//'|'/';'}; do
            k=${attr%%=*}
            v=${attr##"$k"?(=)}
            case $k in
            vg_name)
                print name=$v
                ;;
            vg_comments)
                print info=$v
                ;;
            vg_level_*)
                print $k=$v
                ;;
            vg_groups)
                for group in ${v//':'/';'}; do
                    print group=$group
                done
                ;;
            vg_cmid)
                print cmid=$v
            esac
        done
    done < $SWMROOT/accounts/current
fi

IFS="$ifs"
