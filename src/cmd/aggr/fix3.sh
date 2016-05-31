
for o in "$@"; do
    print $o
    aggrprint -V -V -nodata $o | while read k v; do
        case $k in
        class:) class=$v ;;
        valtype:) valtype=$v ;;
        esac
    done
    aggrcat -o $o.new -cl $class -vt $valtype $o \
    && mv $o.new $o
done
