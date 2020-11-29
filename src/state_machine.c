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
	state** state_list;
	uint8_t num_states;
} state_machine;


state_machine* StateMachine_Create(uint8_t latency) {
	uint8_t max_state = 4 * latency;
	state_machine* new_sm = (state_machine*) malloc(sizeof(state_machine));
	if(NULL != new_sm) {
		new_sm->state_list = (state**) malloc(max_state * sizeof(state*));
		new_sm->num_states = 0;
		transition head2tail;
		new_sm->head = State_Create(0);
		new_sm->tail = State_Create(latency + 1);
		if(NULL != new_sm->head && NULL != new_sm->head && NULL != new_sm->state_list) {
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

void StateMachine_ScheduleOperation(state_machine* self, component* op, uint8_t cycle) {
	uint8_t idx;
	state* temp_state = NULL;
	state* schedule_state = NULL;
	net* conditional_net = NULL; //TODO: Retrieve from component
	if(NULL != self && NULL != op && cycle > 0) {
		//First search to see if state currently exists
		/* Pseudocode
		 * Search through states in SM
		 * Check if cycle & condition are the same
		 */
		for(idx = 0; idx < self->num_states;idx++) {
			temp_state = self->state_list[idx];
			if(cycle == State_GetCycle(temp_state)) { //Same Cycle
				if(Component_GetConditional(op) == State_GetConditional(temp_state)) { //Same condition
					schedule_state = temp_state;
					break;
				}
			}
		}
		if(NULL == schedule_state) { //State could not be found
			//Create State
			schedule_state = State_Create(cycle);
			//Add new state to SM
			StateMachine_AddState(self, schedule_state);

			/* Pseudocode
			 * Create State
			 * Check if component is under a condition
			 * If so
			 * 		Create transition and apply condition
			 * else
			 * 		Create transition and apply no condition
			 * Tie new state to previous state
			 */
		}
		//Add component to state
		State_AddOperation(schedule_state, op);
	}
}

void StateMachine_AddState(state_machine* self, state* new_state) {
	if(NULL != self && NULL != new_state) {
		//Add it to the list
		self->state_list[self->num_states] = new_state;
		self->num_states++;
	}
}

state* StateMachine_FindState(state_machine* self, void* conditional, uint8_t cycle) {
	uint8_t idx;
	state* temp_state;
	state* ret_value = NULL;
	if(NULL != self && NULL != conditional) {
		for(idx = 0; idx < self->num_states;idx++) {
			temp_state = self->state_list[idx];
			if(cycle == State_GetCycle(temp_state)) { //Same Cycle
				if(conditional == State_GetConditional(temp_state)) { //Same condition
					ret_value = temp_state;
					break;
				}
			}
		}
	}
	return ret_value;
}

void StateMachine_Destroy(state_machine** self) {

}

