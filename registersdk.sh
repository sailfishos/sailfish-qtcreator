#! /bin/sh

# Registers MerVM, targets and emulator device of an installed SailfishOS SDK

export SAILFISHOSSDKDIR=$HOME/SailfishOS

bin/sdktool addMerSdk --installdir $SAILFISHOSSDKDIR --vm-name MerSDK --autodetected true --shared-home $HOME --shared-targets $SAILFISHOSSDKDIR/mersdk/targets/ --shared-ssh $SAILFISHOSSDKDIR/mersdk/ssh/ --host localhost --username mersdk --private-key-file $SAILFISHOSSDKDIR/vmshare/ssh/private_keys/engine/mersdk --ssh-port 2222 --www-port 8080 --shared-config $SAILFISHOSSDKDIR/vmshare/ --shared-src $HOME
bin/sdktool addMerTarget --mer-targets-dir $SAILFISHOSSDKDIR/mersdk/targets/ --target-name SailfishOS-i486 --qmake-query $SAILFISHOSSDKDIR/mersdk/dumps/qmake.query.SailfishOS-i486 --gcc-dumpmachine $SAILFISHOSSDKDIR/mersdk/dumps/gcc.dumpmachine.SailfishOS-i486
bin/sdktool addDevice --internalid SailfishOS\ Emulator --name SailfishOS\ Emulator --ostype Mer.Device.Type.i486 --origin autoDetected --type emulator --host localhost --sshport 2223 --uname nemo --authentication privateKey --keyfile $HOME/.ssh/mer-qt-creator-rsa --timeout 10 --freeportsspec 10000-10019
