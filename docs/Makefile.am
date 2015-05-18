if HAVE_DOXYGEN
directory = $(top_srcdir)/docs/html

dist_man_HTML = $(directory)/index.html
$(directory)/index.html: doxyfile.stamp

doxyfile.stamp:
	$(DOXYGEN) Doxyfile
	echo Timestamp > doxyfile.stamp

CLEANFILES = doxyfile.stamp

all-local: doxyfile.stamp
clean-local:
	rm -rf $(directory)
endif
