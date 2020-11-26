/*
 * resource.h
 *
 *  Created on: Nov 22, 2020
 *      Author: cwhos
 */

#ifndef RESOURCE_H_
#define RESOURCE_H_

#include "global.h"

resource* Resource_Create(resource_type type, uint8_t num_cycles);
uint8_t Resource_CheckAvailability(resource* self, uint8_t start_cycle);
resource_type Resource_GetType(resource* self);
void Resource_ScheduleOperation(resource* self, component* operation, uint8_t start_cycle);
void Resource_Destroy(resource** self);

#endif /* RESOURCE_H_ */
