/*
 * circuit.c
 *
 *  Created on: Oct 20, 2020
 *      Author: cwhos
 */

#include "circuit.h"
#include "net.h"
#include "component.h"
#include <string.h>
#include <stddef.h>
#include <stdlib.h>

typedef struct struct_circuit {
	net** input_nets;
	net** output_nets;
	net** netlist;
	component** component_list;

	uint8_t num_nets;
	uint8_t num_inputs;
	uint8_t num_outputs;
	uint8_t num_components;

	float critical_path_ns;
	float* distribution_graphs[4];
	uint8_t latency;
} circuit;

circuit* Circuit_Create() {
	const uint8_t max_inputs = 32;
	const uint8_t max_nets = 32;
	uint8_t idx;
	circuit* new_circuit = (circuit*) malloc(sizeof(circuit));
	if(NULL != new_circuit) {
		new_circuit->num_nets = 0;
		new_circuit->num_inputs = 0;
		new_circuit->num_outputs = 0;
		new_circuit->critical_path_ns = 0.0f;
		new_circuit->input_nets = (net**) malloc(max_inputs * sizeof(net*));
		new_circuit->netlist = (net**) malloc(max_nets * sizeof(net*));
		new_circuit->output_nets = (net**) malloc(max_nets * sizeof(net*));
		new_circuit->component_list = (component**) malloc(max_nets * sizeof(component*));
		for(idx = 0; idx < 4; idx++) {
			new_circuit->distribution_graphs[idx] = (float*) malloc(new_circuit->latency * sizeof(float));
			if(NULL == new_circuit->distribution_graphs[idx]) {
				Circuit_Destroy(&new_circuit);
			}
		}

	}
	if(NULL == new_circuit->input_nets || NULL == new_circuit->netlist || NULL == new_circuit->output_nets || NULL == new_circuit->component_list) {
		Circuit_Destroy(&new_circuit);
	}
	return new_circuit;
}

net* Circuit_FindNet(circuit* self, char* name) {
	uint8_t net_idx = 0;
	net* return_net = NULL;
	char node_name[64];
	while(net_idx < self->num_nets) {
		Net_GetName(self->netlist[net_idx], node_name);
		if(0 == strcmp(node_name, name)) {
			return_net = self->netlist[net_idx];
			break;
		}
		net_idx++;
	}
	return return_net;
}

void Circuit_AddNet(circuit* self, net* new_net) {
	if(NULL != new_net && NULL != self) {
		self->netlist[self->num_nets] = new_net;
		if(net_output == Net_GetType(new_net)) {
			self->output_nets[self->num_outputs] = new_net;
			self->num_outputs++;
		} else if(net_input == Net_GetType(new_net)) {
			self->input_nets[self->num_inputs] = new_net;
			self->num_inputs++;
		}
		self->num_nets++;
	}
	return;
}

net* Circuit_GetNet(circuit* self, uint8_t idx) {
	net* ret_value = NULL;
	if(NULL != self) {
		if(idx < self->num_nets) ret_value = self->netlist[idx];
	}
	return ret_value;
}

uint8_t Circuit_GetNumNet(circuit* self) {
	uint8_t ret_value = 0;
	if(NULL != self) {
		ret_value = self->num_nets;
	}
	return ret_value;
}

void Circuit_AddComponent(circuit* self, component* new_component) {
	if(NULL != self && NULL != new_component) {
		self->component_list[self->num_components] = new_component;
		self->num_components++;
	}
}

component* Circuit_GetComponent(circuit* self, uint8_t idx) {
	component* ret_value = NULL;
	if(NULL != self) {
		if(idx < self->num_components) ret_value = self->component_list[idx];
	}
	return ret_value;
}

uint8_t Circuit_GetNumComponent(circuit* self) {
	uint8_t ret_value = 0;
	if(NULL != self) {
		ret_value = self->num_components;
	}
	return ret_value;
}

void Circuit_CalculateDelay(circuit* self) {
	uint8_t idx;
	const float input_delay_ns = 0.0f;
	float max_delay = 0.0f;
	//Reset netlist for new scheduling
	for(idx = 0; idx < self->num_nets;idx++) {
		Net_ResetDelay(self->netlist[idx]);
	}

	for(idx = 0;idx < self->num_inputs; idx++) {
		Net_UpdatePathDelay(self->input_nets[idx], input_delay_ns);
	}

	//Find critical path value
	for(idx = 0; idx < self->num_nets;idx++) {
		if(max_delay < Net_GetDelay(self->netlist[idx])) {
			max_delay = Net_GetDelay(self->netlist[idx]);
		}
	}
	self->critical_path_ns = max_delay;

}

float Circuit_GetCriticalPath(circuit* self) {
	float critical_path_ns = -1.0f;
	if(NULL != self) {
		critical_path_ns = self->critical_path_ns;
	}
	return critical_path_ns;
}

void PrintCircuit(circuit* self) {
	uint8_t net_idx;
	for(net_idx = 0; net_idx < self->num_nets; net_idx++) {
		PrintNet(self->netlist[net_idx]);
	}
}

void Circuit_ScheduleASAP(circuit* self) {
	uint8_t idx;

	//Reset netlist for new scheduling
	for(idx = 0; idx < self->num_nets;idx++) {
		Net_ResetDelay(self->netlist[idx]);
	}

	for(idx = 0;idx < self->num_inputs; idx++) {
		Net_SchedulePathASAP(self->input_nets[idx], 1);
	}
}

void Circuit_ScheduleALAP(circuit* self) {

}

void Circuit_ScheduleForceDirected(circuit* self) {

}

void Circuit_CalculateDistributionGraphs(circuit* self) {
	uint8_t rsrc_idx, comp_idx, cycle_idx;
	component* cur_comp = NULL;
	for(rsrc_idx = 0; rsrc_idx < resource_error;rsrc_idx++) {
		for(comp_idx=0;comp_idx < self->num_components;comp_idx++) {
			cur_comp = self->component_list[comp_idx];
			if(rsrc_idx == Component_GetResourceType(cur_comp)) {
				for(cycle_idx=0;cycle_idx<self->latency;cycle_idx++) {
					self->distribution_graphs[rsrc_idx][cycle_idx] += Component_GetProbability(cur_comp, cycle_idx);
				}
			}
		}
	}
}

float Circuit_GetDistributionGraph(circuit* self, resource_type type, uint8_t cycle) {
	return 0.0f;
}

void Circuit_Destroy(circuit** self) {
	uint8_t idx = 0;
	if(NULL != *self) {
		while(idx < (*self)->num_nets) {
			Net_Destroy(&((*self)->netlist[idx]));
			idx++;
		}
		while((*self)->num_components > 0 ){
			(*self)->num_components--;
			Component_Destroy(&((*self)->component_list[(*self)->num_components]));
		}
		for(idx = 0; idx < 4; idx++) {
			free((*self)->distribution_graphs[idx]);
		}
		free((*self)->output_nets);
		free((*self)->input_nets);
		free((*self)->netlist);
		free((*self)->component_list);
		free((*self));
		*self = NULL;
	}
}
