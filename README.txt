**Upcoming changes
--------------------------------------------------------------------------------
* in the function determineNextEvent () the last target time will be the
Period length added to calculate the next execution time.
If the connection was interrupted for a long time, however, they accumulate
many versions of the then with a shorter period length
(tv_earliest_next_trigger_time) are processed. If the cycle times anyway
are already tightly laid out, this can lead to an overload. Must
be changed. The next execution time must be the current time plus
Be cycle time.


**Error Codes in Process Image
--------------------------------------------------------------------------------

ModbusTCP
-----------------------------
Modbus_Master_Status
16	0x10		Initialization is failed
17	0x11		This device is not readable(false IP, Cabel?)

Modbus_Action_Status
1	ILLEGAL FUNCTION	The applied function code is not allowed.
				Check whether the right function code is used.
2	ILLEGAL DATA ADDRESS	The applied address is not valid.
				The register is either read-only or not existed.
				Please check the address.
3	ILLEGAL DATA VALUE	At least one part of the applied data is invalid.
				It is possible that the count of register is exceed.
				Please check the value.
13	INVALID DATA		The Slave answered with an unknown package to me.
				This can occur, for example, after the connection
				has been interrupted.
				Please check your wiring.
110	CONNECTION TIMED OUT	The slave didn't respond in time or not at all.
				Please check your configuration and cabling.

ModbusRTU
-----------------------------
Modbus_Master_Status
16	0x10		Initialization is failed
17	0x11		Serial connection can't be opened (wrong device name,
			invalid configuration)

Modbus_Action_Status
1	ILLEGAL FUNCTION	The applied function code is not allowed.
				Check whether the right function code is used.
2	ILLEGAL DATA ADDRESS	The applied address is not valid.
				The register is either read-only or not existed.
				Please check the address.
3	ILLEGAL DATA VALUE	At least one part of the applied data is invalid.
				It is possible that the count of register is exceed.
				Please check the value.
11				Resource temporarily unavailable
12	Invalid CRC		A disturbed packet was received from the slave.
				This can occur, for example, after the connection
				has been interrupted.
				Please check your wiring.
13	INVALID DATA		An incomplete packet was received from the slave.
				This can occur, for example, after the connection
				has been interrupted.
				Please check your wiring.
110	CONNECTION TIMED OUT	The slave didn't respond in time or not at all.
				Please check your configuration and cabling.
104				Connection reset by peer

all internal error messages that do not occur and do not need to be documented
-----------------------------
1	Illegal function
2	Illegal data address
3	Illegal data value
4	Slave device or server failure
5	Acknowledge
6	Slave device or server is busy
7	Negative acknowledge
8	Memory parity error
10	Gateway path unavailable
11	Target device failed to respond
12	Invalid CRC
13	Invalid data
14	Invalid exception code
15	Too many data


**Example configuration for the config.rsc
--------------------------------------------------------------------------------
{

"config": {
	"modbusActions": [
		{
			"SlaveAddress" : 1,
			# Address of the Modbus slave which is addressed by the
			# command

			"FunctionCode" : 2,
			# Modbus function code see below

			"RegisterAddress" : 1,
			# Modbus register address (start address) of the slave

			"QuantityOfRegisters" : 8,
			# Number of Modbus registers that are written / read

			"ActionInterval" : 100000,
			# Time interval in which the command is sent from the
			# master in µsec. For the user, information in 1 ms
			# steps is sufficient

			"ProcessImageStartByte" : 2,
			# The offset in the Pi process image for the Modbus data

			"Action ID" : 1
			# Ascending ID for the command for information in the
			# event of an error
		},
		{
			"SlaveAddress" : 1,
			"FunctionCode" : 3,
			"RegisterAddress" : 1200,
			"QuantityOfRegisters" : 8,
			"ActionInterval" : 200000,
			"ProcessImageStartByte" : 18,
			"Action ID" : 2
		},
		{
			"SlaveAddress" : 2,
			"FunctionCode" : 16,
			"RegisterAddress" : 1024,
			"QuantityOfRegisters" : 8,
			"ActionInterval" : 500000,
			"ProcessImageStartByte" : 2,
			"Action ID" : 3
		}
	]
}

}



Modbus Function Codes:
-----------------------------
Important function codes are marked with '*'.The other function codes may not
need to be supported.

*	eREAD_COILS			= 0x01,
*	eREAD_DISCRETE_INPUTS		= 0x02,
*	eREAD_HOLDING_REGISTERS		= 0x03,
*	eREAD_INPUT_REGISTERS		= 0x04,
*	eWRITE_SINGLE_COIL		= 0x05,
*	eWRITE_SINGLE_REGISTER		= 0x06,
	eREAD_EXCEPTION_STATUS		= 0x07,	//serial line only, not supported
						//by libmodbus v3.0.6
*	eWRITE_MULTIPLE_COILS		= 0x0F,
*	eWRITE_MULTIPLE_REGISTERS	= 0x10,
	eREPORT_SLAVE_ID			= 0x11,
	eWRITE_MASK_REGISTER		= 0x16,
	eWRITE_AND_READ_REGISTERS	= 0x17,

Coils and Discrete Inputs are one bit in size. In the process image, however,
one byte should be reserved for each bit.Holding registers and input registers
have a size of 2 bytes.

For each Modbus command, an additional byte must be reserved in the Pi process
image for error messages.

For the virtual device, a byte must also be reserved in the Pi process image for
error messages.

For each Modbus command and the virtual device, a bit must be reserved to reset
the error byte in the Pi process image
