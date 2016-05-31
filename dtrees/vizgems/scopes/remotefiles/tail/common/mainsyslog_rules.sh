
rules=(
    [syslog]=(
        sev=4
        tmode=keep
        counted=y
        [exclude]=(
            [0]=(
                tool=kernel
                txt='*@(vhci_hcd|ALSA|apparmor|AppArmor|IPVS: Creat|Modules linked|\[<*>\]*0x|audit*callbacks)*'
            )
            [1]=(
                tool='systemd*'
                txt='*@([Ss]ession|Reloaded|Start|Stop|Created|slice|seat|buttons|target|Received SIGRTMIN)*'
            )
            [2]=(
                tool=sshd
                txt='*@(opened|closed|Accepted|disconnected by user)*'
            )
            [3]=( tool='cron*' txt='*@(CMD|REPLACE|opened|closed| info )*' )
            [4]=( tool='@(tls_prune|cyr_expire|master|ctl_cyrusdb)' txt='*' )
            [5]=( tool='dnsmasq*' txt='*' )
            [6]=( tool='ovs*' txt='*INFO*' )
            [7]=( tool='snmptrapd' txt='*' )
            [8]=( tool=ntpd txt='*' )
            [9]=( tool='gdm*' txt='*' )
            [10]=( tool='os-prober' txt='*debug:*' )
            [11]=( tool=hp-upgrade txt='*' )
            [12]=( tool='dbus*' txt='*' )
            [13]=( tool='org.freedesktop*' txt='*' )
            [14]=( tool='org.a11y*' txt='*' )
            [15]=( tool=su txt='*cyrus*' )
            [16]=( tool=irqbalance txt='*affinity_hint subset empty*' )
        )
        [include]=(
            [0]=(
                tool=kernel
                txt='*@(error| sd |Sense|unknown partition)*'
                sev=1 tmode=keep
            )
        )
    )
)
