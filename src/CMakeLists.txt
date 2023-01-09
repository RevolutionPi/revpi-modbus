# SPDX-License-Identifier: GPL-2.0-or-later
#
# Copyright 2023 KUNBUS GmbH
#

include_directories(../piControl/)
set(TARGET_SLAVE piModbusSlave)
set(TARGET_MASTER piModbusMaster)

set(PICONTROLIF ../piControl/piTest/piControlIf.c)
set(COMM_OBJ piConfigParser/piConfigParser.c)

add_executable(${TARGET_MASTER}
	${PICONTROLIF}
	${COMM_OBJ}
	ComAndDataProcessor.c
	ModbusMasterThread.c
	piModbusMaster.c
	Scheduler.c)

target_link_libraries(${TARGET_MASTER} modbus rt pthread json-c)

set_property(TARGET ${TARGET_MASTER} PROPERTY C_STANDARD 99)
target_compile_options(${TARGET_MASTER} PRIVATE
	-Wall -Wextra -Wpedantic -Werror
)

install(TARGETS ${TARGET_MASTER}
	RUNTIME DESTINATION sbin
)

add_executable(${TARGET_SLAVE}
	${PICONTROLIF}
	${COMM_OBJ}
	ModbusSlaveThread.c
	piModbusSlave.c
	piProcessImageAccess.c)

target_link_libraries(${TARGET_SLAVE} modbus pthread json-c)

set_property(TARGET ${TARGET_SLAVE} PROPERTY C_STANDARD 99)
target_compile_options(${TARGET_SLAVE} PRIVATE
	-Wall -Wextra -Wpedantic -Werror
)

install(TARGETS ${TARGET_SLAVE}
	RUNTIME DESTINATION sbin
)
