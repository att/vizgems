
export SWMROOT=${DOCUMENT_ROOT%/htdocs}

cd $SWMDIR || exit 1

integer filen=0
integer filesize=0

for descr in .descr.*; do
    file=${descr#.descr.}
    if [[ -f $file ]] then
        (( filen++ ))
        (( filesize += $(/bin/ls -l $file | awk '{ print $5 }') ))
    fi
done

print "
<table border>
<tr>
  <td>Files</td>
  <td>$filen</td>
</tr>
<tr>
  <td>Bytes</td>
  <td>$filesize</td>
</tr>
</table>
"
