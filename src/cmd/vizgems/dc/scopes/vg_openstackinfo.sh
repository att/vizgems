
usage=$'
[-1p1?
@(#)$Id: vg_openstackinfo (AT&T) 2007-03-25 $
]
[+NAME?vg_openstackinfo - print information about an openstack installation]
[+DESCRIPTION?\bvg_openstackinfo\b queries the specified openstack site
and prints out any information specified by options.
]
[200:host?specifies the DNS name or IP address of the server to query.
]:[host]
[300:user?speficies the user name to use.
]:[user]
[301:pass?speficies the user password to use.
]:[pass]
[400:proj?prints out information about projects.
]
[999:v?increases the verbosity level. May be specified multiple times.]
'

function showusage {
    OPTIND=0
    getopts -a vg_openstackinfo "$usage" opt '-?'
}

host=
user= pass=
showps=

while getopts -a vg_openstackinfo "$usage" opt; do
    case $opt in
    200) host=$OPTARG ;;
    300) user=$OPTARG ;;
    301) pass=$OPTARG ;;
    400) showps=y ;;
    999) (( SWIFTWARNLEVEL++ )) ;;
    *) showusage; exit 1 ;;
    esac
done
shift $OPTIND-1

if [[ ${host} == '' ]] then
    print -u2 "ERROR no host name specified"
    exit 0
fi

if [[ ${user} == '' ]] then
    print -u2 "ERROR no user id specified"
    exit 0
fi

if [[ ${pass} == '' ]] then
    print -u2 "ERROR no user password specified"
    exit 0
fi

authurl=http://$host:5000/v2.0
invurl=http://$host:5000/v3
computeurl=http://$host:8774/v2
volumeurl=http://$host:8776/v2
statsurl=http://$host:8777/v2/samples

uid=
aid=

typeset -A projects groups users
typeset -A seenimages seenservers seenvolumes seenusers seengroups
typeset -A seengs seenss
typeset -A invinfo emitted prevemitted id2is
typeset invinfochanged=n id2ii=0

# authentication

function gettokenbyname { # $1 = user $2 = pass $3 = tenant name
    typeset user=$1 pass=$2 tenant=$3

    typeset tk

    eval $(
        curl -s -X POST \
            -H "Content-type: application/json" -H "Accept: application/json" \
        $authurl/tokens -d "{
          \"auth\": {
            \"tenantName\": \"$tenant\",
            \"passwordCredentials\": {
              \"username\": \"$user\",
              \"password\": \"$pass\"
            }
          }
        }" | vgjson2ksh tk
    )

    uid=${tk.access.user.id}
    aid=${tk.access.token.id}
}

function gettokenbyid { # $1 = user $2 = pass $3 = tenant id
    typeset user=$1 pass=$2 tenant=$3

    typeset tk

    eval $(
        curl -s -X POST \
            -H "Content-type: application/json" -H "Accept: application/json" \
        $authurl/tokens -d "{
          \"auth\": {
            \"tenantId\": \"$tenant\",
            \"passwordCredentials\": {
              \"username\": \"$user\",
              \"password\": \"$pass\"
            }
          }
        }" | vgjson2ksh tk
    )

    uid=${tk.access.user.id}
    aid=${tk.access.token.id}
}

# users and groups

function getgroups { # no args
    typeset gr i id name did descr

    eval $(
        curl -s \
            -H "X-Auth-Token: $aid" -H "Accept: application/json" \
        $invurl/groups | vgjson2ksh gr
    )

    for (( i = 0; ; i++ )) do
        typeset -n r=gr.groups.[$i]
        [[ ${r.id} == '' ]] && break

        id=${r.id}
        name=${r.name}
        did=${r.domain_id}
        descr=${r.description}
        [[ $descr == '' ]] && descr=$name

        groups[$id]=( name=$name id=$id descr=$descr did=$did )
    done
}

function getusers { # no args
    typeset ur i id name enabled email did dpid

    eval $(
        curl -s \
            -H "X-Auth-Token: $aid" -H "Accept: application/json" \
        $invurl/users | vgjson2ksh ur
    )

    for (( i = 0; ; i++ )) do
        typeset -n r=ur.users.[$i]
        [[ ${r.id} == '' ]] && break

        id=${r.id}
        name=${r.name}
        enabled=${r.enabled}
        email=${r.email}
        did=${r.domain_id}
        dpid=${r.default_project_id}

        users[$id]=(
            name=$name id=$id enabled=$enabled email=$email did=$did dpid=$dpid
            typeset -A projects groups
        )

        getuserprojects "$id"
        getusergroups "$id"
    done
}

function getuserprojects { # $1 = uid
    typeset uid=$1
    typeset -n pr=users[$uid].projects

    typeset upr i pid name

    eval $(
        curl -s \
            -H "X-Auth-Token: $aid" -H "Accept: application/json" \
        $invurl/users/$uid/projects | vgjson2ksh upr
    )

    for (( i = 0; ; i++ )) do
        typeset -n r=upr.projects.[$i]
        [[ ${r.id} == '' ]] && break

        pid=${r.id}
        name=${r.name}

        pr[$pid]=( name=$name id=$pid )
    done
}

function getusergroups { # $1 = uid
    typeset uid=$1
    typeset -n gr=users[$uid].groups

    typeset ugr i gid name

    eval $(
        curl -s \
            -H "X-Auth-Token: $aid" -H "Accept: application/json" \
        $invurl/users/$1/groups | vgjson2ksh ugr
    )

    for (( i = 0; ; i++ )) do
        typeset -n r=ugr.groups.[$i]
        [[ ${r.id} == '' ]] && break

        gid=${r.id}
        name=${r.name}

        gr[$gid]=( name=$name id=$gid )
    done
}

# projects

