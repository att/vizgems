#!/bin/bash

d=`date`
/usr/sbin/smartctl --scan-open | while read i; do
    i=${i%%'#'*}
    echo DATE: $d
    echo VIZGEMS1: $i
    /usr/sbin/smartctl -i -A $i
    echo VIZGEMS2: $i
done > /var/log/vgsmartctl.log.tmp \
&& mv /var/log/vgsmartctl.log.tmp /var/log/vgsmartctl.log
