SUBDIRS = src doc

vgenv = G_SLICE=always-malloc G_DEBUG=gc-friendly

vgargs = --tool=memcheck --leak-check=full --leak-resolution=high \
	 --num-callers=20 --suppressions=glib.suppress \
	 --log-file=tmp.log

vgcmd = ./src/unscroll data/example.pdf tmp.pdf

leak-check: all
	$(vgenv) valgrind $(vgargs) $(vgcmd)
