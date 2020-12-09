/*
 * state_machine.c
 *
 *  Created on: Nov 28, 2020
 *      Author: cwhos
 */
#include <stdlib.h>
#include <stdio.h>
#include "state_machine.h"
#include "state.h"
#include "logger.h"


typedef struct struct_state_machine {
	uint8_t latency;
	state* head;
	state** state_list;
	uint8_t num_states;
	uint8_t total_states;
} state_machine;

void StateMachine_TestPrint(state_machine* self);
void StateMachine_TestPrintCycle(state_machine* self);


state_machine* StateMachine_Create(uint8_t latency) {
	uint8_t idx;
	state_machine* new_sm = (state_machine*) malloc(sizeof(state_machine));
	if(NULL != new_sm) {
		new_sm->state_list = (state**) malloc(latency * sizeof(state*));
		for(idx = 0; idx < latency; idx++) {
			new_sm->state_list[idx] = State_Create(idx+1);
		}
		new_sm->total_states = 0;
		new_sm->num_states = 0;
		new_sm->head = State_Create(0);
		new_sm->latency = latency;
		if(NULL == new_sm->head || NULL == new_sm->state_list) {
			StateMachine_Destroy(&new_sm);
		}
	}
	return new_sm;
}

void StateMachine_ScheduleOperation(state_machine* self, component* op, uint8_t cycle) {
	state* schedule_state = NULL;
	if(NULL != self && NULL != op && cycle > 0) {
		if(self->latency >= cycle) {
			schedule_state = self->state_list[cycle-1];
			//Add component to state
			State_AddOperation(schedule_state, op);
		}
	}
}

void StateMachine_Link(state_machine* self) {
	condition initial_condition = {.type = transition_all, .net_condition = NULL};
	if(NULL != self) {
		StateMachine_TestPrintCycle(self);
		self->total_states = State_LinkState(self->head, self, 0, initial_condition, 0);
		StateMachine_TestPrint(self);
	}
}

uint8_t StateMachine_ConditionEnd(state_machine* self, condition cond) {
	uint8_t c_idx;
	uint8_t conditional_used = FALSE;
	state* cur_cycle;
	component* cur_comp;
	uint8_t end_cycle = 0;
	if(NULL != self && cond.type != transition_all) {
		uint8_t idx = self->latency;
		end_cycle = self->latency+1; //Has to end at the Done state
		while(idx > 0) {
			idx--;
			cur_cycle = self->state_list[idx];
			conditional_used = FALSE;
			for(c_idx=0;c_idx < State_GetNumOperations(cur_cycle);c_idx++) {
				cur_comp = State_GetOperation(cur_cycle, c_idx);
				condition comp_cond = Component_GetCondition(cur_comp);
				if(comp_cond.net_condition == cond.net_condition) { //If the cycle contains a condition with the same net then it is the last cycle to have the condition
					end_cycle = idx+2;
					conditional_used = TRUE;
					break;
				}
			}
			if(TRUE == conditional_used) {
				break;
			}
		}
	}
	return end_cycle;
}

state* StateMachine_Search(state_machine* self, uint8_t cycle) {
	state* ret = NULL;
	if(NULL != self) {
		if(cycle > 0 && (cycle < (self->latency+1)))
			ret = State_Search(self->head, cycle);
		else if(0 == cycle) {
			ret = self->head;
		}
	}
	return ret;
}

state* StateMachine_GetCycle(state_machine* self, uint8_t cycle) {
	state* ret = NULL;
	if(NULL != self) {
		if(self->latency >= cycle && cycle > 0) {
			ret = self->state_list[cycle-1];
		}
	}
	return ret;
}

uint8_t StateMachine_GetLatency(state_machine* self) {
	uint8_t ret = 0;
	if(NULL != self) {
		ret = self->latency;
	}
	return ret;
}

void StateMachine_AddState(state_machine* self, state* new_state) {
	uint8_t cur_cycle, next_cycle, new_cycle;
	state* cur_state;
	state* next_state;
	if(NULL != self && NULL != new_state) {
		//Add it to the list
		self->state_list[self->num_states] = new_state;
		self->num_states++;
		new_cycle = State_GetCycle(new_state);
		cur_state = self->head;
		while(NULL != cur_state) {
			next_state = State_GetNextState(cur_state, 0);
			cur_cycle = State_GetCycle(cur_state);
			next_cycle = State_GetCycle(next_state);
			if(new_cycle > cur_cycle && new_cycle <= next_cycle) {
				State_AddNextState(cur_state, new_state);
				break;
			} else {
				cur_state = next_state;
			}
		}
	}
}

state* StateMachine_FindState(state_machine* self, void* conditional, uint8_t cycle) {
	state* cur_state;
	if(NULL != self) {
		cur_state = self->head;
		while(NULL != cur_state) {
			if(cycle == State_GetCycle(cur_state)) {
				return cur_state;
			} else {
				//cur_state = State_GetNextState(cur_state);
			}
		}
	}
	return NULL;
}

uint8_t StateMachine_GetNumStates(state_machine* self) {
	uint8_t ret = 0;
	if(NULL != self) {
		ret = self->total_states;
	}
	return ret;
}

void StateMachine_Destroy(state_machine** self) {
	if(NULL != (*self)) {
		State_Destroy(&(*self)->head);
		while((*self)->num_states > 0) {
			(*self)->num_states--;
			State_Destroy(&((*self)->state_list[(*self)->num_states]));
		}
		free((*self)->state_list);
		free(*self);
		*self = NULL;
	}
}

void StateMachine_TestPrint(state_machine* self) {
	FILE* fp;
	if(NULL != self) {
		fp = fopen("./test/StateMachine.txt", "w+");
		if(NULL == fp) {
			LogMessage("Error: Cannot open output file\n", ERROR_LEVEL);
			return;
		}
		State_TestPrint(self->head, fp);
		fclose(fp);
	}
}

void StateMachine_TestPrintCycle(state_machine* self) {
	FILE* fp;
	uint8_t idx;
	if(NULL != self) {
		fp = fopen("./test/StateMachineCycle.txt", "w+");
		if(NULL == fp) {
			LogMessage("Error: Cannot open output file\n", ERROR_LEVEL);
			return;
		}
		for(idx=0;idx<self->latency;idx++) {
			fprintf(fp, "Cycle %d\n", idx+1);
			State_TestPrintOperations(self->state_list[idx], fp);
		}

		fclose(fp);
	}
}
