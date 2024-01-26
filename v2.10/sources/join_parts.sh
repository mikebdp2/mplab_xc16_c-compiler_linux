#!/bin/sh
printf "=== Removing an old archive if it exists\n"
                        rm -f "./v2.10.src.zip"
printf "=== Making a new archive from its parts\n"
                        touch "./v2.10.src.zip"
cat "v2.10.src.zip_part01" >> "./v2.10.src.zip"
cat "v2.10.src.zip_part02" >> "./v2.10.src.zip"
cat "v2.10.src.zip_part03" >> "./v2.10.src.zip"
cat "v2.10.src.zip_part04" >> "./v2.10.src.zip"
cat "v2.10.src.zip_part05" >> "./v2.10.src.zip"
printf "=== Doing a sync command (just in case)\n"
sync
printf "=== Finding a sha256sum of this archive\n"
sha256sum_correct="faf4d0c0f5d1f9c9f24f0d7203c054300b6fbb142bb286f2af3e65970a20a9c1  ./v2.10.src.zip"
sha256sum_my=$(sha256sum "./v2.10.src.zip")
printf "=== sha256sum should be\n$sha256sum_correct\n"
if [ "$sha256sum_my" = "$sha256sum_correct" ] ; then
    printf "^^^ this is correct, you can use a ./v2.10.src.zip now...\n"
    exit 0
else
    printf "^^^ ! MISMATCH ! Check sha256sum manually: sha256sum ./v2.10.src.zip\n"
    exit 1
fi
###
