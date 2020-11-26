/*
 * resource.h
 *
 *  Created on: Nov 22, 2020
 *      Author: cwhos
 */

#ifndef RESOURCE_H_
#define RESOURCE_H_

resource* Resource_Create(resource_type type);
uint8_t Resource_CheckAvailability(resource* self, uint8_t start_cycle);
void Resource_ScheduleOperation(resource* self, component* operation, uint8_t start_cycle);

#endif /* RESOURCE_H_ */
