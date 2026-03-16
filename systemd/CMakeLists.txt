# SPDX-FileCopyrightText: 2024-2026 KUNBUS GmbH
#
# SPDX-License-Identifier: GPL-2.0-or-later

include(FindPkgConfig)
pkg_check_modules(SYSTEMD REQUIRED "systemd")
pkg_get_variable(SYSTEMD_SYSTEMUNITDIR "systemd" "systemdsystemunitdir")

install(FILES pimodbus-client.service pimodbus-server.service
	DESTINATION ${SYSTEMD_SYSTEMUNITDIR})
