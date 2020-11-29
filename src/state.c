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
	transition next_state[2];
	component** operations;
	uint8_t num_operations;
} state;

const uint8_t max_ops = 32;

state* State_Create(uint8_t cycle) {

	uint8_t idx;
	state* new_state = (state*) malloc(sizeof(state));
	if(NULL != new_state) {
		new_state->cycle = cycle;
		new_state->num_operations = 0;
		for(idx = 0; idx < 2; idx++) {
			new_state->next_state[idx].condition = NULL;
			new_state->next_state[idx].next_state = NULL;
			new_state->next_state[idx].type = transition_error;
		}
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

void State_AddNextState(state* self, transition next_state) {
	uint8_t transition_idx;
	if(NULL != self) {
		if(transition_if == next_state.type || transition_all == next_state.type) {
			transition_idx = 0;
		} else if(transition_else == next_state.type) {
			transition_idx = 1;
		} else {
			LogMessage("Error(State_AddNextState): Attempted to add unknown transition\n", ERROR_LEVEL);
		}
		self->next_state[transition_idx] = next_state;
	}
}

void State_AddOperation(state* self, component* operation) {
	if(NULL != self && NULL != operation) {
		if(max_ops > self->num_operations ) {
			self->operations[self->num_operations] = operation;
			self->num_operations++;
		} else {
			LogMessage("Error(State_AddOperation): Hit max operations\n", ERROR_LEVEL);
		}
	}
}

void State_Destroy(state** self) {
	if(NULL != (*self)) {
		free((*self)->operations);
		free((*self));
		(*self) = NULL;
	}
}
