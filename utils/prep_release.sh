#!/bin/bash
#
# (c) 2008 Tod D. Romo
#     Grossfield Lab
#     URMC
#
#  NOTE:  This script is used internally to create a release from the
#         SVN repository



export PREF=/var/tmp/loos-inst   # Where to put test install of LOOS
export RELDIR=/var/tmp           # Where to export LOOS release for tarballing
export PROCS=4                   # Number of procs to use in scons build

export SVN=svn                   # Change this to echo for debugging (disables checkins)

######################################

function AbortOrContinue {
    local OK

    echo $1
    read -p 'Do you want to proceed?  [y/N] ' OK
    if [ "$OK" = "N" -o "$OK" = "n" -o \( -z "$OK" \) ] ; then
	echo "Release aborted!"
	exit -1
    fi
}


######################################

WHAT=`pwd | sed 's@^.*LOOS/@@'`
echo "Release base: $WHAT"
VERS=`echo $WHAT | sed 's/^.*release-//'`
echo "Release version: $VERS"
echo

### Small sanity check...
OK=`pwd | egrep 'tags/release-[0-9.]+(-(alpha|beta))?$'`
if [ ! $OK ] ; then
    echo "WARNING!!!"
    echo "You don't appear to be prepping a release in a release directory!"
    echo "Make sure you are in a properly tagged release from the SVN"
    echo "prior to running this script."
    
    AbortOrContinue
fi


echo "Checking version ID..."
DOXVERS=`grep '^PROJECT_NUMBER' Doxyfile | gawk '{print $3}' | sed 's/^v//'`
FAILED=""
if [ "$DOXVERS" != "$VERS" ] ; then
    echo "Doxygen version: $DOXVERS"
    echo "SVN version: $VERS"
    AbortOrContinue "Version mismatch in Doxyfile"
    FAILED="yes"
fi

SCONSVERS=`egrep '^loos_version' loos_build_config.py | gawk '{print $3}' | sed "s/'//g"`
if [ "$SCONSVERS" != "$VERS" ] ; then
    AbortOrContinue "Version mismatch in SConstruct"
    FAILED="yes"
fi

echo

if [ -z "$FAILED" ] ; then
    echo "Everything appears ok."
else
    echo "WARNING- version mismatches found, but you decided to continue anyway..."
    sleep 5
fi



echo "*** Building documentation"
echo "+ Cleaning..."
rm -rf Doc                # Manually remove since scons wont
scons -cs caboodle        # Clean everything (to be safe)
echo "+ Building..."
scons -sj$PROCS docs      # Rebuild docs explicitly



echo "*** Checking install target"
scons -sj$PROCS install PREFIX=$PREF
CWD=`pwd`
if check_loos_install.pl --nofull --exclude 'prep_release.sh' --exclude 'membrane_map.hpp' --exclude 'make_macos_tarball.sh' --prep `pwd` $PREF ; then
    echo "Install appears ok.  Generating full report..."
    ( echo "***INSTALL APPEARS OK***" ;\
      echo "Please check the diff list below for any errors..." ;\
      echo ;\
      check_loos_install.pl --full `pwd` $PREF ) | less

    AbortOrContinue

else
    echo "***SUSPECTED ERRORS IN INSTALLATION***"
    echo "Aborting"
    exit -1
fi

echo "*** Removing test install"
rm -r $PREF


echo "*** Checking SVN status"
OK=`svn -q status`
if [ -n "$OK" ] ; then
    echo "ERROR - you appear to have unchecked modifications to this release."
    echo "        Please fix this and run this script again..."
    echo
    echo "> svn -q status"
    echo "$OK"
    exit -1
fi



echo "*** Fixing ChangeLog"
cp ChangeLog ChangeLog-
sed -r 's/@[^>]+//' <ChangeLog- >ChangeLog
rm ChangeLog-


# Update SVN

RELEASE=`pwd | sed -r 's@^/.+/@@'`

read -p 'Shall I automatically update the SVN for you? (this will also remove this script) [Y/n] ' OK

if [ "$OK" = "Y" -o "$OK" = "y" -o \( -z "$OK" \) ] ; then
    echo "Updating SVN for $RELEASE..."
    
#    set -o xtrace
    echo "+ Adding documentation"
    $SVN add Docs docs.built
    $SVN ci -m "Added Documentation to $RELEASE" Docs
    $SVN ci -m "Added docs marker to $RELEASE" docs.built

    echo "+ Checking in ChangeLog"
    $SVN ci -m "Sanitized ChangeLog prior to $RELEASE" ChangeLog

    echo "+ Removing prep_release script"
    $SVN rm prep_release.sh
    $SVN ci -m "Removed prep_release script from $RELEASE" prep_release.sh

    echo "+ Removing make_macos_tarball script"
    $SVN rm make_macos_tarball.sh
    $SVN ci -m "Removed make_macos_tarball script from $RELEASE" make_macos_tarball.sh

    echo "*** Making Release Tarball"
    pushd $RELDIR

    svn export svn+ssh://membrane.urmc.rochester.edu/home/svn/subversion/software/LOOS/$WHAT loos-$VERS
    tar cvf - loos-$VERS | gzip -cv9 >~/loos-$VERS.tar.gz
    rm -r loos-$VERS
    popd

    cat <<EOF

==== UPDATE INSTRUCTIONS ====

The release is in your home directory.  Look in the SVN for the README file.


Login on sourceforge and go into the developer's panel and select the
file list.  Create a new dir for the release and upload the tarball to
it.  You'll also need to create a README file and upload this
too...this is what gets shown beneath the file as a "release notes."
Finally, go back to the tarball file and click on the "i" information
icon and click on "select all" for the default download.  Now, to
update the documentation, go to the Docs directory within the tagged
release in the SVN.  Execute the following rsync command:

  rsync -avk --delete --exclude='.svn' . username,loos@web.sourceforge.net:htdocs

with your Sourceforge username and password.
   
EOF

else
    echo "***WARNING***WARNING***WARNING***"
    echo
    echo "Manual release is expected now"
    echo
    echo "DON'T FORGET YOU MUST UPDATE THE SVN PRIOR TO RELEASING $RELEASE"
fi

echo "=== ALL DONE ==="
echo "BE SURE TO UPDATE /OPT/LOOS/LATEST ON MEMBRANE"
echo "BE SURE TO UPDATE /OPT/LOOS/LATEST ON MEMBRANE"
echo "BE SURE TO UPDATE /OPT/LOOS/LATEST ON MEMBRANE"



