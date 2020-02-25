
usage=$'
[-1p1?
@(#)$Id: vg_openstackinfo (AT&T) 2007-03-25 $
]
[+NAME?vg_openstackinfo - print information about an openstack installation]
[+DESCRIPTION?\bvg_openstackinfo\b queries the specified openstack site
and prints out any information specified by options.
]
[200:host?specifies the DNS name or IP address of the server to query.
Prefix with https:// to use HTTPS.
]:[host]
[300:user?speficies the user name to use.
]:[user]
[301:pass?speficies the user password to use.
]:[pass]
[400:proj?prints out information about projects.
]
[500:authversion?speficies the version of the authentication protocol to use.
]:[version]
[600:domain?speficies the domain to use.
]:[domain]
[999:v?increases the verbosity level. May be specified multiple times.]
'

function showusage {
    OPTIND=0
    getopts -a vg_openstackinfo "$usage" opt '-?'
}

host=
user= pass=
showps=
authversion=3
domain=default

while getopts -a vg_openstackinfo "$usage" opt; do
    case $opt in
    200) host=$OPTARG ;;
    300) user=$OPTARG ;;
    301) pass=$OPTARG ;;
    400) showps=y ;;
    500) authversion=$OPTARG ;;
    600) domain=$OPTARG ;;
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

if [[ ${authversion} == '' ]] then
    print -u2 "ERROR no authversion specified"
    exit 0
fi

if [[ ${domain} == '' ]] then
    print -u2 "ERROR no domain specified"
    exit 0
fi

if [[ $host == https:* ]] then
    authurl=$host:5000/v$authversion
    invurl=$host:5000/v3
    computeurl=$host:8774/v2
    volumeurl=$host:8776/v2
    statsurl=$host:8777/v2/samples
else
    authurl=http://$host:5000/v$authversion
    invurl=http://$host:5000/v3
    computeurl=http://$host:8774/v2
    volumeurl=http://$host:8776/v2
    statsurl=http://$host:8777/v2/samples
fi

uid=
aid=

typeset -A projects groups users
typeset -A seenimages seenservers seenvolumes seenusers seengroups
typeset -A seengs seenss
typeset -A invinfo emitted prevemitted id2is
typeset invinfochanged=n id2ii=0

# authentication

if [[ $authversion == 3 ]] then

function gettokenbyname { # $1 = user $2 = pass $3 = tenant name
    typeset user=$1 pass=$2 tenant=$3

    typeset tk

    eval $(
        curl -D hdr.txt -k -s -X POST \
            -H "Content-type: application/json" -H "Accept: application/json" \
        $authurl/auth/tokens?nocatalog -d "{
          \"auth\": {
            \"identity\": {
              \"methods\": [
                \"password\"
              ],
              \"password\": {
                \"user\": {
                  \"name\": \"$user\",
                  \"domain\": {
                    \"id\": \"default\"
                  },
                  \"password\": \"$pass\"
                }
              }
            },
            \"scope\": {
              \"domain\": { \"id\": \"$domain\" }
            }
          }
        }" | vgjson2ksh tk
    )

    uid=${tk.token.user.id}
    aid=$( egrep X-Subject-Token: hdr.txt )
    aid=${aid#*' '}
    aid=${aid%$'\r'}
    rm -f hdr.txt
}

function gettokenbyid { # $1 = user $2 = pass $3 = tenant id
    typeset user=$1 pass=$2 tenant=$3

    typeset tk

    eval $(
        curl -D hdr.txt -k -s -X POST \
            -H "Content-type: application/json" -H "Accept: application/json" \
        $authurl/auth/tokens?nocatalog -d "{
          \"auth\": {
            \"identity\": {
              \"methods\": [
                \"password\"
              ],
              \"password\": {
                \"user\": {
                  \"name\": \"$user\",
                  \"domain\": {
                    \"id\": \"default\"
                  },
                  \"password\": \"$pass\"
                }
              }
            },
            \"scope\": {
              \"project\": {
                \"domain\": { \"id\": \"$domain\" },
                \"id\": \"$tenant\"
              }
            }
          }
        }" | vgjson2ksh tk
    )

    uid=${tk.access.user.id}
    aid=$( egrep X-Subject-Token: hdr.txt )
    aid=${aid#*' '}
    aid=${aid%$'\r'}
    rm -f hdr.txt
}

else # version = 2.0

function gettokenbyname { # $1 = user $2 = pass $3 = tenant name
    typeset user=$1 pass=$2 tenant=$3

    typeset tk

    eval $(
        curl -k -s -X POST \
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
        curl -k -s -X POST \
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

fi

# projects

function getprojects { # no args
    typeset pr i id name enabled did descr

    eval $(
        curl -k -s \
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

if [[ $showps == y ]] then
    gettokenbyname "$user" "$pass" admin

    getprojects

    for pid in "${!projects[@]}"; do
        typeset -n pr=projects[$pid]
        print -n "id=$pid name=${pr.name} enabled=${pr.enabled}"
        print " descr=${pr.descr} domain=${pr.did}"
    done
fi
