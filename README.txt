piModbus Master und Slave Changelog
=============================================================




Anstehende Änderungen
--------------------------------------------------------------
* in der Funktion determineNextEvent() wird wird zu letzten Soll-Zeit die 
Periodenlänge addiert um die den nächsten Ausführungszeitpunkt zu berchnen. 
Wenn die Verbindung aber längere Zeit unterbrochen war sammeln sich dadurch 
viele Ausführungen an die dann mit einer kürzeren Periodenlänge 
(tv_earliest_next_trigger_time) abgearbeitet werden. Wenn die Zykluszeiten eh 
schon knapp ausgelegt sind, kann das zu einer Überlastung führen. Das muss 
geändert werden. Die nächste Ausführungszeit muss die akutelle Zeit plus 
Zykluszeit sein.




ErrorCodes im Prozessabbild
===============================================================================

ModbusTCP 
-------------------------------------------------------------------------------
Modbus_Master_Status
16	0x10    Initialisierung ist fehlgeschlagen
17	0x11    Das Gerät ist nicht erreichbar (falsche IP, Kabel?)

Modbus_Action_Status
1       ILLEGAL FUNCTION        Der verwendete Functioncode ist nicht erlaubt. Prüfe, ob Du den richtigen Functioncode verwendest.
2       ILLEGAL DATA ADDRESS    Die verwendete Adresse ist nicht gültig. Das Register ist entweder schreibgeschützt oder existiert nicht. Überprüfe die Adresse.
3       ILLEGAL DATA VALUE      Mindestens ein Teil der verwendeten Datenwerte ist ungültig. Es wäre z. B. möglich, dass Du eine zu hohe Registeranzahl angegeben hast. Prüfe Deine Werte.
13      INVALID DATA    	Der Slave hat mit meinen unvollständigen Paket geantwortet. Das kann z.B. auftreten nachdem die Verbindung unterbrochen wurde. Prüfe deine Verkabelung.
110     CONNECTION TIMED OUT    Der Slave hat nicht schnell genug oder garnicht geantwortet. Prüfe Deine Konfiguration und Verkabelung.



ModbusRTU
-------------------------------------------------------------------------------
Modbus_Master_Status
16	0x10    Initialisierung ist fehlgeschlagen
17	0x11    Serielle Verbindung kann nicht geöffnet werden (device name falsch, unzulässige Konfiguration)

Modbus_Action_Status
1       ILLEGAL FUNCTION        Der verwendete Functioncode ist nicht erlaubt. Prüfe, ob Du den richtigen Functioncode verwendest.
2       ILLEGAL DATA ADDRESS    Die verwendete Adresse ist nicht gültig. Das Register ist entweder schreibgeschützt oder existiert nicht. Überprüfe die Adresse.
3       ILLEGAL DATA VALUE      Mindestens ein Teil der verwendeten Datenwerte ist ungültig. Es wäre z. B. möglich, dass Du eine zu hohe Registeranzahl angegeben hast. Prüfe Deine Werte.
11	Resource temporarily unavailable
12	Invalid CRC		Vom Slave wurde ein gestörtes Paket empfangen. Das kann z.B. auftreten nachdem die Verbindung unterbrochen wurde. Prüfe deine Verkabelung.
13      INVALID DATA    	Vom Slave wurde ein unvollständiges Paket empfangen. Das kann z.B. auftreten nachdem die Verbindung unterbrochen wurde. Prüfe deine Verkabelung.
110     CONNECTION TIMED OUT    Der Slave hat nicht schnell genug oder garnicht geantwortet. Prüfe Deine Konfiguration und Verkabelung.
104	Connection reset by peer



alle internen Fehlermeldungen, die aber nicht auftreten und auch nicht dokumentiert werden brauchen
-------------------------------------------------------------------------------
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

Beispielkonfiguration für die config.rsc


{
"config": {
                "modbusActions": [
                {
                        "SlaveAddress" : 1,					Adresse des Modbus Slaves, der durch den Befehl angesprochen wird
                        "FunctionCode" : 2,					Modbus Functionscode s.u.
                        "RegisterAddress" : 1,				Modbus Registeradresse(Startadresse) des Slaves
                        "QuantityOfRegisters" : 8,			Anzahl der Modbus Register, die geschrieben/gelsen werden 
                        "ActionInterval" : 100000,			Zeitintervall indem der Befehl vom Master gesendet wird in µsec. Für den Benutzer reicht hier ggf. Angabe in 1ms Schritten
                        "ProcessImageStartByte" : 2,		Der Offset im Pi Prozess Abbild für die Modbusdaten
                        "Action ID" : 1						Aufsteigende ID für den Befehl zur Information im Fehlerfall
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


____________________________________________________________________________________________________________________________________________

ModbusFunktionscodes:
Wichtige Funktionscodes sind mit '*' gekennzeichnet. Die anderen Funktionscodes müssen ggf. nicht unterstützt werden

*	eREAD_COILS					= 0x01,
*	eREAD_DISCRETE_INPUTS		= 0x02,
*	eREAD_HOLDING_REGISTERS		= 0x03,
*	eREAD_INPUT_REGISTERS		= 0x04,
*	eWRITE_SINGLE_COIL			= 0x05,
*	eWRITE_SINGLE_REGISTER		= 0x06,
	eREAD_EXCEPTION_STATUS		= 0x07,	//serial line only, not supported by libmodbus v3.0.6
*	eWRITE_MULTIPLE_COILS		= 0x0F,
*	eWRITE_MULTIPLE_REGISTERS	= 0x10,
	eREPORT_SLAVE_ID			= 0x11,
	eWRITE_MASK_REGISTER		= 0x16,
	eWRITE_AND_READ_REGISTERS	= 0x17,
	
	Coils und Discrete Inputs haben eine größe von einem Bit. Im Prozessabbild sollte für jedes Bit aber ein Byte reserviert werden.
	Holding Registers und Input Registers haben eine größe von 2 Byte.
	
____________________________________________________________________________________________________________________________________________

Für jeden Modbusbefehl muss zusätzlich ein Byte im Pi Prozessabbild für Fehlermeldungen reserviert werden.
Für das virtuelle Gerät muss zusätzlich ein Byte im Pi Prozessabbild für Fehlermeldungen reserviert werden.

Für jeden Modbusbefehl und das virtuelle Gerät muss jeweils ein bit zum zurücksetzen des Fehlerbytes im Pi Prozessabbild reserviert werden
	