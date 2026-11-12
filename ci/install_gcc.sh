#!/usr/bin/env bash
# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#
# Install a GCC release from the Ubuntu toolchain test archive and select it.
#
# `add-apt-repository ppa:...` does two things: it writes the archive's source
# entry, and it asks the Launchpad web service for the archive's signing key.
# The second is a request to a different service from the one that serves the
# packages, and when that service answers 500 the step fails before a single
# package has been asked for, on a runner that could have installed the
# compiler. Every Linux job starts with this step, so one such answer fails the
# whole run.
#
# Nothing here needs to be looked up. The source entry is fixed, and the key is
# the same one it has been since 2009, so both are written from this
# repository: ci/ubuntu-toolchain-r-test.asc is that key, fingerprint
# 60C3 1780 3A41 BA51 845E 371A 1E93 77A2 BA9E F27F. apt verifies the archive
# against it exactly as before; what is gone is the lookup.
#
# The archive itself can still fail to answer, so the two requests that remain
# are retried.
#
#   $1  GCC major version, e.g. 16

set -euo pipefail

if [[ $# -ne 1 || ! "$1" =~ ^[0-9]+$ ]]; then
    echo "Usage: $0 <gcc-major-version>" >&2
    exit 2
fi
version="$1"

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
key="${here}/ubuntu-toolchain-r-test.asc"
if [[ ! -s "${key}" ]]; then
    echo "The archive key is missing: ${key}" >&2
    exit 1
fi

# shellcheck disable=SC1091
codename="$(. /etc/os-release && printf '%s' "${VERSION_CODENAME}")"
if [[ -z "${codename}" ]]; then
    echo "Cannot tell which Ubuntu release this is." >&2
    exit 1
fi

sudo install -d -m 0755 /etc/apt/keyrings
sudo install -m 0644 "${key}" /etc/apt/keyrings/ubuntu-toolchain-r-test.asc

sudo tee /etc/apt/sources.list.d/ubuntu-toolchain-r-test.sources >/dev/null <<SOURCE
Types: deb
URIs: https://ppa.launchpadcontent.net/ubuntu-toolchain-r/test/ubuntu/
Suites: ${codename}
Components: main
Signed-By: /etc/apt/keyrings/ubuntu-toolchain-r-test.asc
SOURCE

# Five attempts, waiting longer each time: 15, 30, 45 and 60 seconds, so a
# service that is down for a couple of minutes is outlasted and one that is down
# for the afternoon fails in under three.
retry() {
    local attempt
    for attempt in 1 2 3 4 5; do
        if "$@"; then
            return 0
        fi
        if (( attempt < 5 )); then
            echo "Attempt ${attempt} of 5 failed: $*" >&2
            sleep $(( attempt * 15 ))
        fi
    done
    echo "Giving up after 5 attempts: $*" >&2
    return 1
}

# Error-Mode=any: by default `apt-get update` reports an index it could not
# fetch as a warning and exits 0, and the failure then surfaces one command
# later as "unable to locate package gcc-${version}" — which retrying the
# install cannot fix, because the index is still the stale one.
retry sudo apt-get update -o APT::Update::Error-Mode=any
retry sudo apt-get install --yes "gcc-${version}" "g++-${version}"

"gcc-${version}" --version | head -1

if [[ -n "${GITHUB_ENV:-}" ]]; then
    {
        echo "CC=gcc-${version}"
        echo "CXX=g++-${version}"
    } >> "${GITHUB_ENV}"
fi
