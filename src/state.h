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
#include <stdio.h>

state* State_Create(uint8_t cycle);
void State_AddNextState(state* self, state* next_state);
void State_AddOperation(state* self, component* operation);
state* State_Search(state* self, uint8_t cycle);
uint8_t State_LinkState(state* self, state_machine* sm, uint8_t cycle, condition cond, uint8_t state_number);

uint8_t State_GetCycle(state* self);
state* State_GetNextState(state* self, uint8_t idx);
uint8_t State_GetNumStates(state* self);
void State_Destroy(state** self);
uint8_t State_GetNumOperations(state* self);
uint8_t State_GetStateNumber(state* self);
component* State_GetOperation(state* self, uint8_t idx);
void State_Print(state* self, uint8_t state_number, char* buffer);

void State_TestPrint(state* self, FILE* output_file);
void State_TestPrintOperations(state* self, FILE* output_file);

#endif /* STATE_H_ */
