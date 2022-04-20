include_directories(../piControl/)

set(PICONTROLIF ../piControl/piTest/piControlIf.c)
set(COMM_OBJ piConfigParser/piConfigParser.c)

add_executable(piModbusMaster
	${PICONTROLIF}
	${COMM_OBJ}
	ComAndDataProcessor.c
	ModbusMasterThread.c
	piModbusMaster.c
	Scheduler.c)

target_link_libraries(piModbusMaster modbus rt pthread json-c)

set_property(TARGET piModbusMaster PROPERTY C_STANDARD 99)

install(TARGETS piModbusMaster
	RUNTIME DESTINATION sbin
)

add_executable(piModbusSlave
	${PICONTROLIF}
	${COMM_OBJ}
	ModbusSlaveThread.c
	piModbusSlave.c
	piProcessImageAccess.c)

target_link_libraries(piModbusSlave modbus pthread json-c)

set_property(TARGET piModbusSlave PROPERTY C_STANDARD 99)

install(TARGETS piModbusSlave
	RUNTIME DESTINATION sbin
)

