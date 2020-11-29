/*
 * state_machine.c
 *
 *  Created on: Nov 28, 2020
 *      Author: cwhos
 */
#include <stdlib.h>
#include "state_machine.h"
#include "state.h"


typedef struct struct_state_machine {
	uint8_t latency;
	state* head;
	state* tail;
} state_machine;


state_machine* StateMachine_Create(uint8_t latency) {
	state_machine* new_sm = (state_machine*) malloc(sizeof(state_machine));
	if(NULL != new_sm) {
		transition head2tail;
		new_sm->head = State_Create(0);
		new_sm->tail = State_Create(latency + 1);
		if(NULL != new_sm->head && NULL != new_sm->head) {
			head2tail.type = transition_all;
			head2tail.condition = NULL;
			head2tail.next_state = new_sm->tail;
			State_AddNextState(new_sm->head, head2tail);
		} else {
			StateMachine_Destroy(&new_sm);
		}
	}
	return new_sm;
}

void StateMachine_InsertState(state_machine* self, state* new_state, transition state_transition) {
	uint8_t cycle = State_GetCycle(new_state);
	uint8_t idx;
	state* cur_state, next_state;
	if(NULL != self && NULL != new_state && transition_error != state_transition.type) {
		cur_state = self->head;
	}
}

void StateMachine_Destroy(state_machine** self) {

}

