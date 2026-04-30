# CoreModel List Example
#
# Copyright (c) 2022-2026 Corellium Inc.
# SPDX-License-Identifier: Apache-2.0

from coremodel import *

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("usage: python3 coremodel-list.py <address> <port> ")
        sys.exit(1)

    address =  sys.argv[1]
    port = sys.argv[2]

    cm0 = CoreModel("cm0", address, port, "./../../")

    cm0.device_list()

