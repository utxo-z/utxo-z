#!/usr/bin/env bash
# Copyright (c) 2016-present Knuth Project developers.
# Distributed under the MIT software license, see COPYING.
set -euo pipefail

conan profile detect --force
# Public dependency downloads need no release credentials. Keep this remote
# separate from the utxoz publishing/verification remote.
conan remote add kth-deps https://packages.kth.cash/api/ --force
