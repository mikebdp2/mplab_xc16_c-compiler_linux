#!/bin/sh
printf "=== Removing an old archive if it exists\n"
                                                          rm -f "./xc16-v2.10-full-install-linux64-installer.run"
printf "=== Making a new archive from its parts\n"
                                                          touch "./xc16-v2.10-full-install-linux64-installer.run"
cat "./xc16-v2.10-full-install-linux64-installer.run_part01" >> "./xc16-v2.10-full-install-linux64-installer.run"
cat "./xc16-v2.10-full-install-linux64-installer.run_part02" >> "./xc16-v2.10-full-install-linux64-installer.run"
cat "./xc16-v2.10-full-install-linux64-installer.run_part03" >> "./xc16-v2.10-full-install-linux64-installer.run"
printf "=== Doing a sync command (just in case)\n"
sync
printf "=== Finding a sha256sum of this archive\n"
sha256sum_correct="d64d5e7391ec862d4dd169361b6680b725f8960435121709b45eb1a4afe15dc8  ./xc16-v2.10-full-install-linux64-installer.run"
sha256sum_my=$(sha256sum "./xc16-v2.10-full-install-linux64-installer.run")
printf "=== sha256sum should be\n$sha256sum_correct\n"
if [ "$sha256sum_my" = "$sha256sum_correct" ] ; then
    printf "^^^ this is correct, you can use a ./xc16-v2.10-full-install-linux64-installer.run now...\n"
                                     chmod +x "./xc16-v2.10-full-install-linux64-installer.run"
    exit 0
else
    printf "^^^ ! MISMATCH ! Check sha256sum manually: sha256sum ./xc16-v2.10-full-install-linux64-installer.run\n"
    exit 1
fi
###
