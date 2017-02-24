#!/bin/bash
#
# This script allows to clone a Sailfish OS Emulator virtual machine.
#
# Copyright (C) 2017 Jolla Oy
# Contact: Martin Kampas <martin.kampas@jolla.com>
# All rights reserved.
#
# You may use this file under the terms of BSD license as follows:
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#   * Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#   * Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#   * Neither the name of the Jolla Ltd nor the
#     names of its contributors may be used to endorse or promote products
#     derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

set -u
set -e
shopt -s extglob

SELF=$(basename "$0")

UNAME_SYSTEM=$(uname -s)
if [[ $UNAME_SYSTEM != +(Linux|Darwin) ]]; then
    echo "$SELF: Not supported on this platform" >&2
    exit 1
fi

if ! which xmlpatterns &>/dev/null; then
    echo "$SELF: The tool 'xmlpatterns' is not in PATH. Ensure you have Qt installed and tools are in PATH" >&2
    exit 1
fi

if ! which vboxmanage &>/dev/null; then
    # VBox 5 installs here for compatibility with Mac OS X 10.11
    if [[ :$PATH: != *:/usr/local/bin:* ]]; then
        PATH="/usr/local/bin:$PATH"
    fi

    if ! which vboxmanage &>/dev/null; then
        echo "$SELF: The tool 'vboxmanage' is not in PATH" >&2
        exit 1
    fi
fi

CLONE_LIMIT=5
EMULATOR_VM="SailfishOS Emulator"
SDK_ROOT="$HOME/SailfishOS"
BASE_FOLDER="$SDK_ROOT/emulator"
CONFIG_DIR_SUFFIX="-config"
EMULATOR_SSH_DIR="$SDK_ROOT/emulator/1/ssh"
EMULATOR_CONFIG_DIR="$SDK_ROOT/vmshare"
DEF_SSH_PORT=2223
DEF_QMLLIVE_PORT=10234
DEF_FREE_PORT_FIRST=10000
DEF_FREE_PORT_LAST=10019
KEY_FILE="$SDK_ROOT/vmshare/ssh/private_keys/SailfishOS_Emulator/nemo"
DCONF_DB_FILE="device-model.txt"
EGLFS_CONFIG_FILE="65-emul-wayland-ui-scale.conf"
DEVICES_XML_FILE="devices.xml"

if [[ $UNAME_SYSTEM == Darwin ]]; then
    MODELS_XML="$SDK_ROOT/bin/Qt Creator.app/Contents/Resources/SailfishOS-SDK/qtcreator/mersdk-device-models.xml"
    sdktool() { $SDK_ROOT/bin/Qt\ Creator.app/Contents/Resources/sdktool "$@"; }
else
    MODELS_XML="$SDK_ROOT/share/qtcreator/SailfishOS-SDK/qtcreator/mersdk-device-models.xml"
    sdktool() { $SDK_ROOT/libexec/qtcreator/sdktool "$@"; }
fi

OPT_CLONE_NAME=
OPT_INTERNAL=
OPT_INTERNAL_DCONF_DB=
OPT_INTERNAL_EGLFS_CONFIG=
OPT_INTERNAL_VIDEO_MODE=
OPT_INTERNAL_REMOVE_ONE=
OPT_LANDSCAPE=
OPT_MODEL=
OPT_REMOVE_ALL=
OPT_SCALE_DOWN=

synopsis_internal()
{
    [[ $OPT_INTERNAL ]] || return

    cat <<EOF


       Internal commands:
       $SELF {-m|--model DEVICE_MODEL} [-s|--scale-down] --internal-{dconf-db|eglfs-config|video-mode}
       $SELF --internal-remove-one CLONE_NAME
       $SELF --internal-help
EOF
}

synopsis()
{
    cat <<EOF
$SELF is a tool to clone Sailfish OS Emulator virtual machine
Usage: $SELF {-m|--model DEVICE_MODEL} [-s|--scale-down] [-l|--landscape] CLONE_NAME
       $SELF --list-models
       $SELF --remove-all
       $SELF -h|--help$(synopsis_internal)

ATTENTION: This is an internal tool! Use at your own risk!

EOF
}

