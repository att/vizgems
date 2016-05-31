CC.PROBE = /dev/null
CC.PROBEPP = /dev/null

files = ../../input

levelnfiles = $(files:L=*-LEVELS.txt:/.txt$/-NODES.dds/)
levelmfiles = $(files:L=*-LEVELS.txt:/.txt$/-MAPS.dds/)

invnfiles = $(files:L=*-inv.txt:/.txt$/-nodes.dds/)
invefiles = $(files:L=*-inv.txt:/.txt$/-edges.dds/)
invmfiles = $(files:L=*-inv.txt:/.txt$/-maps.dds/)

:ALL: \
$$(levelnfiles) $$(levelmfiles) \
LEVELS-nodes.dds LEVELS-maps.dds \
$$(invnfiles) inv-nodes.dds inv-nodes-uniq.dds \
$$(invefiles) inv-edges.dds inv-edges-uniq.dds \
$$(invmfiles) inv-maps.dds inv-maps-uniq.dds


%-LEVELS-NODES.dds %-LEVELS-MAPS.dds :JOINT: %-LEVELS.txt
	vg_level one $(*) $(<)

if .MAKEVERSION. < 20080101
LEVELS-nodes.dds LEVELS-maps.dds :JOINT: $$$(levelnfiles) $$$(levelmfiles)
	vg_level all LEVELS-NODES LEVELS-MAPS $(<)
else
LEVELS-nodes.dds LEVELS-maps.dds :JOINT: $$(levelnfiles) $$(levelmfiles)
	vg_level all LEVELS-NODES LEVELS-MAPS $(<)
end


%-inv-nodes.dds %-inv-edges.dds %-inv-maps.dds :JOINT: %-inv.txt
	vg_inv one $(*) $(<)

if .MAKEVERSION. < 20080101
inv-nodes.dds inv-nodes-uniq.dds inv-edges.dds inv-edges-uniq.dds \
inv-maps.dds inv-maps-uniq.dds inv-nd2cc.dds inv-cc2nd.dds :JOINT: \
$$$(invnfiles) $$$(invefiles) $$$(invmfiles) LEVELS-nodes.dds LEVELS-maps.dds
	vg_inv all $(*:N=LEVELS-*) inv-nodes inv-edges inv-maps $(<)
else
inv-nodes.dds inv-nodes-uniq.dds inv-edges.dds inv-edges-uniq.dds \
inv-maps.dds inv-maps-uniq.dds inv-cc-nd2cc.dds inv-cc-cc2nd.dds :JOINT: \
$$(invnfiles) $$(invefiles) $$(invmfiles) LEVELS-nodes.dds LEVELS-maps.dds
	vg_inv all $(*:N=LEVELS-*) inv-nodes inv-edges inv-maps $(<)
end