function getprojects { # no args
    typeset pr i id name enabled did descr

    eval $(
        curl -s \
            -H "X-Auth-Token: $aid" -H "Accept: application/json" \
        $invurl/projects | vgjson2ksh pr
    )

    for (( i = 0; ; i++ )) do
        typeset -n r=pr.projects.[$i]
        [[ ${r.id} == '' ]] && break

        id=${r.id}
        name=${r.name}
        enabled=${r.enabled}
        did=${r.domain_id}
        descr=${r.description}
        [[ $descr == '' ]] && descr=$name

        projects[$id]=(
            name=$name id=$id enabled=$enabled descr=$descr did=$did
            typeset -A limits servers volumes images meters
            typeset -A networks subnets routers ports
        )
    done
}

# networks and subnets, attach to projects

function getnetworks { # no args
    typeset nt i j pid id name status shared state type segid external phys bid

    eval $(
        curl -s \
            -H "X-Auth-Token: $aid" -H "Accept: application/json" \
        $networkurl/networks | vgjson2ksh nt
    )

    for (( i = 0; ; i++ )) do
        typeset -n r=nt.networks.[$i]
        [[ ${r.id} == '' ]] && break

        pid=${r.tenant_id}
        [[ ${projects[$pid]} == '' ]] && continue

        id=${r.id}
        name=${r.name}
        status=${r.status}
        shared=${r.shared}
        state=${r.admin_state_up}

        typeset -n rr=nt.networks.[$i].[provider:network_type]
        type=$rr
        typeset -n rr=nt.networks.[$i].[provider:segmentation_id]
        segid=$rr
        typeset -n rr=nt.networks.[$i].[router:external]
        external=$rr
        typeset -n rr=nt.networks.[$i].[provider:physical_network]
        phys=$rr

        [[ ${projects[$pid]} == '' ]] && continue
        projects[$pid].networks[$id]=(
            id=$id pid=$pid name=$name status=$status state=$state type=$type
            segid=$segid external=$external shared=$shared phys=$phys
            typeset -A subnets
        )

        for (( j = 0; ; j++ )) do
            typeset -n rr=nt.networks.[$i].subnets.[$j]
            [[ ${rr} == '' ]] && break

            bid=$rr
            projects[$pid].networks[$id].subnets[$bid]=( id=$bid )
        done
    done
}

function getsubnets { # no args
    typeset snt i j pid id name nid gw cidr dhcp ipv v6addrmode v6ramode
    typeset pi sip eip

    eval $(
        curl -s \
            -H "X-Auth-Token: $aid" -H "Accept: application/json" \
        $networkurl/subnets | vgjson2ksh snt
    )

    for (( i = 0; ; i++ )) do
        typeset -n r=snt.subnets.[$i]
        [[ ${r.id} == '' ]] && break

        pid=${r.tenant_id}
        [[ ${projects[$pid]} == '' ]] && continue

        id=${r.id}
        nid=${r.network_id}
        gw=${r.gateway_ip}
        cidr=${r.cidr}
        dhcp=${r.enable_dhcp}
        ipv=${r.ip_version}
        v6addrmode=${r.ipv6_address_mode}
        v6ramode=${r.ipv6_ra_mode}

        name=${r.name}
        [[ $name == '' ]] && name=$id

        projects[$pid].subnets[$id]=(
            id=$id name=$name pid=$pid nid=$nid gw=$gw cidr=$cidr
            dhcp=$dhcp ipv=$ipv v6addrmode=$v6addrmode v6ramode=$v6ramode
            typeset -A pools
        )

        pi=0
        for (( j = 0; ; j++ )) do
            typeset -n rr=snt.subnets.[$i].allocation_pools.[$j]
            [[ ${rr} == '' ]] && break

            sip=${rr.start}
            eip=${rr.end}

            if [[ $sip != '' && $eip != '' ]] then
                projects[$pid].subnets[$id].pools[$pi]=( sip=$sip eip=$eip )
                (( pi++ ))
            fi
        done
    done
}

function getrouters { # no args
    typeset rt i pid id name state

    eval $(
        curl -s \
            -H "X-Auth-Token: $aid" -H "Accept: application/json" \
        $networkurl/routers | vgjson2ksh rt
    )

    for (( i = 0; ; i++ )) do
        typeset -n r=rt.routers.[$i]
        [[ ${r.id} == '' ]] && break

        pid=${r.tenant_id}
        [[ ${projects[$pid]} == '' ]] && continue

        id=${r.id}
        state=${r.admin_state_up}

        name=${r.name}
        [[ $name == '' ]] && name=$id

        projects[$pid].routers[$id]=(
            id=$id name=$name pid=$pid state=$state
        )
    done
}

function getports { # no args
    typeset prt i j pid id name nid mac status state host pi bid ip
    typeset ownname ownid type

    eval $(
        curl -s \
            -H "X-Auth-Token: $aid" -H "Accept: application/json" \
        $networkurl/ports | vgjson2ksh prt
    )

    for (( i = 0; ; i++ )) do
        typeset -n r=prt.ports.[$i]
        [[ ${r.id} == '' ]] && break

        pid=${r.tenant_id}
        [[ ${projects[$pid]} == '' ]] && continue

        id=${r.id}

        nid=${r.network_id}
        mac=${r.mac_address}
        status=${r.status}
        state=${r.admin_state_up}
        ownid=${r.device_id}

        name=${r.name}
        [[ $name == '' ]] && name=$id

        ownname=${r.device_owner}
        case $ownname in
        compute:*)                type=server      ;;
        network:dhcp)             type=dhcp        ;;
        network:floatingip)       type=floating    ;;
        network:router_gateway)   type=routergw    ;;
        network:router_interface) type=routeriface ;;
        esac

        typeset -n rr=prt.ports.[$i].[binding:host_id]
        host=$rr

        projects[$pid].ports[$id]=(
            id=$id name=$name pid=$pid nid=$nid mac=$mac status=$status
            state=$state ownid=$ownid ownname=$ownname type=$type host=$host
            typeset -A ips
        )

        pi=0
        for (( j = 0; ; j++ )) do
            typeset -n rr=prt.ports.[$i].fixed_ips.[$j]
            [[ ${rr} == '' ]] && break

            bid=${rr.subnet_id}
            ip=${rr.ip_address}
            projects[$pid].ports[$id].ips[$pi]=( bid=$bid ip=$ip )
            (( pi++ ))
        done
    done
}

