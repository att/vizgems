
inst=$1
dhost=$2
dport=$3
mhost=$4
mport=$5
shift 5

if [[
    $inst == "" || $dhost == "" || $dport == "" || $mhost == "" || $mport == ""
]] then
    print -u2 ERROR missing arguments
    exit 1
fi

typeset -u INST=$inst

codebase=/swift/
codejar=jswift.jar

auth=$(suiencode)
auth=${auth//$'\n'/';'}

tools="$*"

typeset -A params

params[swiftinstance]=$inst
params[swiftprefprefix]=${SWMPREFPREFIX#$SWIFTDATADIR}

params[swiftbase]=$SWIFTWEBSERVER
params[swiftauth]=$auth
params[swifttools]=$tools
params[swiftdebug]=0
params[swiftgroups]=$SWMGROUPS
params[swiftuser]=$SWMID

params[swiftmname]=$mhost
params[swiftmport]=$mport

params[swiftdname]=$dhost
params[swiftdport]=$dport

sed -e 's/^<param name=//' -e 's/ value=/ /' -e 's/>$//' < $SWMPREFPREFIX.java \
| while read param value; do
    eval params[swift$param]=$value
done

javasite=http://java.sun.com/products/plugin/1.2
w=500
h=40

print "<center><object"
print "  classid='clsid:8AD9C840-044E-11D1-B3E9-00805F499D93'"
print "  width=$w height=$h"
print "  name=${INST}Run"
print "  codebase='$javasite/jinstall-12-win32.cab#Version=1,2,0,0'"
print ">"
print "  <param name=type value='application/x-java-applet;version=1.2'>"
print "  <param name=code value=${INST}Run>"
print "  <param name=codebase value='$codebase'>"
print "  <param name=archive value='$codejar'>"
print "  <param name=mayscript value=true>"

for param in "${!params[@]}"; do
    print "  <param name=$param value='${params[$param]}'>"
done

print "  <comment>"
print "    <embed"
print "      type='application/x-java-applet;version=1.2' name=${INST}Run"
print "      java_code=${INST}Run java_codebase=$codebase java_archive=$codejar"
print "      width=$w height=$h mayscript=true"

for param in "${!params[@]}"; do
    print "      $param='${params[$param]}'"
done

print "      pluginspage='$javasite/plugin-install.html'"
print "    >"
print "    <noembed>"
print "      Cannot run $name without JDK 1.2 support"
print "    </noembed>"
print "    </embed>"
print "  </comment>"
print "</object></center>"
