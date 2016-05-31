
icase=n
usetimerange=0
time=dummy_time

while [[ $1 == -* ]] do
    case $1 in
    -icase)
        icase=y
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

if [[ $icase == y ]] then
    export ICASE=REG_ICASE
else
    export ICASE=0
fi

typeset -A key2id
n=0
while read line; do
    key=${line%%'|'*}
    val=${line#*'|'}
    if [[ ${key2id[$key]} == '' ]] then
        cases+="case $n: c = CMP (generics, generici, $key); break; "
        key2id[$key]=$n
        (( n++ ))
    fi
    print "${key2id[$key]}|$key|$val"
done > genericparam.$$.txt
export GENERICPARAMFILE=genericparam.$$.txt
export GENERICPARAMSIZE=$(wc -l < genericparam.$$.txt)

ddsfilter -fe $'
    VARS{
    #include <regex.h>
    #include <vmalloc.h>

    typedef struct generic_t {
        int s_type;
        char *s_key;
        char *s_valre;
        regex_t s_code;
    } generic_t;

    static generic_t *generics;
    static int genericn;

    #define CMPFLAGS ( \\
        REG_NULL | REG_EXTENDED | REG_LEFT | REG_RIGHT | '$ICASE$' \\
    )
    #define CMP(p, i, s) (regexec (&p[i].s_code, s, 0, NULL, CMPFLAGS) == 0)

    static int ft, lt, dummy_time;
    }
    BEGIN{
        Sfio_t *fp;
        char *line, *s1, *s2;
        int genericm, ism, filem;

        genericm = 0, genericn = atoi (getenv ("GENERICPARAMSIZE"));
        if (genericn > 0) {
            if (!(generics = vmalloc (Vmheap, sizeof (generic_t) * genericn)))
                SUerror ("vg_generic_lsearch", "cannot allocate generics");
            memset (generics, 0, sizeof (generic_t) * genericn);
            if (!(fp = sfopen (NULL, getenv ("GENERICPARAMFILE"), "r")))
                SUerror (
                    "vg_generic_lsearch", "cannot open generic filter file"
                );
            while ((line = sfgetr (fp, \'\\n\', 1))) {
                if (!(s1 = strchr (line, \'|\'))) {
                    SUwarning (0, "vg_generic_lsearch", "bad line: %s", line);
                    break;
                }
                *s1++ = 0;
                if (!(s2 = strchr (s1, \'|\'))) {
                    SUwarning (0, "vg_generic_lsearch", "bad line: %s", s1);
                    break;
                }
                *s2++ = 0;
                generics[genericm].s_type = atoi (line);
                if (!(generics[genericm].s_key = vmstrdup (Vmheap, s1))) {
                    SUwarning (
                        0, "vg_generic_lsearch", "cannot allocate key: %s", s2
                    );
                    break;
                }
                if (!(generics[genericm].s_valre = vmstrdup (Vmheap, s2))) {
                    SUwarning (
                        0, "vg_generic_lsearch", "cannot allocate text: %s", s2
                    );
                    break;
                }
                if (regcomp (
                    &generics[genericm].s_code, generics[genericm].s_valre,
                    CMPFLAGS
                ) != 0) {
                    SUwarning (
                        0, "vg_generic_lsearch",
                        "cannot compile regex: %s", generics[genericm].s_valre
                    );
                    break;
                }
                genericm++;
            }
            sfclose (fp);
        }
        if (genericm != genericn)
            genericn = -1;

        if ('$usetimerange$') {
            ft = atoi (getenv ("GENERICFT"));
            lt = atoi (getenv ("GENERICLT"));
        }
    }
    {
        int generici, genericm, c;

        DROP;

        for (generici = genericm = 0; generici < genericn; generici++) {
            c = 0;
            switch (generics[generici].s_type) {
            '"$cases"$'
            }
            if (!c)
                break;
            genericm++;
        }
        if (genericm == genericn) {
            if ('$usetimerange$') {
                if ('$time$' >= ft && '$time$' <= lt)
                    KEEP;
            } else
                KEEP;
        }
    }
' $1
