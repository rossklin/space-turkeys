if HAVE_DOXYGEN
directory = $(top_srcdir)/docs/html

dist_man_HTML = $(directory)
$(dist_man_HTML): force_check

all-local: force_check

force_check:
	$(DOXYGEN) Doxyfile
	true

clean-local:
	rm -rf $(directory)
endif
