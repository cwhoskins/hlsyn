/*
 * resource.c
 *
 *  Created on: Nov 22, 2020
 *      Author: cwhos
 */


#include "resource.h"
#include "component.h"
#include <stdlib.h>

typedef struct {
	net* conditional_net;

} state_transition;

typedef struct {
	uint8_t cycle;
	component* comp;
} comp_schedule;

typedef struct {
	uint8_t cycle;
	net* scheduled_net;
} net_schedule;

typedef struct struct_resource {
	uint8_t delay_cycle;
	uint8_t num_cycles;
	uint8_t width; //unsure if necessary
	uint8_t num_operations;
	resource_type type;
	component** component_list;
	uint8_t* scheduling;
	comp_schedule* scheduled_operations;
	net_schedule* scheduled_nets;

} resource;



resource* Resource_Create(resource_type type, uint8_t num_cycles) {
	uint8_t idx;
	uint8_t max_resource = 2 * num_cycles;
	resource* new_resource = (resource*) malloc(sizeof(resource));
	if(NULL != new_resource) {
		new_resource->type = type;
		new_resource->num_cycles = num_cycles;
		new_resource->num_operations = 0;
		switch(type) {
		case resource_multiplier:
			new_resource->delay_cycle = MULTIPLIER_CYCLE_DELAY;
		break;
		case resource_divider:
			new_resource->delay_cycle = DIVIDER_CYCLE_DELAY;
			break;
		case resource_logical:
			new_resource->delay_cycle = LOGICAL_CYCLE_DELAY;
			break;
		case resource_alu:
			new_resource->delay_cycle = ALU_CYCLE_DELAY;
			break;
		default:
			break;
		}
		new_resource->component_list = (component**) malloc(max_resource * sizeof(component*));
		new_resource->scheduled_operations = (comp_schedule*) malloc(max_resource * sizeof(comp_schedule));
		new_resource->scheduled_nets = (net_schedule*) malloc(max_resource * sizeof(net_schedule));
		new_resource->scheduling = (uint8_t*) malloc(max_resource * sizeof(uint8_t));
		for(idx = 0; idx < max_resource; idx++) {
			new_resource->scheduling[idx] = FALSE;
			new_resource->component_list[idx] = NULL;
			new_resource->scheduled_operations[idx].comp = NULL;
			new_resource->scheduled_operations[idx].cycle = 0;
			new_resource->scheduled_nets[idx].scheduled_net = NULL;
			new_resource->scheduled_nets[idx].cycle = 0;
		}
		if(NULL == new_resource->component_list || NULL == new_resource->scheduled_operations || NULL == new_resource->scheduled_nets || NULL == new_resource->scheduling) {
			Resource_Destroy(&new_resource);
		}
	}
	return new_resource;
}

uint8_t Resource_CheckAvailability(resource* self, uint8_t start_cycle) {
	uint8_t is_available = FALSE;
	uint8_t idx;
	uint8_t end_cycle = start_cycle + self->delay_cycle;
	if(NULL != self && start_cycle > 0 && end_cycle <= self->num_cycles) {
		is_available = TRUE;
		for(idx = start_cycle; idx < (start_cycle + self->delay_cycle); idx++) {
			if(TRUE == self->scheduling[idx-1]) {
				is_available = FALSE;
				break;
			}
		}
	}
	return is_available;
}

void Resource_ScheduleOperation(resource* self, component* operation, uint8_t start_cycle) {
	net* assigned_net;
	port temp_port;
	uint8_t idx;
	uint8_t end_cycle = start_cycle + self->delay_cycle;
	if(NULL != self && NULL != operation) {
		if(TRUE == Resource_CheckAvailability(self, start_cycle)) {//Resource can be scheduled
			temp_port = Component_GetOutputPort(operation, 0);
			assigned_net = temp_port.port_net;
			self->scheduled_nets[self->num_operations].scheduled_net = assigned_net;
			self->scheduled_nets[self->num_operations].cycle = end_cycle;
			self->scheduled_operations[self->num_operations].comp = operation;
			self->scheduled_operations[self->num_operations].cycle = start_cycle;
			self->num_operations++;
			for(idx = start_cycle; idx < end_cycle; idx++) {
				self->scheduling[idx] = TRUE;
			}
		}
	}
}

resource_type Resource_GetType(resource* self) {
	resource_type ret_value = resource_error;
	if(NULL != self) {
		ret_value = self->type;
	}
	return ret_value;
}

void Resource_Destroy(resource** self) {
	while((*self)->num_cycles > 0) {
		free((*self)->component_list[((*self)->num_cycles - 1)]);
		(*self)->num_cycles--;
	}
	free((*self)->component_list);
	free((*self)->scheduled_nets);
	free((*self)->scheduled_operations);
	free((*self)->scheduling);
	free((*self));
	(*self) = NULL;
}