print_brief_help()
{
    cat <<EOF
$(synopsis)

Pass '--help' to show full description of available options.
EOF
}

print_help()
{
    cat <<EOF
$(synopsis)

Unless you want to experiment, follow these recommendations:

1. Quit Qt Creator, Qt QmlLive and all virtual machines before using this tool
2. Remove (backup) ~/.config/SailfishOS-SDK to start with clean configuration
3. Do not change any options on the cloned emulators from within Qt Creator

The maximum number of clones is limited to $CLONE_LIMIT.

OPTIONS
  CLONE_NAME
    The name for the cloned virtual machine

  -m|--model DEVICE_MODEL
    Choose the device model to emulate. Invoke with '--list-models' to list the
    known device models.

  -s|--scale-down
    Scale down the screen to half of its original size. Pass multiple times to
    scale down more.

  -l|--landscape
    Use landscape orientation

  --list-models
    List the known device models.

  --remove-all
    Remove all previously created clones. Removing single clone is not possible.
EOF
}

bad_usage()
{
    synopsis >&2
    echo "$SELF: $*" >&2
}

list_clones()
{
    (
        cd "$BASE_FOLDER"
        find * -maxdepth 0 -type d -name "*$CONFIG_DIR_SUFFIX" |sed "s/$CONFIG_DIR_SUFFIX$//"
    )
}

xq()
{
    local xml="$1"
    local query="$2"
    xmlpatterns <(echo "$query") "$xml"
}

xq_models()
{
    local query="$1"

    if [[ $OPT_INTERNAL ]]; then
        echo "Executing query: $query" >&2
    fi

    xq "$MODELS_XML" "$query"
}

list_models()
{
    xq_models '//value[@key="MerDeviceModel.Name"]/concat(text(), ";")' \
        |tr ';' '\n' |sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//' |grep . | {
        if [[ $# -gt 0 ]]; then
            echo "Known device models:"
            sed 's/^/ - /'
        else
            cat
        fi
    }
}

xq_model()
{
    local property="$1"

    xq_models '
          //value[@key="MerDeviceModel.Name" and text() = "'"$OPT_MODEL"'"]
                 /../value[@key="MerDeviceModel.'$property'"]/text()
          '
}

dconf_db()
{
    xq_model DconfDb
}

eglfs_config()
{
    local display_resolution=$(xq_model DisplayResolution)
    display_resolution=${display_resolution%%+*}

    local display_size=$(xq_model DisplaySize)
    display_size=${display_size%%+*}

    if [[ $OPT_LANDSCAPE ]]; then
        display_resolution=${display_resolution#*x}x${display_resolution%x*}
    fi

    if [[ $OPT_SCALE_DOWN ]]; then
        echo "QT_QPA_EGLFS_SCALE=$OPT_SCALE_DOWN"
    else
        echo "#QT_QPA_EGLFS_SCALE="
    fi

    cat <<EOF
QT_QPA_EGLFS_WIDTH=${display_resolution%x*}
QT_QPA_EGLFS_HEIGHT=${display_resolution#*x}
QT_QPA_EGLFS_PHYSICAL_WIDTH=${display_size%x*}
QT_QPA_EGLFS_PHYSICAL_HEIGHT=${display_size#*x}
EOF
}

video_mode()
{
    local display_resolution=$(xq_model DisplayResolution)
    display_resolution=${display_resolution%%+*}

    if [[ $OPT_LANDSCAPE ]]; then
        display_resolution=${display_resolution#*x}x${display_resolution%x*}
    fi

    local hres=${display_resolution%x*}
    local vres=${display_resolution#*x}

    if [[ $OPT_SCALE_DOWN ]]; then
        hres=$((hres / OPT_SCALE_DOWN))
        vres=$((vres / OPT_SCALE_DOWN))
    fi

    local depth=32

    echo "${hres}x${vres}x${depth}"
}

devices_xml()
{
    local index=$(($1 + 1))
    cat <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<devices>
    <device name="$OPT_CLONE_NAME" type="vbox">
        <index>$index</index>
        <subnet>10.220.220</subnet>
        <sshkeypath>ssh/private_keys/SailfishOS_Emulator</sshkeypath>
        <mac>08:00:5A:11:00:0$index</mac>
    </device>
</devices>
EOF
}

while [[ $# -gt 0 ]]; do
    OPTION=$1

    if [[ $OPTION =~ internal ]]; then
        OPT_INTERNAL=1
    fi

    case $OPTION in
        -h)
            print_brief_help
            exit
            ;;
        --help|--internal-help|--help-internal)
            print_help
            exit
            ;;
        --list)
            list_clones
            exit
            ;;
        --list-models)
            list_models
            exit
            ;;
        -m|--model)
            shift
            OPT_MODEL=$1
            if [[ ! $OPT_MODEL ]]; then
                bad_usage "Missing argument to '$OPTION'"
                list_models - >&2
                exit 1
            fi
            ;;
        --remove-all)
            OPT_REMOVE_ALL=1
            ;;
        -s|--scale-down)
            OPT_SCALE_DOWN=$((${OPT_SCALE_DOWN:-1} * 2))
            ;;
        -l|--landscape)
            OPT_LANDSCAPE=1
            ;;
        --internal-dconf-db)
            OPT_INTERNAL_DCONF_DB=1
            ;;
        --internal-eglfs-config)
            OPT_INTERNAL_EGLFS_CONFIG=1
            ;;
        --internal-video-mode)
            OPT_INTERNAL_VIDEO_MODE=1
            ;;
        --internal-remove-one)
            OPT_INTERNAL_REMOVE_ONE=1
            ;;
        -*)
            bad_usage "Unknown option '$1'"
            exit 1
            ;;
        *)
            if [[ $OPT_CLONE_NAME ]]; then
                bad_usage "Can only create one clone at a time"
                exit 1
            fi

            OPT_CLONE_NAME=$1
            ;;
    esac
    shift
