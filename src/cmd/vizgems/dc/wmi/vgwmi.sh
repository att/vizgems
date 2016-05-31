#!/bin/ksh

typeset -A ts cf2t helpers

ts[0]='PERF_COUNTER_RAWCOUNT_HEX'
ts[256]='PERF_COUNTER_LARGE_RAWCOUNT_HEX'
ts[2816]='PERF_COUNTER_TEXT'
ts[65536]='PERF_COUNTER_RAWCOUNT'
ts[65792]='PERF_COUNTER_LARGE_RAWCOUNT'
ts[73728]='PERF_DOUBLE_RAW'
ts[4195328]='PERF_COUNTER_DELTA'
ts[4195584]='PERF_COUNTER_LARGE_DELTA'
ts[4260864]='PERF_SAMPLE_COUNTER'
ts[4523008]='PERF_COUNTER_QUEUELEN_TYPE'
ts[4523264]='PERF_COUNTER_LARGE_QUEUELEN_TYPE'
ts[5571840]='PERF_COUNTER_100NS_QUEUELEN_TYPE'
ts[6620416]='PERF_COUNTER_OBJ_TIME_QUEUELEN_TYPE'
ts[272696320]='PERF_COUNTER_COUNTER'
ts[272696576]='PERF_COUNTER_BULK_COUNT'
ts[537003008]='PERF_RAW_FRACTION'
ts[537003264]='PERF_LARGE_RAW_FRACTION'
ts[541132032]='PERF_COUNTER_TIMER'
ts[541525248]='PERF_PRECISION_SYSTEM_TIMER'
ts[542180608]='PERF_100NSEC_TIMER'
ts[542573824]='PERF_PRECISION_100NS_TIMER'
ts[543229184]='PERF_OBJ_TIME_TIMER'
ts[543622400]='PERF_PRECISION_OBJECT_TIMER'
ts[549585920]='PERF_SAMPLE_FRACTION'
ts[557909248]='PERF_COUNTER_TIMER_INV'
ts[558957824]='PERF_100NSEC_TIMER_INV'
ts[574686464]='PERF_COUNTER_MULTI_TIMER'
ts[575735040]='PERF_100NSEC_MULTI_TIMER'
ts[591463680]='PERF_COUNTER_MULTI_TIMER_INV'
ts[592512256]='PERF_100NSEC_MULTI_TIMER_INV'
ts[805438464]='PERF_AVERAGE_TIMER'
ts[807666944]='PERF_ELAPSED_TIME'
ts[1073742336]='PERF_COUNTER_NODATA'
ts[1073874176]='PERF_AVERAGE_BULK'
ts[1073939457]='PERF_SAMPLE_BASE'
ts[1073939458]='PERF_AVERAGE_BASE'
ts[1073939459]='PERF_RAW_BASE'
ts[1073939712]='PERF_PRECISION_TIMESTAMP'
ts[1073939715]='PERF_LARGE_RAW_BASE'
ts[1107494144]='PERF_COUNTER_MULTI_BASE'
ts[2147483648]='PERF_COUNTER_HISTOGRAM_TYPE'

