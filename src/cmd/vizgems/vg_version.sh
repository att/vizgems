
if [[ $SWIFTWARNLEVEL != '' ]] then
    set -x
fi

. vg_hdr
print $VG_S_VERSION

if [[ $1 == -v ]] then
    p=$(whence vg_version)
    if [[ $p == */current/* ]] then
        ls -lP ${p%/current/*}/current | read pp
        pp=${pp%.sw}
        pp=${pp##*.v}
        [[ $pp != '' ]] && print $pp
    fi
fi