# specific project resources: limits images servers volumes networks subnets

function getlimits { # $1 = pid
    typeset pid=$1
    typeset -n lr=projects[$pid].limits

    typeset lmt i name value

    eval $(
        curl -s \
            -H "X-Auth-Token: $aid" -H "Accept: application/json" \
        $computeurl/$pid/limits | vgjson2ksh lmt
    )

    for i in "${!lmt.limits.absolute.@}"; do
        [[ ${i##*.absolute.} == *.* ]] && continue
        typeset -n r=$i

        name=${i##*.}
        value=$r

        lr[$name]=( name=$name value=$value )
    done
}

function getimages { # $1 = pid
    typeset pid=$1
    typeset -n ir=projects[$pid].images

    typeset img i id name

    eval $(
        curl -s \
            -H "X-Auth-Token: $aid" -H "Accept: application/json" \
        $computeurl/$pid/images | vgjson2ksh img
    )

    for (( i = 0; ; i++ )) do
        typeset -n r=img.images.[$i]
        [[ ${r.id} == '' ]] && break

        id=${r.id}
        name=${r.name}

        ir[$id.$pid]=( name=$name id=$id.$pid )
        getimagedetails $pid $id
    done
}

function getimagedetails { # $1 = pid, $2 = iid
    typeset pid=$1 iid=$2
    typeset -n ir=projects[$pid].images[$iid.$pid]

    typeset iinfo

    eval $(
        curl -s \
            -H "X-Auth-Token: $aid" -H "Accept: application/json" \
        $computeurl/$pid/images/$iid | vgjson2ksh iinfo
    )

    typeset -n r=iinfo.image
    [[ $r == '' ]] && break

    ir.status=${r.status}
    ir.updated=${r.updated}
    ir.created=${r.created}
    ir.mindisk=${r.minDisk}
    ir.progress=${r.progress}
    ir.minram=${r.minRam}

    typeset -n rr=iinfo.image.[OS-EXT-IMG-SIZE:size]
    ir.size=$rr

    typeset -n rr=iinfo.image.metadata.image_state
    ir.state=$rr
}

function getservers { # $1 = pid
    typeset pid=$1
    typeset -n sr=projects[$pid].servers

    typeset srv i id name

    eval $(
        curl -s \
            -H "X-Auth-Token: $aid" -H "Accept: application/json" \
        $computeurl/$pid/servers | vgjson2ksh srv
    )

    for (( i = 0; ; i++ )) do
        typeset -n r=srv.servers.[$i]
        [[ ${r.id} == '' ]] && break

        id=${r.id}
        name=${r.name}

        sr[$id]=( name=$name id=$id typeset -A addresses volumes meterinsts )
        getserverdetails $pid $id
    done
}

function getserverdetails { # $1 = pid, $2 = sid
    typeset pid=$1 sid=$2
    typeset -n pr=projects[$pid]
    typeset -n sr=projects[$pid].servers[$sid]

    typeset sinfo i j k nname nid stid rid vid ipv addr type mac ai

    eval $(
        curl -s \
            -H "X-Auth-Token: $aid" -H "Accept: application/json" \
        $computeurl/$pid/servers/$sid | vgjson2ksh sinfo
    )

    typeset -n r=sinfo.server
    [[ $r == '' ]] && break

    sr.status=${r.status}
    sr.hostid=${r.hostId}
    sr.uid=${r.user_id}
    sr.created=${r.created}
    sr.updated=${r.updated}
    sr.key_name=${r.key_name}

    typeset -n rr=sinfo.server.[OS-SRV-USG:launched_at]
    sr.launched=$rr

    typeset -n rr=sinfo.server.[OS-SRV-USG:terminated_at]
    sr.terminated=$rr

    typeset -n rr=sinfo.server.[OS-EXT-SRV-ATTR:instance_name]
    sr.instancename=$rr

    typeset -n rr=sinfo.server.[OS-EXT-SRV-ATTR:host]
    sr.host=$rr

    typeset -n rr=sinfo.server.[OS-EXT-SRV-ATTR:hypervisor_hostname]
    sr.hypervisor=$rr

    sr.iid=${r.image.id}.$pid

    ai=0
    for j in "${!sinfo.server.addresses.@}"; do
        [[ ${j#*.addresses.} == *.* ]] && continue
        for (( k = 0; ; k++ )) do
            typeset -n rr=sinfo.server.addresses.${j##*.}.[$k]
            [[ ${rr.addr} == '' ]] && break

            nname=${j##*.}
            ipv=${rr.version}
            addr=${rr.addr}

            typeset -n rrr=rr.[OS-EXT-IPS:type]
            type=$rrr

            typeset -n rrr=rr.[OS-EXT-IPS-MAC:mac_addr]
            mac=$rrr

            sr.addresses[$ai]=(
                nname=$nname ipv=$ipv addr=$addr type=$type mac=$mac
            )

            for nid in "${!pr.networks[@]}"; do
                if [[ ${pr.networks[$nid].name} == $nname ]] then
                    sr.addresses[$ai].nid=$nid
                    break
                fi
            done

            (( ai++ ))
        done
    done

    if [[ ${sinfo.server.metadata.metering} != '' ]] then
        typeset -n rr=sinfo.server.metadata.metering.stack_id
        stid=$rr
        typeset -n rr=sinfo.server.metadata.metering.resource_id
        rid=$rr
        sr.meterinsts[$stid-$rid]=(
            name=$stid-$rid mid=$stid rid=$rid
        )
        if [[ ${pr.meters[$stid]} == '' ]] then
            pr.meters[$stid]=(
                mid=$stid name=$stid typeset -A insts
            )
        fi
        pr.meters[$stid].insts[$stid-$rid]=(
            name=$stid-$rid mid=$stid iid=$rid
        )
    fi

    for (( j = 0; ; j++ )) do
        typeset -n rr=sinfo.server.[os-extended-volumes:volumes_attached].[$j]
        [[ ${rr} == '' ]] && break

        vid=${rr.id}
        sr.volumes[$vid]=( id=$vid )
    done
}

function getvolumes { # $1 = pid
    typeset pid=$1
    typeset -n vr=projects[$pid].volumes

    typeset vol i id name

    eval $(
        curl -s \
            -H "X-Auth-Token: $aid" -H "Accept: application/json" \
        $volumeurl/$pid/volumes | vgjson2ksh vol
    )

    for (( i = 0; ; i++ )) do
        typeset -n r=vol.volumes.[$i]
        [[ ${r.id} == '' ]] && break

        id=${r.id}
        name=${r.name}

        vr[$id]=( name=$name id=$id )
        getvolumedetails $pid $id
    done
}

function getvolumedetails { # $1 = pid, $2 = vid
    typeset pid=$1 vid=$2
    typeset -n vr=projects[$pid].volumes[$vid]

    typeset j

    eval $(
        curl -s \
            -H "X-Auth-Token: $aid" -H "Accept: application/json" \
        $volumeurl/$pid/volumes/$vid | vgjson2ksh vinfo
    )

    typeset -n r=vinfo.volume
    [[ $r == '' ]] && break

    vr.status=${r.status}
    vr.created=${r.created_at}
    vr.size=${r.size}

    vr.descr=${r.description}
    [[ ${vr.descr} == '' ]] && vr.descr=${vr.name}

    typeset -n rr=vinfo.volume.[os-vol-host-attr:host]
    vr.host=$rr

    for (( j = 0; ; j++ )) do
        typeset -n rr=vinfo.volume.attachments.[$j]
        [[ ${rr} == '' ]] && break

        vr.device=${rr.device}
        vr.sid=${rr.server_id}
        vr.vdid=${vr.device#'/dev/'}
    done
}

# emit inventory

function genasset { # $1 = pid, $2 = id, $3 = type, $4 = name var
    typeset pid=$1 id=$2 type=$3 var=$4

    typeset pname=${projects[$pid].name}
    typeset -n name=$var

    #print "edge|o|$AID|o|$id|mode|virtual"
    print "edge|o|$id|o|$id|mode|virtual"

    if [[ ${seengs[$pid]} == '' ]] then
        print "node|g|g:$pid|name|Group $pname"
        print "node|g|g:$pid|nodename|Group $pname"
        print "node|g|g:$pid|groupname|Group $pname"
        seengs[$pid]=y
    fi
    if [[ ${seenss[$pid]} == '' ]] then
        print "node|s|s:$pid|name|Site $pname"
        print "node|s|s:$pid|nodename|Site $pname"
        seenss[$pid]=y
    fi

    print "map|o|$id|b|$bizid||"
    print "map|o|$id|c|$projid||"
    print "map|o|$id|g|g:$pid||"
    print "map|g|g:$pid|s|s:$pid||"
    print "map|s|s:$pid|c|$projid||"
    print "map|s|s:$pid|l|$locid||"

    print "node|o|$id|ostype|$type"
    case $type in
    IMG) print "node|o|$id|shape|box"       ;;
    FS)  print "node|o|$id|shape|folder"    ;;
    VM)  print "node|o|$id|shape|box"       ;;
    U)   print "node|o|$id|shape|signature" ;;
    NET) print "node|o|$id|shape|box"       ;;
    SUB) print "node|o|$id|shape|box"       ;;
    RTR) print "node|o|$id|shape|box"       ;;
    PRT) print "node|o|$id|shape|box"       ;;
    esac

    invinfo[$id]=( id=$id inst='' label="$type:$name" )
}

function genimage { # $1 = iid $2 = var
    typeset iid=$1
    typeset -n ir=$2

    typeset ai

    [[ ${seenimages[$iid]} != '' ]] && return
    seenimages[$iid]=y

    for ai in "${!ir.@}"; do
        typeset -n ar=$ai
        if [[ ${ai##*.} == name ]] then
            print "node|o|$iid|name|I:$ar"
            print "node|o|$iid|nodename|I:$ar"
        else
            print "node|o|$iid|${ai##*.}|$ar"
        fi
    done
}

function genserver { # $1 = sid, $2 = pid, $3 = var
    typeset sid=$1
    typeset pid=$2
    typeset -n sr=$3

    typeset miid vid ai mii=0

    [[ ${seenservers[$sid]} != '' ]] && return
    seenservers[$sid]=y

    print "node|o|$sid|name|VM:${sr.name}"
    print "node|o|$sid|nodename|VM:${sr.name}"
    print "node|o|$sid|instancename|${sr.instancename}"
    print "node|o|$sid|keyname|${sr.key_name}"
    print "node|o|$sid|created|${sr.created}"
    print "node|o|$sid|updated|${sr.updated}"
    print "node|o|$sid|launched|${sr.launched}"
    print "node|o|$sid|terminated|${sr.terminated}"
    print "node|o|$sid|host|${sr.host}"
    print "node|o|$sid|hostid|${sr.hostid}"
    print "node|o|$sid|hypervisor|${sr.hypervisor}"
    print "node|o|$sid|status|${sr.status}"
    print "node|o|$sid|iid|${sr.iid}"
    print "node|o|$sid|uid|${sr.uid}"
    if [[ ${sr.hypervisor} == +([0-9.]) ]] then
        print "edge|o|$sid|o|${sr.hypervisor}|ostype|vm2hyp"
    else
        print "edge|o|$sid|o|${sr.hypervisor%%.*}|ostype|vm2hyp"
    fi
    mii=0
    for miid in "${!sr.meterinsts[@]}"; do
        print "node|o|$sid|miid$mii|$miid"
        print "edge|o|$sid|o|$miid|ostype|vm2mi"
        (( mii++ ))
    done

    invinfo[${sr.instancename}-$sid]=( id=$sid inst='' label="VM:${sr.name}" )

    print "edge|o|$pid|o|$sid|mode|virtual"
    print "edge|o|$sid|o|${sr.iid}|ostype|vm2img"
    print "edge|o|${sr.uid}|o|$sid|ostype|u2vm"
    seenusers[${sr.uid}]=y

    for ai in "${!sr.addresses[@]}"; do
        typeset -n ar=sr.addresses[$ai]
        print "node|o|$sid|ip_addr$ai|${ar.addr}"
        print "node|o|$sid|ip_mac$ai|${ar.mac}"
        print "node|o|$sid|ip_type$ai|${ar.type}"
        print "node|o|$sid|ip_version$ai|${ar.ipv}"
        print "node|o|$sid|ip_nid$ai|${ar.nid}"
    done

    for vid in "${!sr.volumes[@]}"; do
        typeset -n vr=sr.volumes[$vid]
        print "edge|o|$sid|o|$vid|ostype|vm2fs"
    done
}

function genvolume { # $1 = vid $2 = var
    typeset vid=$1
    typeset -n vr=$2

    typeset ai

    [[ ${seenvolumes[$vid]} != '' ]] && return
    seenvolumes[$vid]=y

    for ai in "${!vr.@}"; do
        typeset -n ar=$ai
        if [[ ${ai##*.} == name ]] then
            print "node|o|$vid|name|FS:$ar"
            print "node|o|$vid|nodename|FS:$ar"
        else
            print "node|o|$vid|${ai##*.}|$ar"
        fi
        if [[ ${ai##*.} == host && $ar != '' ]] then
            print "edge|o|$vid|o|${ar%%[.#]*}|ostype|fs2hyp"
        fi
    done

    if [[ ${vr.sid} == '' || ${vr.device} != */vd* ]] then
        return
    fi

    invinfo[${vr.sid}-${vr.device##*/}]=(
        id=${vr.sid} inst=${vr.device##*/} label="FS:${vr.name}"
    )
}

function gennetwork { # $1 = nid $2 = var
    typeset nid=$1
    typeset -n nr=$2

    typeset ai

    [[ ${seennetworks[$nid]} != '' ]] && return
    seennetworks[$nid]=y

    for ai in "${!nr.@}"; do
        [[ ${ai##"$2".} == @(subnets|id|pid) ]] && continue
        [[ ${ai##"$2".} == *.* ]] && continue
        typeset -n ar=$ai
        if [[ ${ai##"$2".} == name ]] then
            print "node|o|$nid|name|NET:$ar"
            print "node|o|$nid|nodename|NET:$ar"
        else
            print "node|o|$nid|${ai##*.}|$ar"
        fi
    done
}

function gensubnet { # $1 = bid $2 = var
    typeset bid=$1
    typeset -n br=$2

    typeset ai

    [[ ${seensubnets[$bid]} != '' ]] && return
    seensubnets[$bid]=y

    for ai in "${!br.@}"; do
        [[ ${ai##"$2".} == @(pools|id|pid) ]] && continue
        [[ ${ai##"$2".} == *.* ]] && continue
        typeset -n ar=$ai
        if [[ ${ai##"$2".} == name ]] then
            print "node|o|$bid|name|SUB:$ar"
            print "node|o|$bid|nodename|SUB:$ar"
        else
            print "node|o|$bid|${ai##*.}|$ar"
        fi
    done
}

function genrouter { # $1 = rtid $2 = var
    typeset rtid=$1
    typeset -n rtr=$2

    typeset ai

    [[ ${seenrouters[$rtid]} != '' ]] && return
    seenrouters[$rtid]=y

    for ai in "${!rtr.@}"; do
        [[ ${ai##"$2".} == @(id|pid) ]] && continue
        [[ ${ai##"$2".} == *.* ]] && continue
        typeset -n ar=$ai
        if [[ ${ai##"$2".} == name ]] then
            print "node|o|$rtid|name|RT:$ar"
            print "node|o|$rtid|nodename|RT:$ar"
        else
            print "node|o|$rtid|${ai##*.}|$ar"
        fi
    done
}

function genport { # $1 = prtid $2 = var
    typeset prtid=$1
    typeset -n prtr=$2

    typeset ai

    [[ ${seenports[$prtid]} != '' ]] && return
    seenports[$prtid]=y

    for ai in "${!prtr.@}"; do
        [[ ${ai##"$2".} == @(ips|id|pid) ]] && continue
        [[ ${ai##"$2".} == *.* ]] && continue
        typeset -n ar=$ai
        if [[ ${ai##"$2".} == name ]] then
            print "node|o|$prtid|name|PRT:$ar"
            print "node|o|$prtid|nodename|PRT:$ar"
        else
            print "node|o|$prtid|${ai##*.}|$ar"
        fi
    done
}

function genmeter { # $1 = mid $2 = var
    typeset mid=$1
    typeset -n mr=$2

    typeset ai

    [[ ${seenmeters[$mid]} != '' ]] && return
    seenmeters[$mid]=y

    for ai in "${!mr.@}"; do
        [[ ${ai##"$2".} == @(insts) ]] && continue
        [[ ${ai##"$2".} == *.* ]] && continue
        typeset -n ar=$ai
        if [[ ${ai##"$2".} == name ]] then
            print "node|o|$mid|name|M:$ar"
            print "node|o|$mid|nodename|M:$ar"
        else
            print "node|o|$mid|${ai##*.}|$ar"
        fi
    done
}

function genmeterinst { # $1 = iid $2 = var
    typeset iid=$1
    typeset -n ir=$2

    typeset ai

    [[ ${seenmeterinsts[$iid]} != '' ]] && return
    seenmeterinsts[$iid]=y

    for ai in "${!ir.@}"; do
        [[ ${ai##"$2".} == *.* ]] && continue
        typeset -n ar=$ai
        if [[ ${ai##"$2".} == name ]] then
            print "node|o|$iid|name|M:$ar"
            print "node|o|$iid|nodename|M:$ar"
        else
            print "node|o|$iid|${ai##*.}|$ar"
        fi
    done
}

function geninventory {
    typeset gid uid pid lid iid sid vid mid nid bid prtid rid
    typeset ai pi vi si li mi ii ui gi ni bi ri vdid mac ipi prti

    for pid in "${!projects[@]}"; do
        [[ ${projects[$pid].emit} != y ]] && continue

        typeset -n pr=projects[$pid]
        genasset "$pid" "$pid" "CUS" "projects[$pid].name"
        print "node|o|$AID|si_projectid$pid|$pid"
        pi=${id2is[$pid]}
        if [[ $pi == '' ]] then
            (( pi = id2ii++ ))
            id2is[$pid]=$pi
        fi
        print "node|o|$AID|os_pinfo$pi|${pr.name}"
        print "node|o|$pid|name|C:${pr.name}"
        print "node|o|$pid|nodename|C:${pr.name}"
        print "node|o|$pid|descr|${pr.descr}"
        print "node|o|$pid|enabled|${pr.enabled}"
        print "node|o|$pid|did|${pr.did}"
        li=0
        for lid in "${!pr.limits[@]}"; do
            typeset -n lr=pr.limits[$lid]
            print "node|o|$AID|os_plim${pi}_$(( li++ ))|${lr.name}=${lr.value}"
            print "node|o|$pid|l_${lr.name}|${lr.value}"
        done
        for iid in "${!pr.images[@]}"; do
            typeset -n ir=pr.images[$iid]
            ii=${id2is[$iid]}
            if [[ $ii == '' ]] then
                (( ii = id2ii++ ))
                id2is[$iid]=$ii
            fi
            print "node|o|$AID|os_iinfo${pi}_$ii|${ir.name}"
            genasset "$pid" "$iid" "IMG" "projects[$pid].images[$iid].name"
            #print "edge|o|$pid|o|$iid|ostype|c2i"
            genimage "$iid" "projects[$pid].images[$iid]"
        done
        for vid in "${!pr.volumes[@]}"; do
            typeset -n vr=pr.volumes[$vid]
            vi=${id2is[$vid]}
            if [[ $vi == '' ]] then
                (( vi = id2ii++ ))
                id2is[$vid]=$vi
            fi
            print "node|o|$AID|os_vinfo${pi}_$vi|${vr.name}"
            genasset "$pid" "$vid" "FS" "projects[$pid].volumes[$vid].name"
            print "edge|o|$pid|o|$vid|mode|virtual"
            genvolume "$vid" "projects[$pid].volumes[$vid]"
        done
        for nid in "${!pr.networks[@]}"; do
            typeset -n nr=pr.networks[$nid]
            ni=${id2is[$nid]}
            if [[ $ni == '' ]] then
                (( ni = id2ii++ ))
                id2is[$nid]=$ni
            fi
            print "node|o|$AID|os_ninfo${pi}_$ni|${nr.name}"
            genasset "$pid" "$nid" "NET" "projects[$pid].networks[$nid].name"
            #print "edge|o|$pid|o|$nid|ostype|c2n"
            gennetwork "$nid" "projects[$pid].networks[$nid]"
        done
        for bid in "${!pr.subnets[@]}"; do
            typeset -n br=pr.subnets[$bid]
            bi=${id2is[$bid]}
            if [[ $bi == '' ]] then
                (( bi = id2ii++ ))
                id2is[$bid]=$bi
            fi
            print "node|o|$AID|os_binfo${pi}_$bi|${br.name}"
            genasset "$pid" "$bid" "SUB" "projects[$pid].subnets[$bid].name"
            print "edge|o|$bid|o|${br.nid}|ostype|sub2net"
            gensubnet "$bid" "projects[$pid].subnets[$bid]"
        done
        for rid in "${!pr.routers[@]}"; do
            typeset -n rr=pr.routers[$rid]
            ri=${id2is[$rid]}
            if [[ $ri == '' ]] then
                (( ri = id2ii++ ))
                id2is[$rid]=$ri
            fi
            print "node|o|$AID|os_rinfo${pi}_$ri|${rr.name}"
            genasset "$pid" "$rid" "RTR" "projects[$pid].routers[$rid].name"
            genrouter "$rid" "projects[$pid].routers[$rid]"
        done
        for prtid in "${!pr.ports[@]}"; do
            typeset -n prtr=pr.ports[$prtid]
            prti=${id2is[$prtid]}
            if [[ $prti == '' ]] then
                (( prti = id2ii++ ))
                id2is[$prtid]=$prti
            fi
            print "node|o|$AID|os_prtinfo${pi}_$prti|${prtr.name}"
            genasset "$pid" "$prtid" "PRT" "projects[$pid].ports[$prtid].name"
            for ipi in "${!prtr.ips[@]}"; do
                typeset -n ipr=prtr.ips[$ipi]
                print "edge|o|$prtid|o|${ipr.bid}|ostype|prt2sub"
            done
            case ${prtr.type} in
            server)
                if [[ ${pr.servers[${prtr.ownid}]} != '' ]] then
                    print "edge|o|${prtr.ownid}|o|$prtid|ostype|vm2prt"
                fi
                ;;
            dhcp)
                ;;
            floating)
                ;;
            routergw|routeriface)
                if [[ ${pr.routers[${prtr.ownid}]} != '' ]] then
                    print "edge|o|${prtr.ownid}|o|$prtid|ostype|rtr2prt"
                fi
                ;;
            esac
            if [[ ${prtr.host} != '' ]] then
                print "edge|o|$prtid|o|${prtr.host%%[.#]*}|ostype|prt2hyp"
            fi
            genport "$prtid" "projects[$pid].ports[$prtid]"
        done
        for sid in "${!pr.servers[@]}"; do
            typeset -n sr=pr.servers[$sid]
            si=${id2is[$sid]}
            if [[ $si == '' ]] then
                (( si = id2ii++ ))
                id2is[$sid]=$si
            fi
            ii=${id2is[${sr.iid}]}
            if [[ $ii == '' ]] then
                (( ii = id2ii++ ))
                id2is[${sr.iid}]=$ii
            fi
            ui=${id2is[${sr.uid}]}
            if [[ $ui == '' ]] then
                (( ui = id2ii++ ))
                id2is[${sr.uid}]=$ui
            fi
            print "node|o|$AID|os_sinfo${pi}_$si|${sr.name}"
            print "node|o|$AID|os_p2s2i${pi}_${si}_$ii|${pi}_${si}_$ii"
            print "node|o|$AID|os_p2s2u${pi}_${si}_$ui|${pi}_${si}_$ui"
            print "node|o|$AID|si_sindex_id_$sid|_id_$sid"

            for vid in "${!sr.volumes[@]}"; do
                typeset -n vr=sr.volumes[$vid]
                vi=${id2is[$vid]}
                print "node|o|$AID|os_p2s2v${pi}_${si}_$vi|${pi}_${si}_$vi"
                print "edge|o|$sid|o|$vid|ostype|vm2fs"
            done

            for ai in "${!sr.addresses[@]}"; do
                typeset -n ar=sr.addresses[$ai]
                [[ ${ar.nid} == '' ]] && continue
                ni=${id2is[${ar.nid}]}
                print "node|o|$AID|os_p2s2n${pi}_${si}_$ni|${pi}_${si}_$ni"
                mac=${ar.mac}
                print "node|o|$AID|si_saindex$mac._id_$sid|$mac._id_$sid"
                #print "edge|o|$sid|o|${ar.nid}|ostype|vm2net"
            done

            genasset "$pid" "$sid" "VM" "projects[$pid].servers[$sid].name"
            print "edge|o|$pid|o|$sid|mode|virtual"
            genserver "$sid" "$pid" "projects[$pid].servers[$sid]"
            invinfo[$sid-vda]=( id=$sid inst=vda label="FS:root" )
            print "node|o|$AID|si_svindexvda._id_$sid|vda._id_$sid"
            for vid in "${!sr.volumes[@]}"; do
                typeset -n vr=projects[$pid].volumes[$vid]
                [[ ${vr.vdid} == '' ]] && continue
                vdid=${vr.vdid}
                print "node|o|$AID|si_svindex$vdid._id_$sid|$vdid._id_$sid"
            done
        done
        for mid in "${!pr.meters[@]}"; do
            typeset -n mr=pr.meters[$mid]
            mi=${id2is[$mid]}
            if [[ $mi == '' ]] then
                (( mi = id2ii++ ))
                id2is[$mid]=$mi
            fi
            print "node|o|$AID|os_minfo${pi}_$mi|${mr.name}"
            genasset "$pid" "$mid" "MET" "projects[$pid].meters[$mid].name"
            #print "edge|o|$pid|o|$stid|ostype|c2m"
            genmeter "$mid" "projects[$pid].meters[$mid]"
            for iid in "${!mr.insts[@]}"; do
                typeset -n ir=pr.meters[$mid].insts[$iid]
                ii=${id2is[$iid]}
                if [[ $ii == '' ]] then
                    (( ii = id2ii++ ))
                    id2is[$iid]=$ii
                fi
                print "node|o|$AID|os_miinfo${pi}_${mi}_$ii|${ir.name}"
                genasset \
                    "$pid" "$iid" "MI" \
                "projects[$pid].meters[$mid].insts[$iid].name"
                #print "edge|o|$mid|o|$iid|ostype|m2mi"
                genmeterinst "$iid" "projects[$pid].meters[$mid].insts[$iid]"
            done
        done
    done

    for uid in "${!users[@]}"; do
        [[ ${seenusers[$uid]} != y ]] && continue
        typeset -n ur=users[$uid]
        ui=${id2is[$uid]}
        if [[ $ui == '' ]] then
            (( ui = id2ii++ ))
            id2is[$uid]=$ui
        fi
        print "node|o|$uid|name|U:${ur.name}"
        print "node|o|$uid|nodename|U:${ur.name}"
        print "node|o|$uid|enabled|${ur.enabled}"
        print "node|o|$uid|did|${ur.did}"
        for gid in "${!ur.groups[@]}"; do
            typeset -n gr=ur.groups[$gid]
            print "edge|o|${gr.id}|o|$uid|ostype|g2u"
            seengroups[${gr.id}]=y
        done
        [[ ${ur.dpid} != '' ]] && print "node|o|$uid|defpid|${ur.dpid}"
        for pid in "${!ur.projects[@]}"; do
            [[ ${projects[$pid].emit} != y ]] && continue
            typeset -n pr=ur.projects[$pid]
            print "node|o|$AID|os_uinfo${id2is[$pid]}_$ui|${ur.name}"
            genasset "$pid" "$uid" "U" "users[$uid].name"
            print "edge|o|${pr.id}|o|$uid|mode|virtual"
        done
    done

    for gid in "${!groups[@]}"; do
        [[ ${seengroups[$gid]} != y ]] && continue
        typeset -n gr=groups[$gid]
        print "node|o|$gid|name|G:${gr.name}"
        print "node|o|$gid|nodename|G:${gr.name}"
        print "node|o|$gid|descr|${gr.descr}"
        print "node|o|$gid|did|${gr.did}"
    done

    typeset -p invinfo > invinfo.sh.tmp && mv invinfo.sh.tmp invinfo.sh
}

# emit stats

function genstats { # $1 = pid
    typeset pid=$1

    typeset -A emitbykey emitbyid

    typeset url stat key t rid id mi name key val label l f mac inst unit namei
    typeset type mdk dv i
    typeset -u u

    f='rt=STAT name="%s" rid="%s" rti=%ld num=%.3f unit="%s" type=%s label="%s"'
    fnd='rt=STAT name="%s" nodata=2'
    t=$(TZ=UTC printf '%(%Y-%m-%dT%H:%M:%S)T' "#$(( VG_JOBTS - 2 * VG_JOBIV ))")

    for namei in "${!names[@]}"; do
        print rt=STAT name="$namei" nodata=2
    done

    eval $(
        url="$statsurl?q.field=timestamp&q.op=ge&q.value=$t"
        url+="&q.field=project_id&q.op=eq&q.value=$pid"
        curl -s -H "X-Auth-Token: $aid" -H "Accept: application/json" "$url" \
        | vgjson2ksh stats
    )

    for (( i = 0; ; i++ )) do
        typeset -n r=stats.[$i]
        [[ ${r.id} == '' ]] && break

        rid=${r.resource_id}
        id=$rid inst='' name=$rid
        if [[ ${invinfo[$rid]} != '' ]] then
            id=${invinfo[$rid].id} inst=${invinfo[$rid].inst}
            name=${invinfo[$rid].label}
        elif [[ ${invinfo[$rid.$pid]} != '' ]] then
            id=${invinfo[$rid.$pid].id} inst=${invinfo[$rid.$pid].inst}
            name=${invinfo[$rid.$pid].label}
        else
            if [[
                $rid == instance-*-tap*-* && ${invinfo[${rid%-tap*}]} != ''
            ]] then
                mac=
                for mdk in "${!r.metadata.@}"; do
                    if [[ $mdk == *.key ]] then
                        typeset -n mdkr=$mdk
                        if [[ $mdkr == mac ]] then
                            typeset -n mdvr=${mdk%.key}.value
                            mac=$mdvr
                            break
                        fi
                    fi
                    [[ $mac != '' ]] && break
                done
                if [[ $mac != '' ]] then
                    id=${invinfo[${rid%-tap*}].id} inst=$mac
                    name=$mac
                    invinfo[$rid]=( id=$id inst=$inst label=$name )
                    invinfochanged=y
                fi
            else
                touch -d "14 day ago" inv.out
            fi
        fi

        t=$(TZ=UTC printf '%(%#)T' "${r.timestamp}")
        label=''
        for mi in ${r.meter//./' '}; do
            u=${mi:0:1}
            l=${shorts[$u${mi:1}]:-$u${mi:1}}
            label+=" $l"
        done
        label="${label#' '} ($name)"
        if [[
            ${drops[${r.meter}]} != '' ||
            (${r.meter} == *:* && ${drops[${r.meter%%:*}:]} != '')
        ]] then
            continue
        fi
        key=${r.meter//[.:]/_}
        [[ $inst != '' ]] && key+=".$inst"
        [[ $key != *.* ]] && key+="._total"
        key+="._id_$id"
        val=${r.volume}
        unit=${shorts[${r.unit}]:-${r.unit}}
        case ${r.type} in
        cumulative) type=counter ;;
        *)          type=number  ;;
        esac
        if (( emitted[$key].last < t )) then
            if [[ $type == number ]] then
                printf "$f\n" "$key" "$id" $t $val "$unit" "$type" "$label"
            elif [[ $type == counter && ${emitted[$key].val} != '' ]] then
                (( dv = val - emitted[$key].val ))
                printf "$f\n" "$key" "$id" $t $dv "$unit" number "$label"
            else
                printf "$fnd\n" "$key"
            fi
            emitted[$key]=( last=$t val=$val id=$id )
        else
            printf "$fnd\n" "$key"
        fi
        emitbykey[$key]=1
        emitbyid[$id]=1
    done

    for key in "${!emitted[@]}"; do
        if [[ ${emitbykey[$key]} != 1 ]] then
            printf "$fnd\n" "$key"
        fi
        if [[ ${emitbyid[${emitted[$key].id}]} != 1 ]] then
            touch -d "14 day ago" inv.out
            emitted[$key].last=0
        fi
    done

    if [[ $invinfochanged == y ]] then
        typeset -p invinfo > invinfo.sh.tmp && mv invinfo.sh.tmp invinfo.sh
    fi

    (( t = VG_JOBTS - 25 * 60 * 60 ))
    for key in "${!emitted[@]}"; do
        (( emitted[$key].last < t )) && unset emitted[$key]
    done

    typeset -p emitted > emitted.sh.tmp && mv emitted.sh.tmp emitted.sh
}

if [[ $showps == y ]] then
    gettokenbyname "$user" "$pass" admin

    getprojects

    for pid in "${!projects[@]}"; do
        typeset -n pr=projects[$pid]
        print -n "id=$pid name=${pr.name} enabled=${pr.enabled}"
        print " descr=${pr.descr} domain=${pr.did}"
    done
fi