cf2t['Win32_ComputerSystem.TotalPhysicalMemory']=65536
cf2t['Win32_LogicalDisk.FreeSpace']=65536
cf2t['Win32_LogicalDisk.Size']=65536
cf2t['Win32_OperatingSystem.FreePhysicalMemory']=65536
cf2t['Win32_OperatingSystem.FreeSpaceInPagingFiles']=65536
cf2t['Win32_OperatingSystem.SizeStoredInPagingFiles']=65536
cf2t['Win32_PerfRawData_PerfOS_Memory.PagesInputPersec']=272696320
cf2t['Win32_PerfRawData_PerfOS_Memory.PagesOutputPersec']=272696320
cf2t['Win32_PerfRawData_PerfOS_Objects.Processes']=65536
cf2t['Win32_PerfRawData_PerfOS_Objects.Threads']=65536
cf2t['Win32_PerfRawData_PerfOS_Processor.PercentPrivilegedTime']=542180608
cf2t['Win32_PerfRawData_PerfOS_Processor.PercentProcessorTime']=558957824
cf2t['Win32_PerfRawData_PerfOS_Processor.PercentUserTime']=542180608
cf2t['Win32_PerfRawData_PerfOS_System.ProcessorQueueLength']=65536
cf2t['Win32_PerfRawData_Tcpip_NetworkInterface.PacketsOutboundErrors']=65536
cf2t['Win32_PerfRawData_Tcpip_NetworkInterface.PacketsReceivedErrors']=65536
cf2t['Win32_PerfRawData_Tcpip_NetworkInterface.PacketsReceivedPersec']=272696320
cf2t['Win32_PerfRawData_Tcpip_NetworkInterface.PacketsSentPersec']=272696320

cf2t['Win32_PerfRawData_MSSQLSERVER_SQLServerBufferManager.Buffercachehitratio']=537003264
cf2t['Win32_PerfRawData_MSSQLSERVER_SQLServerBufferManager.Freepages']=65792
cf2t['Win32_PerfRawData_MSSQLSERVER_SQLServerBufferManager.ReadaheadpagesPersec']=272696576
cf2t['Win32_PerfRawData_MSSQLSERVER_SQLServerDatabases.ActiveTransactions']=65792
cf2t['Win32_PerfRawData_MSSQLSERVER_SQLServerDatabases.LogCacheHitRatio']=537003264
cf2t['Win32_PerfRawData_MSSQLSERVER_SQLServerDatabases.PercentLogUsed']=65792
cf2t['Win32_PerfRawData_MSSQLSERVER_SQLServerGeneralStatistics.UserConnections']=65792
cf2t['Win32_PerfRawData_MSSQLSERVER_SQLServerLatches.AverageLatchWaitTimems']=1073874176
cf2t['Win32_PerfRawData_MSSQLSERVER_SQLServerLocks.AverageWaitTimems']=1073874176
cf2t['Win32_PerfRawData_MSSQLSERVER_SQLServerLocks.LockRequestsPersec']=272696576
cf2t['Win32_PerfRawData_MSSQLSERVER_SQLServerLocks.NumberofDeadlocksPersec']=272696576
cf2t['Win32_PerfRawData_MSSQLSERVER_SQLServerMemoryManager.MemoryGrantsPending']=65792
cf2t['Win32_PerfRawData_MSSQLSERVER_SQLServerMemoryManager.TotalServerMemoryKB']=65792

helpers['time']=Timestamp_PerfTime
helpers['freq']=Frequency_PerfTime
helpers['nsec']=Timestamp_Sys100NS

while [[ $1 == -* ]] do
    case $1 in
    -h) host=$2; shift 2 ;;
    -t) tfile=$2; shift 2 ;;
    -l) lfile=$2; shift 2 ;;
    -s) sfile=$2; shift 2 ;;
    -i) ifile=$2; shift 2 ;;
    -*) break ;;
    esac
done

