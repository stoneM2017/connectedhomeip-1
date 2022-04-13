#!/usr/bin/env bash

#
#    Copyright (c) 2020 Project CHIP Authors
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

set -e

# Build script for GN examples GitHub workflow.

MATTER_ROOT=$(dirname "$0")/../../

source "$(dirname "$0")/../../scripts/activate.sh"

EXAMPLE_DIR=$MATTER_ROOT/examples/$1/bouffalolab/bl702
OUTPUT_DIR=$MATTER_ROOT/examples/$1/bouffalolab/bl702/out/debug

export BL_IOT_SDK_PATH=$MATTER_ROOT/third_party/bouffalolab/bl_iot_sdk/repo

if [[ "$OSTYPE" == "linux-gnu"* ]]; then
export PATH="$BL_IOT_SDK_PATH/toolchain/riscv/Linux/bin:$PATH"
elif [[ "$OSTYPE" == "darwin"* ]]; then
export PATH="$BL_IOT_SDK_PATH/toolchain/riscv/Darwin/bin:$PATH"
fi

GN_ARGS=()

NINJA_ARGS=()
for arg; do
    case $arg in
        -v)
            NINJA_ARGS+=(-v)
            ;;
        *=*)
            GN_ARGS+=("$arg")
            ;;
        *import*)
            GN_ARGS+=("$arg")
            ;;
        *)
            if [[ $1 == $arg ]]; then
                continue
            fi
            echo >&2 "invalid argument: $arg"
            exit 2
            ;;
    esac
done

#gn gen --check --fail-on-unused-args --root="$EXAMPLE_DIR" "$OUTPUT_DIR" --args="${GN_ARGS[*]}"
gn gen --fail-on-unused-args --root="$EXAMPLE_DIR" "$OUTPUT_DIR" --args="${GN_ARGS[*]}"

ninja -C "$OUTPUT_DIR" "${NINJA_ARGS[@]}"
