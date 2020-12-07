/*
 * state_machine.h
 *
 *  Created on: Nov 28, 2020
 *      Author: Charles Hoskins
 */

#ifndef STATE_MACHINE_H_
#define STATE_MACHINE_H_

#include "global.h"

state_machine* StateMachine_Create(uint8_t latency);

void StateMachine_AddState(state_machine* self, state* new_state);
void StateMachine_ScheduleOperation(state_machine* self, component* op, uint8_t cycle);
void StateMachine_InsertState(state_machine* self, state* new_state, transition state_transition);
state* StateMachine_FindState(state_machine* self, void* conditional, uint8_t cycle);

void StateMachine_Destroy(state_machine** self);

#endif /* STATE_MACHINE_H_ */
