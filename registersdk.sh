#! /bin/sh

# Registers MerVM, targets and emulator device of an installed Sailfish SDK

export SAILFISHOSSDKDIR=$HOME/SailfishOS

bin/sdktool addMerSdk --installdir $SAILFISHOSSDKDIR --vm-name MerSDK --autodetected true --shared-home $HOME --shared-targets $SAILFISHOSSDKDIR/mersdk/targets/ --shared-ssh $SAILFISHOSSDKDIR/mersdk/ssh/ --host localhost --username mersdk --private-key-file $SAILFISHOSSDKDIR/vmshare/ssh/private_keys/engine/mersdk --ssh-port 2222 --www-port 8080 --shared-config $SAILFISHOSSDKDIR/vmshare/ --shared-src $HOME
bin/sdktool addMerTarget --mer-targets-dir $SAILFISHOSSDKDIR/mersdk/targets/ --target-name SailfishOS-i486 --qmake-query $SAILFISHOSSDKDIR/mersdk/dumps/qmake.query.SailfishOS-i486 --gcc-dumpmachine $SAILFISHOSSDKDIR/mersdk/dumps/gcc.dumpmachine.SailfishOS-i486
bin/sdktool addDev --id SailfishOS\ Emulator --name SailfishOS\ Emulator --osType Mer.Device.Type.i486 --origin 0 --type 0 --host localhost --sshPort 2223 --uname nemo --authentication 1 --keyFile $HOME/.ssh/mer-qt-creator-rsa --timeout 10 --freePorts 10000-10019
