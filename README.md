VizGEMS
===

This is the AT&amp;T VizGEMS system from AT&amp;T Research.
It can be used to monitor the operation of very large compute infrastructures,
including both physical and virtual assets.

It comes with a collection of tools to monitor the most popular types of
devices: Linux / Unix, Microsoft Windows, Cisco and other brands of switches,
routers, firewalls, etc. It can also interface with OpenStack to extract
both the inventory, alarms, and performance statistics for the virtual assets
managed by OpenStack.

All this data (as well as arbitrary data delivered by other systems),
is processed in various ways: there are 2 types of alarm correlation engines,
one that can consolidate raw alarms into a smaller number of 'events',
and another that can calculate the impact of alarms to production services.
Statistics are also analyzed to generate detailed profiles than can then
be used for proactive alarming and also capacity planning.

This software is used to build itself, using NMAKE.
After cloning this repo, cd to the top directory of it and run:

./bin/package make

Almost all the tools in this package (including the bin/package script are
self-documenting; run <tool> --man (or --html) for the man page for the tool.

VizGEMS is based on several other AT&amp;T Research technologies that have
already been open-sourced. Copies of them are included here but if you
want to get the most recent versions the right place is indicated here:

1. AST. This is a toolkit that includes many tools and libraries,
like KSH, NMAKE, SFIO, VMALLOC, VCODEX, etc. It also includes more efficient
replacements for a lot of the POSIX tools.
Latest version available on https://github.com/att/ast

2. Graphviz. This is a graph layout toolkit. It includes many different
layout styles.
Latest version available from http://graphviz.org

3. SWIFT. This is the toolkit that implements the data processing backend
for VizGEMS. Uses on-the-fly compilation to improve performance.

In addition, VizGEMS currently uses the libgd library to generate images.
These are used in the current HTML4 UI. The next version of VizGEMS will
be switching to a D3 UI, using tools like dc.js, cola.js, etc.
Latest version of libgd: https://github.com/libgd/libgd/releases
