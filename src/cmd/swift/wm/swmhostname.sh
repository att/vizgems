
if [[ $SWMADDRMAPFUNC != '' ]] then
    $SWMADDRMAPFUNC
    exit
fi

if [[ $SERVER_NAME != "" ]] then
    print $SERVER_NAME
    exit
fi

if [[ $SERVER_ADDR != "" ]] then
    print $SERVER_ADDR
    exit
fi

name=$(hostname)
if [[ $name == *.* ]] then
    print $name
    exit
fi

ip=$(nslookup -retry=4 -timeout=15 $name | egrep 'Name:|Address:' | tail -2)
if [[ $ip != Name* ]] then
    exit 1
fi

ip=${ip##*\ }
print $ip
