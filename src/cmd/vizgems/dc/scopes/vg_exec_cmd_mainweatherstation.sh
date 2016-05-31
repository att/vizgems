
targetname=$1
targetaddr=$2
targettype=$3

typeset -A names args

ifs="$IFS"
IFS='|'
set -f
while read -r name type unit impl; do
    names[$name]=( name=$name type=$type unit=$unit impl=$impl typeset -A args )
    IFS='&'
    set -A al -- ${impl#*\?}
    unset as; typeset -A as
    for a in "${al[@]}"; do
        k=${a%%=*}
        v=${a#*=}
        v=${v//'+'/' '}
        v=${v//@([\'\\])/'\'\1}
        eval "v=\$'${v//'%'@(??)/'\x'\1"'\$'"}'"
        v=${v//%26/'&'}
        args[$k]=$v
        names[$name].args[$k]=$v
    done
    IFS='|'
done
set +f
IFS="$ifs"

user=$EXECUSER pass=$EXECPASS

host=$targetaddr

if [[ ${args[path]} == '' ]] then
    print "rt=ALARM sev=1 type=ALARM tech=weatherstation txt=\"path not specified for weatherstation collection\""
    exit 0
fi
path=${args[path]}

# emit stats

typeset -A emitted

typeset -A info=(
    [temp_in]=( n=sensor_temp.in l="Inside Temperature" u=F t=number )
    [temp_out]=( n=sensor_temp.out l="Outside Temperature" u=F t=number )
    [humid_in]=( n=sensor_humi.in l="Inside Humidity" u=% t=number )
    [humid_out]=( n=sensor_humi.out l="Outside Humidity" u=% t=number )
    [windsp_avg_10min]=( n=sensor_wind.out l="Wind Speed" u=mph t=number )
)

function genstats {
    typeset -A emitbykey

    typeset ifs f fnd nowt mint maxt t pat key namei

    typeset unix ts bar_3hf pkt next_rec barom_in barom_hPa temp_in humid_in
    typeset temp_out windsp windsp_avg_10min wind_dir humid_out rain_rate
    typeset uv_index solar storm_rain storm_date storm_date_bits rain_day
    typeset rain_mon rain_year rain_day_et rain_mon_et rain_year_et alarm_in
    typeset alarm_rain alarm_out bat_trasm bat_console forecast_ico sunrise
    typeset sunset

    f='rt=STAT name="%s" rti=%ld num=%.3f unit="%s" type=%s label="%s"'
    fnd='rt=STAT name="%s" nodata=2'
    nowt=$(printf '%(%#)T')
    (( maxt = nowt + 60 ))
    (( mint = nowt - 4 * VG_JOBIV ))

    pat=
    for (( i = 10; i >= 1; i-- )) do
        if [[ ${mint:0:$i} == ${maxt:0:$i} ]] then
            pat="^${mint:0:$i}"
            break
        fi
    done
    if [[ $pat == '' ]] then
        print -u2 ERROR cannot find time prefix
        return 1
    fi

    SSHPASS=$pass vgssh "$user@$host" "egrep '$pat' $path" > stats.csv

    ifs="$IFS"
    IFS='|'
    while read \
        unix ts bar_3hf pkt next_rec barom_in barom_hPa temp_in humid_in \
        temp_out windsp windsp_avg_10min wind_dir humid_out rain_rate uv_index \
        solar storm_rain storm_date storm_date_bits rain_day rain_mon \
        rain_year rain_day_et rain_mon_et rain_year_et alarm_in alarm_rain \
        alarm_out bat_trasm bat_console forecast_ico sunrise sunset; \
    do
        (( $unix < $mint || $unix > $maxt )) && continue
        for key in \
            unix ts bar_3hf pkt next_rec barom_in barom_hPa temp_in humid_in \
            temp_out windsp windsp_avg_10min wind_dir humid_out rain_rate \
            uv_index solar storm_rain storm_date storm_date_bits rain_day \
            rain_mon rain_year rain_day_et rain_mon_et rain_year_et alarm_in \
            alarm_rain alarm_out bat_trasm bat_console forecast_ico sunrise \
            sunset; \
        do
            [[ ${info[$key]} == '' ]] && continue
            typeset -n val=$key
            typeset -n ir=info[$key]
            t=$unix
            if (( emitted[$key].last < t )) then
                if [[ ${ir.t} == number ]] then
                    printf "$f\n" "${ir.n}" $t $val "${ir.u}" number "${ir.l}"
                elif [[ ${ir.t} == counter && ${emitted[$key].val} != '' ]] then
                    (( dv = val - emitted[$key].val ))
                    printf "$f\n" "${ir.n}" $t $dv "${ir.u}" number "${ir.l}"
                else
                    printf "$fnd\n" "${ir.n}"
                fi
                emitted[$key]=( last=$t val=$val )
                emitbykey[${ir.n}]=1
            fi
        done
    done < stats.csv
    IFS="$ifs"

    for namei in "${!names[@]}"; do
        [[ ${emitbykey[$namei]} == 1 ]] && continue
        printf "$fnd\n" "$namei"
    done

    (( t = VG_JOBTS - 25 * 60 * 60 ))
    for key in "${!emitted[@]}"; do
        (( emitted[$key].last < t )) && unset emitted[$key]
    done

    typeset -p emitted > emitted.sh.tmp && mv emitted.sh.tmp emitted.sh
}

case $INVMODE in
y)
    ;;
*)
    [[ -f emitted.sh ]] && . ./emitted.sh

    genstats
    ;;
esac
