#!/bin/bash
#
# Copyright (C) 2021 Jolla Ltd.
# Contact: http://jolla.com/
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# - Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# - Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in
#   the documentation and/or other materials provided with the
#   distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.


export SFDK_GENDOC_RUN=1

usage()
{
    cat <<END
Usage: gendoc.sh [--incremental] <output-directory>

Generate sfdk documentation.

Results will be stored under the given <directory>.

OPTIONS
    --incremental
        Only create missing documentation files.
END
}

bad_usage()
{
    printf '%s\n' "$*"
    printf "Pass '--help' for more information.\n"
} >&2

a2x()
{
    command a2x "$@" >&2
}

# Suppress command's stderr unless it exits with non-zero
silent()
{
    local stderr=
    { stderr=$("$@" 3>&1 1>&2 2>&3 3>&-); } 2>&1
    local rc=$?
    [[ $rc -eq 0 ]] || printf >&2 "%s\n" "$stderr"
    return $rc
}

gendoc()
{
    local command=("$@")
    local temp=

    echo "gendoc.sh: Processing '${command[*]}'..." >&2

    local ok=
    gendoc_cleanup()
    (
        trap 'echo cleaning up...' INT TERM HUP
        if [[ $temp ]]; then
            rm -f "$temp"
        fi

        if [[ ! $ok ]]; then
            echo "gendoc.sh: Error while processing '${command[*]}'" >&2
        fi
    )
    trap 'gendoc_cleanup; trap - RETURN' RETURN
    trap 'return 1' INT TERM HUP

    temp=$(mktemp --tmpdir=. sfdk.adoc.XXX) || return

    "${command[@]}" >"$temp" || return

    silent a2x --format manpage --verbose "$temp" || return
    ok=1
}

as_needed()
{
    [[ $OPT_INCREMENTAL && -e $1.1.gz ]] || "${@:2}"
}

parse_options()
{
    OPT_INCREMENTAL=
    OPT_OUTPUT_DIRECTORY=
    OPT_USAGE=

    local positional=()

    while (($# > 0)); do
        case $1 in
            -h|--help)
                OPT_USAGE=1
                return
                ;;
            --incremental)
                OPT_INCREMENTAL=1
                ;;
            -*)
                bad_usage "Unrecognized option: '$1'"
                return 1
                ;;
            *)
                positional+=("$1")
                ;;
        esac
        shift
    done

    if ((${#positional[*]} == 0)); then
        bad_usage "Argument expected"
        return 1
    fi

    if ((${#positional[*]} > 1)); then
        bad_usage "Unexpected argument: '${positional[1]}'"
        return 1
    fi

    OPT_OUTPUT_DIRECTORY=${positional[0]}
}

main()
{
    parse_options "$@" || return

    if [[ $OPT_USAGE ]]; then
        usage
        return
    fi

    cd "$OPT_OUTPUT_DIRECTORY" || return

    as_needed "sfdk" gendoc sfdk --help || return
    as_needed "sfdkmanual" gendoc sfdk --help-all || return
    for domain in $(sfdk misc inspect domains); do
        [[ $domain == general ]] && continue
        as_needed "sfdkmanual-$domain" gendoc sfdk --help-"$domain" || return
    done
    for command in $(sfdk misc inspect commands); do
        as_needed "sfdk-$command" gendoc sfdk "$command" --help || return
    done

    ! ls *.1 &>/dev/null || gzip *.1
}

main "$@"
