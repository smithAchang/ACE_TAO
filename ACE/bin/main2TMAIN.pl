eval '(exit $?0)' && eval 'exec perl -pi -S $0 ${1+"$@"}'
    & eval 'exec perl -pi -S $0 $argv:q'
    if 0;

#
# You may want to run the "find" command with this script, which maybe
# something like this:
#
# find . -type f \( -name "*.C" -o -name "*.cc" -o -name "*.c" -o -name "*.cpp" \) -print | xargs $ACE_ROOT/bin/main2TMAIN.pl

# The first three lines above let this script run without specifying the
# full path to perl, as long as it is in the user's PATH.
# Taken from perlrun man page.

s/main( *\(int[ A-Za-z]*, *ACE_TCHAR)/ACE_TMAIN$1/g;