if [[ $sfile != '' && $sfile != */* ]] then
    sfile=./$sfile
fi

typeset -A pvals nvals
if [[ $sfile != '' && -f $sfile ]] then
    . $sfile
fi

> wmi.err

typeset -A classes

export WMICPASSWD=$WMIPASSWD

if [[ $ifile != '' ]] then
    ifs="$IFS"
    IFS='|'
    if [[ $ifile == - ]] then
        cat
    else
        cat $ifile
    fi | while read class iname except field; do
        if [[ ${classes[$class]} == '' ]] then
            classes[$class]=( class=$class typeset -A specs )
        fi
        classes[$class].specs[$iname]=(
            name=$iname except=${except//_OR_/\|} field=$field
        )
    done
    for class in "${!classes[@]}"; do
        typeset -n classr=classes[$class]
        status=0
        unset i2n n2i vals
        typeset -A i2n n2i vals
        wmic --delimiter '_BAR_BAR_' -U "$WMIUSER" \
            //$host "select * from $class" \
        2>> wmi.err | egrep -v 'failed NT status' | while read line; do
            if [[ $status == 0 ]] then
                if [[ $line == CLASS:* ]] then
                    status=1
                fi
            elif [[ $status == 1 ]] then
                status=2
                line=${line//\|/.}
                line=${line//_BAR_BAR_/\|}
                set -f
                set -A ls -- $line
                set +f
                for li in "${!ls[@]}"; do
                    i2n[$li]=${ls[$li]}
                    n2i[${ls[$li]}]=$li
                done
            elif [[ $status == 2 ]] then
                status=2
                line=${line//\|/.}
                line=${line//_BAR_BAR_/\|}
                set -f
                set -A ls -- $line
                set +f
                for li in "${!ls[@]}"; do
                    vals[${i2n[$li]}]=${ls[$li]}
                done
                for iname in "${!classr.specs[@]}"; do
                    typeset -n specr=classes[$class].specs[$iname]
                    if [[ ${specr.except} != '' ]] then
                        if [[ ${vals['Name']} == ~(i)@(${specr.except}) ]] then
                            continue
                        fi
                    fi
                    val=
                    if [[ ${specr.field} != '' ]] then
                        val=${vals[${specr.field}]}
                    fi
                    print "${specr.name}|${vals['Name']}|$val"
                done
            fi
        done
    done
    IFS="$ifs"
elif [[ $tfile != '' ]] then
    typeset -F3 cval pval tval ptval fval pfval sval psval
    ifs="$IFS"
    IFS='|'
    while read mname mtype munit var mt except label; do
        class=${var%%.*}
        rest=${var##"$class"?(.)}
        field=${rest%%.*}
        inst=${rest##"$field"?(.)}
        if [[ ${classes[$class]} == '' ]] then
            classes[$class]=( class=$class typeset -A specs )
        fi
        classes[$class].specs[$mname]=(
            mname=$mname mtype=$mtype munit=$munit
            var=$var mt=$mt except=${except//_OR_/\|} label=$label
            field=$field inst=$inst
        )
    done < $tfile
    for class in "${!classes[@]}"; do
        typeset -n classr=classes[$class]
        status=0
        unset i2n n2i vals
        typeset -A i2n n2i vals
        wmic --delimiter '_BAR_BAR_' -U "$WMIUSER" \
            //$host "select * from $class" \
        2>> wmi.err | egrep -v 'failed NT status' | while read line; do
            if [[ $status == 0 ]] then
                if [[ $line == CLASS:* ]] then
                    status=1
                fi
            elif [[ $status == 1 ]] then
                status=2
                line=${line//\|/.}
                line=${line//_BAR_BAR_/\|}
                set -f
                set -A ls -- $line
                set +f
                for li in "${!ls[@]}"; do
                    i2n[$li]=${ls[$li]}
                    n2i[${ls[$li]}]=$li
                done
            elif [[ $status == 2 ]] then
                status=2
                line=${line//\|/.}
                line=${line//_BAR_BAR_/\|}
                set -f
                set -A ls -- $line
                set +f
                for li in "${!ls[@]}"; do
                    vals[${i2n[$li]}]=${ls[$li]}
                done
                for mname in "${!classr.specs[@]}"; do
                    typeset -n specr=classes[$class].specs[$mname]
                    if [[ ${specr.except} != '' ]] then
                        if [[ ${vals['Name']} == ~(i)@(${specr.except}) ]] then
                            continue
                        fi
                    fi
                    if [[ ${specr.field} == '' ]] then
                        continue
                    fi
                    if [[ ${specr.inst} == ALL ]] then
                        inst=${vals['Name']}
                    else
                        if [[ ${vals['Name']} != "${specr.inst}" ]] then
                            continue
                        fi
                        inst=${specr.inst}
                    fi
                    cf=$class.${specr.field}
                    cval=${vals[${specr.field}]}
                    t=${ts[${cf2t[$cf]}]}
                    [[ ${specr.mt} != '' ]] && t=${specr.mt}
                    case $t in
                    PERF_COUNTER_RAWCOUNT|PERF_COUNTER_LARGE_RAWCOUNT|simple)
                        val=$cval
                        ;;
                    PERF_LARGE_RAW_FRACTION|withbase)
                        bval=${vals[${specr.field}_Base]}
                        if [[ $cval == '' || $bval == '' ]] then
                            continue
                        fi
                        if (( bval == 0)) then
                            continue
                        fi
                        if (( (val = 100.0 * (cval / bval)) < 0 )) then
                            if (( val > -1 )) then
                                val=0
                            else
                                continue
                            fi
                        fi
                        ;;
                    PERF_AVERAGE_BULK)
                        bval=${vals[${specr.field}_Base]}
                        typeset -n pvalr=pvals[${specr.var}]
                        if [[ $cval == '' || $bval == '' ]] then
                            continue
                        fi
                        nvals[${specr.var}]=(
                            val=$cval bval=$bval
                        )
                        pval=${pvals[${specr.var}].val}
                        pbval=${pvals[${specr.var}].bval}
                        if [[ $pval == '' || $pbval == '' ]] then
                            continue
                        fi
                        if (( cval < pval || bval == 0 )) then
                            continue
                        fi
                        if (( (val = (cval - pval) / bval) < 0 )) then
                            if (( val > -1 )) then
                                val=0
                            else
                                continue
                            fi
                        fi
                        ;;
                    PERF_COUNTER_COUNTER|PERF_COUNTER_BULK_COUNT)
                        tval=${vals[${helpers['time']}]}
                        fval=${vals[${helpers['freq']}]}
                        typeset -n pvalr=pvals[${specr.var}]
                        if [[
                            $cval == '' || $tval == '' || $fval == ''
                        ]] then
                            continue
                        fi
                        nvals[${specr.var}]=(
                            val=$cval tval=$tval fval=$fval
                        )
                        pval=${pvals[${specr.var}].val}
                        ptval=${pvals[${specr.var}].tval}
                        pfval=${pvals[${specr.var}].fval}
                        if [[
                            $pval == '' || $ptval == '' || $pfval == ''
                        ]] then
                            continue
                        fi
                        if ((
                            cval < pval || tval <= ptval || fval == 0
                        )) then
                            continue
                        fi
                        if (( (val = (cval - pval) / (
                            (tval - ptval) / fval
                        )) < 0 )) then
                            if (( val > -1 )) then
                                val=0
                            else
                                continue
                            fi
                        fi
                        ;;
                    PERF_100NSEC_TIMER|PERF_100NSEC_TIMER_INV)
                        sval=${vals[${helpers['nsec']}]}
                        typeset -n pvalr=pvals[${specr.var}]
                        if [[ $cval == '' || $sval == '' ]] then
                            continue
                        fi
                        nvals[${specr.var}]=( val=$cval sval=$sval )
                        pval=${pvals[${specr.var}].val}
                        psval=${pvals[${specr.var}].sval}
                        if [[ $pval == '' || $psval == '' ]] then
                            continue
                        fi
                        if (( cval < pval || sval <= psval )) then
                            continue
                        fi
                        if [[ $t == PERF_100NSEC_TIMER ]] then
                            if (( (val = 100.0 * (
                                (cval - pval) / (sval - psval)
                            )) < 0 )) then
                                if (( val > -1 )) then
                                    val=0
                                else
                                    continue
                                fi
                            fi
                        else
                            if (( (val = 100.0 * (
                                1.0 - (cval - pval) / (sval - psval)
                            )) < 0 )) then
                                if (( val > -1 )) then
                                    val=0
                                else
                                    continue
                                fi
                            fi
                        fi
                        ;;
                        *)
                            print -u2 SWIFT ERROR Unknown type for ${specr}
                            ;;
                    esac
                    print -n "rt=STAT type=${specr.mtype}"
                    print -n " name=${specr.mname} unit=${specr.munit}"
                    print " num=$val label=\"${specr.label}\""
                done
            fi
        done
    done
    IFS="$ifs"
    typeset -p nvals | sed 's/-A nvals/-A pvals/' \
    > $sfile.tmp && mv $sfile.tmp $sfile
else
    ifs="$IFS"
    IFS='|'
    while read mname file srcs etyps; do
        nvals[$mname]=${pvals[$mname]}
        srcs=${srcs//:/\|}
        etyps=${etyps//:/\|}
        status=0
        unset i2n n2i vals
        typeset -A i2n n2i vals
        reci=${pvals[$mname]}
        first=n
        if [[ $reci == '' ]] then
            first=y
        fi
        haverec=n
        if [[ $reci == '' ]] then
            wmic --delimiter '_BAR_BAR_' -U "$WMIUSER" //$host \
                "select * from Win32_NTLogEvent where LogFile='$file'" \
            2>> wmi.err | egrep -v 'failed NT status'
        else
            wmic --delimiter '_BAR_BAR_' -U "$WMIUSER" //$host \
                "select * from Win32_NTLogEvent \
                    where LogFile='$file' and RecordNumber >= $reci\
                " \
            2>> wmi.err | egrep -v 'failed NT status'
        fi | while read line; do
            if [[ $haverec == y && $first == y ]] then
                break
            fi
            if [[ $status == 0 ]] then
                if [[ $line == CLASS:* ]] then
                    status=1
                fi
            elif [[ $status == 1 ]] then
                status=2
                line=${line//\|/.}
                line=${line//_BAR_BAR_/\|}
                set -f
                set -A ls -- $line
                set +f
                for li in "${!ls[@]}"; do
                    i2n[$li]=${ls[$li]}
                    n2i[${ls[$li]}]=$li
                done
            elif [[ $status == 2 ]] then
                status=2
                line=${line//\|/.}
                line=${line//_BAR_BAR_/\|}
                set -f
                set -A ls -- $line
                set +f
                for li in "${!ls[@]}"; do
                    vals[${i2n[$li]}]=${ls[$li]//\'/\"}
                done
                src=${vals['SourceName']}
                etyp=${vals['EventType']}
                recj=${vals['RecordNumber']}
                haverec=y
                if (( reci < recj )) then
                    (( reci = recj ))
                fi
                nvals[$mname]=$reci
                if [[ $recj == ${pvals[$mname]} ]] then
                    continue
                fi
                if [[ $srcs != '' && $src != @($srcs) ]] then
                    continue
                fi
                if [[ $etyps != '' && $etyp != @($etyps) ]] then
                    continue
                fi
                case $etyp in
                1) sev=1; typ=ALARM ;;
                2) sev=4; typ=ALARM ;;
                3) sev=5; typ=ALARM ;;
                4) sev=5; typ=CLEAR ;;
                5) sev=1; typ=ALARM ;;
                esac
                code=${vals['EventCode']}
                print -n "rt=ALARM alarmid='src' sev=$sev type=$typ tech=WMI"
                print " cond='$file-$src-$code' txt='$txt'"
            fi
        done
        if [[ $haverec != y ]] then
            nvals[$mname]=0
        fi
        print -n "rt=STAT type=counter name=$mname unit="
        print " num=${nvals[$mname]}"
    done < $lfile
    IFS="$ifs"
    typeset -p nvals | sed 's/-A nvals/-A pvals/' \
    > $sfile.tmp && mv $sfile.tmp $sfile
fi
