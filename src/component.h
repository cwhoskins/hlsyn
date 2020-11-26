/*
 * component.h
 *
 *  Created on: Oct 17, 2020
 *      Author: cwhos
 */

#ifndef COMPONENT_H_
#define COMPONENT_H_

#include <stdint.h>
#include "global.h"



component* Component_Create(component_type type);

void Component_UpdatePathDelay(component* self, float path_delay_ns);
void Component_UpdateDelay(component* self);

void Component_SchedulePathASAP(component* self, uint8_t cycle);
void Component_SchedulePathALAP(component* self, uint8_t cycle);
uint8_t Component_GetTimeFrameEnd(component* self);
uint8_t Component_GetTimeFrameStart(component* self);
float Component_CalculateSelfForce(component* self, circuit* circ, uint8_t cycle);
resource_type Component_GetResourceType(component* self);
float Component_GetProbability(component* self, uint8_t cycle);
float Component_CalculatePredecessorForce(component* self, circuit* circ, uint8_t cycle);
float Component_CalculateSuccessorForce(component* self, circuit* circ, uint8_t cycle);
uint8_t Component_GetDelayCycle(component* self);
uint8_t Component_GetIsScheduled(component* self);
void Component_UpdateTimeFrameStart(component* self, uint8_t cycle);
void Component_UpdateTimeFrameEnd(component* self, uint8_t cycle);

uint8_t Component_AddInputPort(component* self, net* input, port_type type);
uint8_t Component_AddOutputPort(component* self, net* output, port_type type);


float Component_GetDelay(component* self);
component_type Component_GetType(component* self);
uint8_t Component_GetWidth(component* self);
uint8_t Component_GetNumInputs(component* self);
uint8_t Component_GetNumOutputs(component* self);
port Component_GetInputPort(component* self, uint8_t idx);
port Component_GetOutputPort(component* self, uint8_t idx);
net_sign Component_GetSign(component* self);

void Component_Destroy(component** self);

#endif /* COMPONENT_H_ */
