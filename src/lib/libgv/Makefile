:PACKAGE: graphviz ast
.SOURCE.h: ../libgraph
.SOURCE.a: /usr/X11/lib64

CCFLAGS += $$(CC.DLL)

gv 1.0 :LIBRARY: gvc.c gvconfig.c gvcontext.c gvdevice.c gvevent.c gvjobs.c \
	gvlayout.c gvloadimage.c gvplugin.c gvrender.c gvtextlayout.c \
	gvusershape.c \
	args.c arrows.c colxlate.c emit.c fontmetrics.c geom.c \
	globals.c htmllex.c htmlparse.c htmltable.c input.c labels.c memory.c \
	ns.c output.c pointset.c postproc.c psusershape.c routespl.c shapes.c \
	splines.c strcasecmp.c strncasecmp.c timing.c utils.c \
	cvt.c inpoly.c route.c shortest.c shortestpth.c \
	solvers.c triang.c util.c visibility.c \
	ccomps.c pack.c \
	BinaryHeap.c IntStack.c SparseMatrix.c general.c \
	xdot.c \
	fPQ.c intset.c maze.c ortho.c partition.c rawgraph.c \
	sgraph.c trapezoid.c \
	acyclic.c aspect.c class1.c class2.c cluster.c \
	compound.c conc.c decomp.c dotinit.c dotsplines.c fastgr.c flat.c \
	mincross.c position.c rank.c sameport.c \
	clusteredges.c comp.c dbg.c fdpinit.c grid.c layout.c \
	tlayout.c xlayout.c \
	LinkedList.c Multilevel.c PriorityQueue.c QuadTree.c \
	post_process.c sfdpinit.c sparse_solve.c spring_electrical.c \
	stress_model.c uniform_stress.c \
	circle.c twopiinit.c \
	adjust.c bfs.c call_tri.c circuit.c closest.c \
	compute_hierarchy.c conjgrad.c constrained_majorization.c \
	constrained_majorization_ipsep.c constraint.c delaunay.c dijkstra.c \
	edges.c embed_graph.c geometry.c heap.c hedges.c info.c kkutils.c \
	legal.c lu.c matinv.c matrix_ops.c neatomemory.c mosek_quad_solve.c \
	multispline.c neatoinit.c neatosplines.c opt_arrangement.c overlap.c \
	pca.c poly.c printvis.c quad_prog_solve.c quad_prog_vpsc.c site.c \
	smart_ini_x.c solve.c stress.c stuff.c voronoi.c \
	block.c blockpath.c blocktree.c circpos.c circular.c \
	circularinit.c deglist.c edgelist.c nodelist.c nodeset.c \
	osageinit.c \
	misc.c red_black_tree.c stack.c \
	patchwork.c patchworkinit.c tree_map.c \
	gvloadimage_core.c gvplugin_core.c gvrender_core_dot.c \
	gvrender_core_pic.c gvrender_core_pov.c \
	gvrender_core_fig.c gvrender_core_map.c gvrender_core_ps.c \
	gvrender_core_svg.c gvrender_core_tk.c gvrender_core_vml.c \
	gvdevice_gd.c gvloadimage_gd.c gvplugin_gd.c \
	gvrender_gd.c gvrender_gd_vrml.c gvtextlayout_gd.c \
	gvlayout_dot_layout.c gvplugin_dot_layout.c \
	gvlayout_neato_layout.c gvplugin_neato_layout.c \
	index.c node.c nrtmain.c rectangle.c split.q.c xlabels.c \
	taper.c \
	gv.ign -lgraph -lgd -lfontconfig -lfreetype -lz -lpng -lexpat -lgts

$(INCLUDEDIR) :INSTALLDIR: \
gvc.h gvio.h gvplugin.h gvcext.h gvcjob.h gvcommon.h color.h types.h geom.h \
textpara.h arith.h usershape.h globals.h pathgeom.h
