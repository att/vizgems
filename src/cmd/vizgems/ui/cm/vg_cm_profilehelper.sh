#!/bin/ksh

dir=$SWIFTDATADIR/data/main/latest/processed/total

export STATMAPFILE=$dir/stat.list
export INVNODEATTRFILE=$dir/inv-nodes.dds

vg_cm_profilefilter
