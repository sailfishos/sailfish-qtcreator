#!/bin/bash

set -o nounset

# Either pass this in environment or as the only command line argument
: ${AMBIENCE_URL:=$1}

TYPE_AMBIENCE=1 # AmbienceContent::Type::Ambience

call()
{
    local member=$1
    local args=("${@:2}")

    dbus-send --print-reply=literal --session --type=method_call --dest=com.jolla.ambienced \
              /com/jolla/ambienced com.jolla.ambienced."$member" ${args:+"${args[@]}"}
}

parse()
{
    local wanted_type=$1

    local type= value= garbage=
    read type value garbage || return
    [[ $type == "$wanted_type" ]] || return
    [[ ! $garbage ]] || return
    ! read garbage || return
    printf "%s" "$value"
}

ambienceId=$(call createAmbience string:"$AMBIENCE_URL") || exit
ambienceId=$(parse int64 <<<"$ambienceId") || exit
call remove int32:"$TYPE_AMBIENCE" int64:"$ambienceId" || exit
call refreshAmbiences || exit
call setAmbience string:"$AMBIENCE_URL" >/dev/null || exit
