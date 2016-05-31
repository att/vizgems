#!/bin/ksh

export SWMROOT=${DOCUMENT_ROOT%/htdocs}
. $SWMROOT/bin/swmenv
[[ $SECONDS != *.* ]] && exec $SHELL $0 "$@"
. $SWMROOT/bin/swmgetinfo

print "Content-type: text/html

<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 TRANSITIONAL//EN\">
<html>
<head>
<title>SWIFT Change Password</title>
</head>
<body>
<h3 align=center>
<font color='#FF0000'>Change Password</font>
</h3>
<form action=$SWIFTWEBPREFIX/cgi-bin-members/swmchangepasswd.cgi method=POST>
<center><i>
To use this form, you must know your SWIFT userid and password.<br>
Please choose your password wisely!
</i></center>
<p>
<center>
<table>
<tr>
  <th align=left> User name:</th>
  <td><input type=text name=user value='$SWMID'></td>
</tr>
<tr>
  <th align=left>New password:</th>
  <td>
    <input type=password name=newpasswd1> -- <i>at least 6 characters.</i>
  </td>
</tr>
<tr>
  <th align=left>Re-type new password:</th>
  <td><input type=password name=newpasswd2></td>
</tr>

<tr><td colspan=2></td></tr>

<tr><td colspan=2>
  <input type=submit value='Change password'>
  <input type=reset value='Reset these fields'>
</td></tr>
</table>
</center>
</form>
</body>
</html>
"
