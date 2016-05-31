if [[ ! -f compress.done ]] then
    for i in "$@"; do
        [[ ! -f $i ]] && continue
        [[ -L $i ]] && continue
        ddsinfo -q $i | egrep compression | read a b
        [[ $b == '' || $b != none ]] && continue
        print compressing $i
        if ddscat -ozm rtb $i > $i.new; then
            onrec=$(ddsinfo $i | egrep nrecs:)
            onrec=${onrec##*' '}
            nnrec=$(ddsinfo $i.new | egrep nrecs:)
            nnrec=${nnrec##*' '}
            if [[ $onrec == $nnrec ]] then
                touch -r $i $i.new
                mv $i.new $i
            else
                print ERROR compression failed for $i
                rm -f $i.new
            fi
        else
            print ERROR compression failed for $i
            rm -f $i.new
        fi
    done
fi
touch compress.done
