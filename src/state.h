/*
 * state.h
 *
 *  Created on: Nov 28, 2020
 *      Author: cwhos
 */

#ifndef STATE_H_
#define STATE_H_

#include "global.h"
#include "component.h"

state* State_Create(uint8_t cycle);
void State_AddNextState(state* self, transition next_state);
void State_AddOperation(state* self, component* operation);
uint8_t State_GetCycle(state* self);
transition State_GetTransition(state* self);
void State_Destroy(state** self);

#endif /* STATE_H_ */
