#!/bin/ksh

export SWMROOT=${DOCUMENT_ROOT%/htdocs}
. $SWMROOT/bin/swmenv
[[ $SECONDS != *.* ]] && exec $SHELL $0 "$@"
. $SWMROOT/bin/swmgetinfo

if [[ $SWMMODE == secure ]] then
    exit 0
fi

cd $SWMDIR || exit 1

print "Content-type: text/html\n
<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 TRANSITIONAL//EN\">
<html>
<head>
</head>
<body>
<h2>Delete a File</h2>
<form
  action=$SWIFTWEBPREFIX/cgi-bin-members/swmdeletefile2.cgi
  enctype='multipart/form-data'
  method=post
>
<table><tr>
  <td><input type='submit'></td>
  <td><input type='reset'></td>
  <td><a href='#help'>Help</a></td>
</tr></table>
<select name=files multiple>
"

for descr in .descr.*; do
    file=${descr#.descr.}
    if [[ -f $file ]] then
        print "  <option> $file"
    fi
done

print "</select>"

print "
</form>
<a name=help>
<h2>Help</h2>
<h3>File to Upload</h3>
Use the browse button to find the file on your local machine
that you want to upload. The file must be a plain ascii file.
<h3>File Name on Server</h3>
Enter the name the file should have on your personal area on
the SWIFT server.
<h3>Description of Contents</h3>
Describe the contents of the file. This description will be
printed when you list your files on the SWIFT server.
</body>
</html>
"
