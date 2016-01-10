#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

PKG_NAME="Gabber"

(test -f $srcdir/configure.in \
## put other tests here
) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level $PKG_NAME directory"
    exit 1
}

acldir=`aclocal --print-ac-dir`
if [ -d "$acldir/gnome-macros" -a -r "$acldir/gnome-macros/autogen.sh" ]; then
    acldir="$acldir/gnome-macros"
elif [ -d "$acldir/gnome" -a -r "$acldir/gnome/autogen.sh" ]; then
    acldir="$acldir/gnome"
elif [ -d "$srcdir/macros" -a -r "$srcdir/macros/autogen.sh" ]; then
    acldir="$srcdir/macros"
    echo "./macros directory is DEPRECATED. See README.cvs." >&2
else
    echo "GNOME aclocal macros not found." >&2
    echo "RPM users should install the gnome-common package." >&2
    echo "Debian users should install the libgnome-dev package." >&2
    echo "Further information can be found at http://gabber.sf.net/cvsinfo.php" >&2
    exit 1
fi
ACLOCAL_FLAGS="$ACLOCAL_FLAGS -I $acldir" . "$acldir/autogen.sh"
