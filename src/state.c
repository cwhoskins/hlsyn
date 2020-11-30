/*
 * state.c
 *
 *  Created on: Nov 28, 2020
 *      Author: cwhos
 */

#include <stdlib.h>
#include "state.h"
#include "logger.h"
#include "component.h"

typedef struct struct_state {
	uint8_t cycle;
	state* next_state;
	state* prev_state;
	uint8_t num_prev_states;
	component** operations;
	uint8_t num_operations;
	condition state_condition;
} state;

const uint8_t max_ops = 32;

state* State_Create(uint8_t cycle) {

	uint8_t idx;
	state* new_state = (state*) malloc(sizeof(state));
	if(NULL != new_state) {
		new_state->cycle = cycle;
		new_state->num_operations = 0;
		new_state->num_prev_states = 0;
		new_state->next_state = NULL;
		new_state->prev_state = NULL;
		new_state->operations = (component**) malloc(max_ops * sizeof(component*));
		if(new_state->operations != NULL) {
			for(idx = 0; idx < max_ops; idx++) {
				new_state->operations[idx] = NULL;
			}
		} else {
			State_Destroy(&new_state);
		}
	}
	return new_state;
}

void State_AddNextState(state* self, state* next_state) {
	if(NULL != self && NULL != next_state) {
		self->next_state = next_state;
	}
}

void State_AddPreviousState(state* self, state* prev_state) {
	if(NULL != self && NULL != prev_state) {
		if(self->num_prev_states < 4) {
			self->prev_state = prev_state;
		} else {
			LogMessage("ERROR(State_AddPreviousState): Too many previous states\n", ERROR_LEVEL);
		}
	}
}

void State_AddOperation(state* self, component* operation) {
	if(NULL != self && NULL != operation) {
		if(max_ops > self->num_operations ) {
			self->operations[self->num_operations] = operation;
			self->num_operations++;
		} else {
			LogMessage("ERROR(State_AddOperation): Hit max operations\n", ERROR_LEVEL);
		}
	}
}

uint8_t State_GetCycle(state* self) {
	uint8_t ret_value = 0;
	if(NULL != self) {
		ret_value = self->cycle;
	}
	return ret_value;
}

state* State_GetNextState(state* self) {
	state* ret_value = NULL;
	if(NULL != self) {
		ret_value = self->next_state;
	}
	return ret_value;
}

void State_Destroy(state** self) {
	if(NULL != (*self)) {
		free((*self)->operations);
		free((*self));
		(*self) = NULL;
	}
}
