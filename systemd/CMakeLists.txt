include(FindPkgConfig)
pkg_check_modules(SYSTEMD REQUIRED "systemd")
pkg_get_variable(SYSTEMD_SYSTEMUNITDIR "systemd" "systemdsystemunitdir")

install(FILES pimodbus-master.service pimodbus-slave.service
	DESTINATION ${SYSTEMD_SYSTEMUNITDIR})