done

if [[ $OPT_INTERNAL_DCONF_DB ]]; then
    if [[ ! $OPT_MODEL ]]; then
        bad_usage "'--model' not specified"
        exit 1
    fi
    dconf_db
    exit
fi

if [[ $OPT_INTERNAL_EGLFS_CONFIG ]]; then
    if [[ ! $OPT_MODEL ]]; then
        bad_usage "'--model' not specified"
        exit 1
    fi
    eglfs_config
    exit
fi

if [[ $OPT_INTERNAL_VIDEO_MODE ]]; then
    if [[ ! $OPT_MODEL ]]; then
        bad_usage "'--model' not specified"
        exit 1
    fi
    video_mode
    exit
fi

if [[ $OPT_REMOVE_ALL ]]; then
    (
        IFS=$'\n'
        for clone in $(list_clones); do
            $0 --internal-remove-one "$clone" || true
        done
    )
    exit
fi

if [[ ! $OPT_CLONE_NAME ]]; then
    bad_usage "Clone name expected"
    exit 1
fi

SNAPSHOT_NAME="For $OPT_CLONE_NAME clone"
SNAPSHOT_DESC="Created by the $SELF tool for the '$OPT_CLONE_NAME' clone."
CONFIG_DIR="$BASE_FOLDER/$OPT_CLONE_NAME$CONFIG_DIR_SUFFIX"

