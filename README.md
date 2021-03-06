This is Vlerq, a Tcl extension for data manipulation and storage.

This package is released as open source under the same license as Tcl,
see ./license.terms and http://www.tcl.tk/software/tcltk/license.html

UNIX BUILD
----------

Building under most UNIX systems is easy, just run the configure script
and then run make. For more information about the build process, see the
tcl/unix/README file in the Tcl src dist. The following will configure,
test, build, and install the vlerq extension in the /opt/tcl directory.

	$ ./configure --prefix=/opt/tcl --exec-prefix=/opt/tcl
	$ make
	$ make test
	$ make install

WINDOWS BUILD
-------------

The recommended method to build extensions under windows is to use the Msys
+ Mingw build process. This provides a Unix-style build while generating
native Windows binaries. Using the Msys + Mingw build tools means that you
can use the same configure script as per the Unix build to create a
Makefile. See the tcl/win/README file for the URL of the Msys + Mingw
download.

If you have VC++ then you may wish to use the files in the win subdirectory
and build the extension using just VC++. These files have been designed to
be as generic as possible but will require some additional maintenance by
the project developer to synchronise with the TEA configure.in and
Makefile.in files. Instructions for using the VC++ makefile are written in
the first part of the Makefile.vc file.

INSTALLATION
------------

The installation of a TEA package is structured like so:

	       $exec_prefix
		/       \
	      lib       bin
	       |         |
	 PACKAGEx.y   (dependent .dll files on Windows)
	       |
	pkgIndex.tcl (.so|.dll|.dylib and .tcl files)

The main .so|.dll library file gets installed in the versioned PACKAGE
directory, which is OK on all platforms because it will be directly
referenced with by 'load' in the pkgIndex.tcl file.  Dependent DLL files on
Windows must go in the bin directory (or other directory on the user's
PATH) in order for them to be found.
