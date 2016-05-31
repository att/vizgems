
ip=$(nslookup -retry=2 -timeout=15 $1 | egrep 'Name:' | tail -1)
if [[ $ip != Name* ]] then
    exit 1
fi

ip=${ip##*\ }

print $ip
