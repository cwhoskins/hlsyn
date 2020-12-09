/*
 * state.c
 *
 *  Created on: Nov 28, 2020
 *      Author: cwhos
 */

#include <stdlib.h>
#include <stdio.h>
#include "state.h"
#include "state_machine.h"
#include "logger.h"
#include "component.h"
#include "file_writer.h"

typedef struct struct_state {
	uint8_t state_number;
	uint8_t cycle;
	state* next_state[2];
	uint8_t num_states;
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
		new_state->state_number = 255;
		new_state->num_operations = 0;
		new_state->num_states = 0;
		new_state->next_state[0] = NULL;
		new_state->next_state[1] = NULL;
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
		if(self->num_states < 2) {
			self->next_state[self->num_states] = next_state;
			self->num_states++;
		} else {
			LogMessage("Error(): Adding too many states\n", ERROR_LEVEL);
		}
	}
}

void State_AddOperation(state* self, component* operation) {
	if(NULL != self && NULL != operation) {
		if(max_ops > self->num_operations) {
			self->operations[self->num_operations] = operation;
			Component_SchedulePathFDS(operation, self->cycle);
			self->num_operations++;
		} else {
			LogMessage("ERROR(State_AddOperation): Hit max operations\n", ERROR_LEVEL);
		}
	}
}

uint8_t State_LinkState(state* self, state_machine* sm, uint8_t cycle, condition cond, uint8_t state_number) {
	uint8_t has_if_else = FALSE;
	uint8_t idx;
	uint8_t cur_state_num = state_number;
	component* cur_comp;
	condition cur_cond;
	condition next_condition = cond;
	state* new_state = NULL;
	if(NULL != self && NULL != sm) {
		if(state_number < self->state_number) self->state_number = state_number;
		cur_state_num++;
		state* cur_cycle = StateMachine_GetCycle(sm, cycle);
		if(NULL != cur_cycle) {
			for(idx=0;idx < State_GetNumOperations(cur_cycle);idx++) {
				cur_comp = State_GetOperation(cur_cycle,idx);
				cur_cond = Component_GetCondition(cur_comp);
				if(transition_all == cur_cond.type || cond.type == cur_cond.type) {
					State_AddOperation(self, cur_comp);
				}
				if(component_if_else == Component_GetType(cur_comp)) {
					has_if_else = TRUE;
					port temp;
					temp = Component_GetInputPort(cur_comp, 0);
					next_condition.net_condition = temp.port_net;
					next_condition.type = transition_if;
				}
			}
			if(transition_all != cond.type) {
				if((cycle+1) == StateMachine_ConditionEnd(sm, cond) || (cycle == StateMachine_GetLatency(sm))) { //Next state should merge conditional or go to done state
					new_state = StateMachine_Search(sm, cycle+1); //Search to check if state already exists
					next_condition.net_condition = NULL; //End condition
					next_condition.type = transition_all;
				}
				if(NULL != new_state) State_AddNextState(self, new_state);
			}
		}
		if(NULL == new_state && self->cycle <= StateMachine_GetLatency(sm)) {
			new_state= State_Create(self->cycle+1);
			State_AddNextState(self, new_state);
			cur_state_num = State_LinkState(new_state, sm, cycle+1, next_condition, cur_state_num);
		}
		if(TRUE == has_if_else) {
			state* else_state = State_Create(self->cycle+1);
			State_AddNextState(self, else_state);
			next_condition.type = transition_else;
			cur_state_num = State_LinkState(else_state, sm, cycle+1, next_condition, cur_state_num);
		}
	}
	return cur_state_num;
}

state* State_Search(state* self, uint8_t cycle) {
	state* ret = NULL;
	uint8_t idx;
	if(NULL != self) {
		if(cycle > self->cycle) {
			for(idx=0;idx<self->num_states;idx++) {
				ret = State_Search(self->next_state[idx], cycle);
				if(NULL != ret) break;
			}
		} else if(cycle == self->cycle) {
			ret = self;
		}
	}
	return ret;
}


uint8_t State_GetCycle(state* self) {
	uint8_t ret_value = 0;
	if(NULL != self) {
		ret_value = self->cycle;
	}
	return ret_value;
}

uint8_t State_GetNumStates(state* self) {
	uint8_t ret_value = 0;
	if(NULL != self) {
		ret_value = self->num_states;
	}
	return ret_value;
}

state* State_GetNextState(state* self, uint8_t idx) {
	state* ret_value = NULL;
	if(NULL != self && idx < 2) {
		ret_value = self->next_state[idx];
	}
	return ret_value;
}

void State_Destroy(state** self) {
	if(NULL != (*self)) {
		while((*self)->num_states > 0) {
			(*self)->num_states--;
			//State_Destroy(&((*self)->next_state[(*self)->num_states]));
		}
		free((*self)->operations);
		free((*self));
		(*self) = NULL;
	}
}

uint8_t State_GetNumOperations(state* self) {
	uint8_t ret_value = 0;
	if(NULL != self) {
		ret_value = self->num_operations;
	}
	return ret_value;
}

component* State_GetOperation(state* self, uint8_t idx) {
	component* ret_value;
	if(NULL != self) {
		ret_value = self->operations[idx];
	}

	return ret_value;
}

uint8_t State_GetStateNumber(state* self) {
	uint8_t ret = 0;
	if(NULL != self) {
		ret = self->state_number;
	}
	return ret;
}

void State_TestPrint(state* self, FILE* output_file) {
	uint8_t idx;
	char line_buffer[128];
	if(NULL != self && NULL != output_file) {
		fprintf(output_file, "\nCycle #%d\tState #%d\n", self->cycle, self->state_number);
		for(idx=0;idx<self->num_operations;idx++) {
			DeclareComponent(self->operations[idx], line_buffer, idx);
			fprintf(output_file, "\t%s\n", line_buffer);
		}
		for(idx=0;idx<self->num_states;idx++) {
			State_TestPrint(self->next_state[idx], output_file);
		}
	}
}

void State_TestPrintOperations(state* self, FILE* output_file) {
	uint8_t idx;
	char line_buffer[128];
	if(NULL != self && NULL != output_file) {
		for(idx=0;idx<self->num_operations;idx++) {
			DeclareComponent(self->operations[idx], line_buffer, idx);
			fprintf(output_file, "\t%s\n", line_buffer);
		}
	}
}
