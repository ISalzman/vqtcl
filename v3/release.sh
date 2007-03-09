# build and test from scratch, then build a clean tar distro in ~/Desktop

set -e

opwd=$PWD
name=vlerq3
now=`date '+%Y-%m-%d'`
tar=$name-$now.tgz

echo ____________________________________________________________________
echo "> Most recent entry in ChangeLog:"
darcs changes >ChangeLog
head -2 ChangeLog

if [ configure.in -nt configure ]
  then echo "> Run autoconf:"; autoconf
fi
  
echo "> Run configure script:"
rm -rf /tmp/make-$name
mkdir /tmp/make-$name
cd /tmp/make-$name
sh $opwd/configure --prefix=$PWD --exec-prefix=$PWD >config.out

echo "> Make and run tests:"
make -s test

echo "> Test install:"
make install >/dev/null
tclsh <<'EOF'
  exit [catch {
    set auto_path .
    package require mklite
    package require ratcl
    set il [join [lsort [info loaded]] " "]
    set pn [lsort [package names]]
    puts "loaded: $il\npackages: $pn"
    if {$il != "./libvlerq3[info sharedlib] Vlerq" ||
        $pn != "Tcl mklite ratcl vfs::m2m vfs::mkcl vlerq"} { exit 1 }
  }]
EOF

echo "> Create tarfile:"
rm -rf /tmp/dist/$name /tmp/dist/$name.tar.gz
mkdir -p /tmp/dist
chgrp staff /tmp/dist
make dist >/dev/null

echo ====================================================================
tar tfz /tmp/dist/$name.tar.gz | column | expand | sed 's/^/  /'
echo ====================================================================

cd ~/Desktop
rm -rf /tmp/make-$name /tmp/dist/$name
mv /tmp/dist/$name.tar.gz $tar

ls -l $tar
echo "> Done."