create()
{
    if [[ ! $OPT_MODEL ]]; then
        bad_usage "'--model' required"
        list_models - >&2
        exit 1
    fi

    if ! list_models |grep -q --line-regexp -F "$OPT_MODEL"; then
        bad_usage "Unknown device model '$OPT_MODEL'" >&2
        list_models - >&2
        exit 1
    fi

    index=$(ls -1 "$BASE_FOLDER"/*/*.vbox |wc -l)
    if [[ $index -ge $CLONE_LIMIT ]]; then
        echo "Maximum number of clones ($CLONE_LIMIT) reached. Cannot create more clones" >&2
        exit 1
    fi

    echo "Cloning '$EMULATOR_VM' as '$OPT_CLONE_NAME', using device model '$OPT_MODEL'" >&2

    vboxmanage snapshot "$EMULATOR_VM" \
               take "$SNAPSHOT_NAME" \
               --description "$SNAPSHOT_DESC"

    vboxmanage clonevm "$EMULATOR_VM" \
               --snapshot "$SNAPSHOT_NAME" \
               --options link \
               --name "$OPT_CLONE_NAME" \
               --basefolder "$BASE_FOLDER" \
               --register

    macaddress2="08:00:5A:11:00:0$((index + 1))"
    ssh_port=$((DEF_SSH_PORT + index * 100))
    qmllive_port=$((DEF_QMLLIVE_PORT + index * 100))

    vboxmanage modifyvm "$OPT_CLONE_NAME" \
               --cableconnected1 on \
               --cableconnected2 on \
               --macaddress2 ${macaddress2//:/} \
               --natpf1 delete guestssh \
               --natpf1 guestssh,tcp,127.0.0.1,$ssh_port,,22 \
               --natpf1 delete qmllive_1 \
               --natpf1 qmllive_1,tcp,127.0.0.1,$qmllive_port,,$DEF_QMLLIVE_PORT \
               $(
        for ((def_free_port=DEF_FREE_PORT_FIRST; def_free_port <= DEF_FREE_PORT_LAST; def_free_port++)); do
            free_port=$((def_free_port + index * 100))
            echo --natpf1 delete freeport_$def_free_port \
                 --natpf1 freeport_$free_port,tcp,127.0.0.1,$free_port,,$free_port
        done
               )

    vboxmanage setextradata "$OPT_CLONE_NAME" \
               CustomVideoMode1 "$(video_mode)"

    mkdir -p "$CONFIG_DIR"

    vboxmanage sharedfolder \
               remove "$OPT_CLONE_NAME" \
               --name config
    vboxmanage sharedfolder \
               add "$OPT_CLONE_NAME" \
               --name config \
               --hostpath "$CONFIG_DIR" \
               --readonly

    dconf_db > "$CONFIG_DIR/$DCONF_DB_FILE"
    eglfs_config > "$CONFIG_DIR/$EGLFS_CONFIG_FILE"
    devices_xml $index > "$CONFIG_DIR/$DEVICES_XML_FILE"

    free_port_first=$((DEF_FREE_PORT_FIRST + index * 100))
    free_port_last=$((DEF_FREE_PORT_LAST + index * 100))

    view_scaled=false
    [[ $OPT_SCALE_DOWN ]] && view_scaled=true

    # Cannot pass --merSharedConfig=$CONFIG_DIR. With this Qt Creator would list
    # absolute path to SSH key in devices.xml, so it could not be resolved
    # inside VM. OTOH passing $EMULATOR_CONFIG_DIR breaks emulator mode changing
    # from within Qt Creator.

    sdktool addDev --id "$OPT_CLONE_NAME" \
            --name "$OPT_CLONE_NAME" \
            --osType Mer.Device.Type \
            --origin 0 \
            --sdkProvided bool:false \
            --type 1 \
            --host localhost \
            --sshPort $ssh_port \
            --uname nemo \
            --authentication 1 \
            --keyFile "$KEY_FILE" \
            --timeout 30 \
            --freePorts $free_port_first-$free_port_last \
            --version int:0 \
            --virtualMachine QString:"$OPT_CLONE_NAME" \
            --merMac QString:$macaddress2 \
            --merSubnet QString:10.220.220 \
            --merSharedSsh QString:"$EMULATOR_SSH_DIR" \
            --merSharedConfig QString:"$EMULATOR_CONFIG_DIR" \
            --merDeviceModel QString:"$OPT_MODEL" \
            MER_DEVICE_VIEW_SCALED bool:$view_scaled \
            MER_DEVICE_QML_LIVE_PORTS QString:$qmllive_port
}

remove()
{
    echo "Removing '$OPT_CLONE_NAME' clone" >&2

    set +e

    sdktool rmDev --id "$OPT_CLONE_NAME"

    rm -f "$CONFIG_DIR/$DEVICES_XML_FILE"
    rm -f "$CONFIG_DIR/$DCONF_DB_FILE"
    rm -f "$CONFIG_DIR/$EGLFS_CONFIG_FILE"
    rmdir "$CONFIG_DIR"

    vboxmanage unregistervm "$OPT_CLONE_NAME" --delete

    vboxmanage snapshot "$EMULATOR_VM" delete "$SNAPSHOT_NAME"
}

if [[ $OPT_INTERNAL_REMOVE_ONE ]]; then
    remove
else
    create
fi
