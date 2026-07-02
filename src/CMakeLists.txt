# SPDX-License-Identifier: GPL-2.0-or-later
#
# SPDX-FileCopyrightText: 2023-2026 KUNBUS GmbH
#

include_directories(../piTest/include/)
include_directories(../piTest/lib/piControl/src/)
set(TARGET_SERVER piModbusServer)
set(TARGET_CLIENT piModbusClient)

set(PICONTROLIF ../piTest/src/piControlIf.c)
set(COMM_OBJ piConfigParser/piConfigParser.c)

add_executable(${TARGET_CLIENT}
	${PICONTROLIF}
	${COMM_OBJ}
	ComAndDataProcessor.c
	ModbusClientThread.c
	piModbusClient.c
	Scheduler.c)

target_link_libraries(${TARGET_CLIENT} modbus rt pthread json-c)

set_property(TARGET ${TARGET_CLIENT} PROPERTY C_STANDARD 99)
target_compile_options(${TARGET_CLIENT} PRIVATE
	-Wall -Wextra -Wpedantic -Werror
)

install(TARGETS ${TARGET_CLIENT}
	RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR}
)

add_executable(${TARGET_SERVER}
	${PICONTROLIF}
	${COMM_OBJ}
	ModbusServerThread.c
	piModbusServer.c
	piProcessImageAccess.c)

target_link_libraries(${TARGET_SERVER} modbus pthread json-c)

set_property(TARGET ${TARGET_SERVER} PROPERTY C_STANDARD 99)
target_compile_options(${TARGET_SERVER} PRIVATE
	-Wall -Wextra -Wpedantic -Werror
)

install(TARGETS ${TARGET_SERVER}
	RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR}
)
