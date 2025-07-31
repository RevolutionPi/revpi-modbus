# SPDX-License-Identifier: GPL-2.0-or-later
#
# Copyright 2023 KUNBUS GmbH
#

cmake_minimum_required(VERSION 3.7)

project(revpi-modbus LANGUAGES C)

include(GNUInstallDirs)

add_subdirectory(src)
add_subdirectory(systemd)

