/* SPDX-License-Identifier: GPL-2.0-or-later */

/*!
 *
 * Project: piModbusSlave
 * (C)    : KUNBUS GmbH, Heerweg 15C, 73370 Denkendorf, Germany
 *
 */
#ifndef PI_PROCESS_IMAGE_ACCESS_H_
#define PI_PROCESS_IMAGE_ACCESS_H_

#include "modbusconfig.h"

int32_t writeModbusDataToProcessImage(modbus_mapping_t* mbMapping, TProcessImageConfiguration* ptrSPiProcessImageOffsets);
int32_t readModbusDataFromProcessImage(modbus_mapping_t* mbMapping, TProcessImageConfiguration* ptrSPiProcessImageOffsets);


#endif /* PI_PROCESS_IMAGE_ACCESS_H_ */
