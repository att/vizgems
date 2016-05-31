
usetimerange=0
time=dummy_time
key=

while [[ $1 == -* ]] do
    case $1 in
    -key)
        key=$2
        shift
        ;;
    -time)
        s=$2
        time=${s%%:*}; rest=${s##$time?(:)}
        ft=${rest%%:*}; rest=${rest##$ft?(:)}
        lt=${rest%%:*}
        usetimerange=1
        export FT=$ft LT=$lt
        shift
        ;;
    esac
    shift
done

if [[ $key == '' ]] then
    print -u2 SWIFT ERROR: no key field specified
    exit 1
fi
if [[ $usetimerange == 1 ]] then
    if [[ $time == '' ]] then
        print -u2 SWIFT ERROR: no time field specified
        exit 1
    fi
    if [[ $FT == '' || $LT == '' ]] then
        print -u2 SWIFT ERROR: no time range specified
        exit 1
    fi
fi

while read line; do
    print -r "$line"
done > generickey.$$.txt
export GENERICKEYFILE=generickey.$$.txt
export GENERICKEYSIZE=$(wc -l < generickey.$$.txt)

ddsfilter -fe $'
    VARS{
    #include <regex.h>
    #include <vmalloc.h>

    typedef struct generic_t {
        char *s_key;
    } generic_t;

    static generic_t *generics;
    static int genericn, generici;

    #define CMP(k) (strcmp (generics[generici].s_key, k))

    static int ft, lt, dummy_time;

    #define START 1
    #define BSEARCH 2
    #define BACKUP 3
    #define FOUND 4
    #define NEXT 5
    #define FINISH 6
    static int minindex, maxindex, anchorindex;
    static int state = START, c;
    }
    BEGIN{
        Sfio_t *fp;
        char *line;
        int genericm;

        genericm = 0, genericn = atoi (getenv ("GENERICKEYSIZE"));
        if (genericn > 0) {
            if (!(generics = vmalloc (Vmheap, sizeof (generic_t) * genericn)))
                SUerror ("vg_generic_bsearch", "cannot allocate generics");
            memset (generics, 0, sizeof (generic_t) * genericn);
            if (!(fp = sfopen (NULL, getenv ("GENERICKEYFILE"), "r")))
                SUerror ("vg_generic_bsearch", "cannot open generic key file");
            while ((line = sfgetr (fp, \'\\n\', 1))) {
                if (!(generics[genericm].s_key = vmstrdup (Vmheap, line))) {
                    SUwarning (
                        0, "vg_generic_bsearch", "cannot allocate key: %s", line
                    );
                    break;
                }
                genericm++;
            }
            sfclose (fp);
        }
        if (genericm != genericn)
            genericn = -1;

        state = START;
        generici = 0;

        if ('$usetimerange$') {
            ft = atoi (getenv ("GENERICFT"));
            lt = atoi (getenv ("GENERICLT"));
        }
    }
    {
        DROP;
    again:
        if (state == START) {
            if (NREC == -1)
                SUerror ("vg_generic_bsearch", "cannot seek");
            minindex = 0, maxindex = NREC - 1;
            INDEX = (minindex + maxindex) / 2;
            state = BSEARCH;
        }
        if (state == BSEARCH) {
            c = CMP ('$key$');
            if (c == 0) {
                if (INDEX != minindex) {
                    anchorindex = INDEX;
                    INDEX = (INDEX + minindex) / 2;
                    state = BACKUP;
                    goto next;
                } else
                    state = FOUND;
            } else if (maxindex == minindex) {
                state = FINISH;
            } else {
                if (c > 0)
                    if (INDEX == maxindex)
                        state = FINISH;
                    else
                        minindex = INDEX + 1;
                else
                    if (INDEX == minindex)
                        state = FINISH;
                    else
                        maxindex = INDEX - 1;
                if (state == BSEARCH) {
                    INDEX = (maxindex + minindex) / 2;
                    goto next;
                }
            }
        }
        if (state == BACKUP) {
            c = CMP ('$key$');
            if (c == 0) {
                if (INDEX != minindex) {
                    anchorindex = INDEX;
                    INDEX = (INDEX + minindex) / 2;
                    goto next;
                } else
                    state = FOUND;
            } else {
                minindex = INDEX + 1;
                INDEX = (anchorindex + minindex) / 2;
                goto next;
            }
        }
        if (state == FOUND) {
            c = CMP ('$key$');
            if (c == 0) {
                if ('$usetimerange$') {
                    if ('$time$' >= ft && '$time$' <= lt)
                        KEEP;
                } else
                    KEEP;
                minindex = INDEX + 1;
                INDEX = minindex;
                goto next;
            } else {
                state = FINISH;
            }
        }
        if (state == FINISH) {
            if (generici == genericn - 1)
                INDEX = NREC;
            else {
                state = START;
                generici++;
                goto again;
            }
        }
    next: ;
    }
' $1
