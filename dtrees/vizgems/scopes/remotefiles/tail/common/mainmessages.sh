
n=$(uname -n)
n=${n%%.*}
t=$(uname -s)

set -o noglob
case $t in
Linux*)
    ./vgtail /var/log/messages "" ./messages.state.$n
    ;;
IRIX*)
    ./vgtail /var/adm/SYSLOG /var/adm/oSYSLOG ./messages.state.$n
    ;;
Darwin*)
    ./vgtail /var/log/system.log "" ./messages.state.$n
    ;;
esac \
| while read -r line; do
    set -f
    set -A ls -- $line
    set +f
    if [[ ${ls[0]} == *[0-9]T[0-9]* ]] then
        line="X X $line"
    fi
    set -f
    set -A ls -- $line
    set +f
    id=${ls[3]#[0-9][A-Z]:}
    tool=${ls[4]//\[*\]/}
    tool=${tool//\(*\)/}
    tool=${tool%:}
    i=5
    for (( j = 0; j < i; j++ )) do
        unset ls[$j]
    done
    txt="${ls[*]}"
    case "$txt" in
    *"Can't contact LDAP server"*) continue ;;
    *"message repeated"*times*) continue ;;
    *@(successful)*)
        aid= type=ALARM sev=5 tmode=defer
        ;;
    *)
        aid= type=ALARM sev=1 tmode=keep
        ;;
    esac
    case "$tool" in
    *sshd)
        case $txt in
        *@(Accepted|tty modes failed|[Aa]uthentication failure)*) continue ;;
        *@(subsystem request|session opened|session closed)*) continue ;;
        *@([Ff]ailed password for)*) continue ;;
        *@(syslogin_perform_logout|Received disconnect)*) continue ;;
        *@(session_by_pid: unknown|Unknown error|uudecode)*) continue ;;
        esac
        aid=sshd type=ALARM sev=1 tmode=keep
        ;;
    *pure-ftpd)
        case $txt in
        *@(NOTICE|INFO)*) continue ;;
        esac
        aid=ftpd type=ALARM sev=4 tmode=drop
        ;;
    *ftpd)
        case $txt in
        *@(connection from|FTP LOGIN FROM|timed out)*) continue ;;
        esac
        aid=ftpd type=ALARM sev=3 tmode=keep
        ;;
    *cron*|*CRON*)
        case $txt in
        *@(CMD|LIST|BEGIN|END|REPLACE|RELOAD|INFO)*) continue ;;
        esac
        aid=cron type=ALARM sev=3 tmode=keep
        ;;
    *kernel)
        case $txt in
        *@(state change|floating-point|unaligned access|npviewer)*) continue ;;
        *@(Gyration|USB|usb|QUEUE FULL detected|vhci_hcd)*) continue ;;
        *@(Context seems to be stalled - )*) continue ;;
        esac
        aid=kernel type=ALARM sev=1 tmode=keep
        case $txt in
        *@(dks|I/O error|xfs_)*)
            aid=storage type=ALARM sev=1 tmode=keep
            txt="storage issue: $txt"
            ;;
        esac
        ;;
    *unix)
        case $txt in
        *@(state change|floating-point|unaligned access|npviewer)*) continue ;;
        esac
        aid=unix type=ALARM sev=1 tmode=keep
        case $txt in
        *@(dks|I/O error|xfs_)*)
            aid=storage type=ALARM sev=1 tmode=keep
            txt="storage issue: $txt"
            ;;
        esac
        ;;
    *sudo)
        case $txt in
        @(cjolsen |ek )*) continue ;;
        *@(pam_unix|pam_systemd)*) continue ;;
        esac
        aid=su type=ALARM sev=1 tmode=keep
        ;;
    *su)
        case $txt in
        *@( ek | cjolsen )*"to root"*) continue ;;
        *"to root"*@( ek | cjolsen )*) continue ;;
        *"(to root) root on (null)"*) continue ;;
        *"to root"*) ;;
        *) continue ;;
        esac
        aid=su type=ALARM sev=1 tmode=keep
        ;;
    *smartd)
        case $txt in
        *Usage*changed*|*changed*from*) continue ;;
        esac
        aid=smartd type=ALARM sev=3 tmode=keep
        ;;
    *named)
        case $txt in
        *transfer*started*|*'end of transfer'*) continue ;;
        esac
        aid=named type=ALARM sev=3 tmode=keep
        ;;
    *vmd)
        case $txt in
        *terminating*successful*) continue ;;
        esac
        aid=backup type=ALARM sev=3 tmode=keep
        ;;
    *auditd)
        case $txt in
        *'Audit daemon rotating log files'*) continue ;;
        esac
        aid=auditd type=ALARM sev=1 tmode=keep
        ;;
    *xinetd)
        case $txt in
        *@(START:|EXIT:|Reading|Starting|removing|Reconfigured:)*) continue ;;
        *@(Swapping|readjusting|Started|Exiting)*) continue ;;
        esac
        aid=xinetd type=ALARM sev=4 tmode=drop
        ;;
    *yum)
        case $txt in
        *@(Updated)*) continue ;;
        esac
        aid=yum type=ALARM sev=4 tmode=drop
        ;;
    *polkitd)
        case $txt in
        *@(Registered|Unregistered|successfully)*) continue ;;
        esac
        aid=xinetd type=ALARM sev=4 tmode=drop
        ;;
    *com.apple.backupd*)
        continue
        ;;
    *--)
        case $txt in
        *@(MARK)*) continue ;;
        esac
        aid=xinetd type=ALARM sev=4 tmode=drop
        ;;
    systemd*)
        case $txt in
        *@(Removed session|New session|temporary hack|Session)*)
            continue
            ;;
        esac
        ;;
    ntpd*)
        case $txt in
        *@(Listen|ntp_io|ntpd|refreshed|proto:|step time)*)
            continue
            ;;
        esac
        ;;
    master*)
        case $txt in
        *@(exited|about to|egister)*)
            continue
            ;;
        esac
        ;;
    *@(syslog|zmd|ctl_mboxlist|sendmail|rdistd|fsr|xdm|Xsession)*) continue ;;
    *@(rld|avahi|usb|pulseaudio|mtp-probe|eventmond|availmon)*) continue ;;
    *@(SQLAnywhere|bluetoothd|dbus-daemon|ctl_cyrusdb|snmptrapd)*) continue ;;
    *@(thttpd|dbus|cyr_expire|kcheckpass|ntpdate|tls_prune|pcscd)*) continue ;;
    *@(hp-firmware|hp-upgrade|kdm|logger|logrotate|org.gnome)*) continue ;;
    *@(org.gtk|org.kde|org.a11y|org.fedorahosted|pop3|rtkit)*) continue ;;
    *) ;;
    esac

    txt="$tool $txt"
    txt=${txt//%/__P__}
    txt=${txt//' '/%20}
    txt=${txt//'+'/%2B}
    txt=${txt//';'/%3B}
    txt=${txt//'&'/%26}
    txt=${txt//'|'/%7C}
    txt=${txt//\'/%27}
    txt=${txt//\"/%22}
    txt=${txt//'<'/%3C}
    txt=${txt//'>'/%3E}
    txt=${txt//\\/%5C}
    txt=${txt//$'\n'/' '}
    txt=${txt//$'\r'/' '}
    txt=${txt//__P__/%25}

    print -r "ID=$id:AID=$aid:TYPE=$type:SEV=$sev:TMODE=$tmode:TECH=SYSLOG:TXT=$txt"
done
