
export SWMROOT=${DOCUMENT_ROOT%/htdocs}

cd $SWMDIR || exit 1

print "<table border>"
for descr in .descr.*; do
    file=${descr#.descr.}
    if [[ -f $file ]] then
        print "<tr>"
        print "<td><a href=$SWMDIR_REL/$file>$file</a></td>"
        print "<td>$(/bin/ls -l $file | awk '{ print $5 }') bytes</td>"
        print "<td>$(< $descr)</td>"
        print "</tr>"
    fi
done
print "</table>"
