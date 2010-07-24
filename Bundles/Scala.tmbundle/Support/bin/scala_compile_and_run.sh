#!/usr/bin/env bash

if [ "$1" = "--version" ]
then
    "$TM_SCALA" -version 2>&1 | head -1
    exit 0
fi

SOURCE="$1"
shift
TMP=/tmp/tm_scalamate 
mkdir -p "$TMP"
cd "$TMP"
"$TM_SCALA" -unchecked -deprecation -d "$TMP" -encoding UTF8 "$SOURCE"
# if (($? >= 1)); then exit; fi
#"$TM_SCALA" -Dfile.encoding=utf-8 $(basename -s .scala "$SOURCE") $@
echo -e "\nProgram exited with status $?.";