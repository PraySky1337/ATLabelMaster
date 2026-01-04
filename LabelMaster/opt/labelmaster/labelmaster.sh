#!/bin/bash 
scriptpath=`realpath "$0"`
appname=`basename "${scriptpath}" | sed s,\.sh$,,`
dirname=`dirname ${scriptpath}`
tmp="${dirname#?}"
if [ "${dirname%$tmp}" != "/" ]; then
	dirname=$PWD/$dirname
fi
LD_LIBRARY_PATH="${dirname}/lib"
export LD_LIBRARY_PATH
export QT_QPA_PLATFORM=xcb
$dirname/$appname "$@"


