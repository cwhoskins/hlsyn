/*
 * circuit.c
 *
 *  Created on: Oct 20, 2020
 *      Author: cwhos
 */

#include "circuit.h"
#include "net.h"
#include "component.h"
#include "logger.h"
#include "resource.h"
#include "file_writer.h"
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

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

void Circuit_TestPrint(circuit* self);
void Circuit_PrintDistributionGraph(circuit* self);

circuit* Circuit_Create(uint8_t latency) {
	const uint8_t max_inputs = 64;
	const uint8_t max_nets = 128;
	uint8_t idx;
	circuit* new_circuit = (circuit*) malloc(sizeof(circuit));
	if(NULL != new_circuit) {
		new_circuit->num_nets = 0;
		new_circuit->num_inputs = 0;
		new_circuit->num_outputs = 0;
		new_circuit->num_components = 0;
		new_circuit->critical_path_ns = 0.0f;
		new_circuit->latency = latency;
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
	uint8_t cur_usage = 0;
	char node_name[64];
	if(NULL != self && NULL != name) {
		while(net_idx < self->num_nets) {
			Net_GetName(self->netlist[net_idx], node_name);
			if(0 == strcmp(node_name, name)) {
				if(Net_GetUsage(self->netlist[net_idx]) > cur_usage) {
					return_net = self->netlist[net_idx];
					cur_usage = Net_GetUsage(self->netlist[net_idx]);
				}
			}
			net_idx++;
		}
	}
	return return_net;
}

net* Circuit_FindNet_Usage(circuit* self, char* name, uint8_t usage) {
	uint8_t net_idx = 0;
	net* return_net = NULL;
	char node_name[64];
	if(NULL != self && NULL != name && usage > 0) {
		while(net_idx < self->num_nets) {
			Net_GetName(self->netlist[net_idx], node_name);
			if(0 == strcmp(node_name, name)) {
				if(Net_GetUsage(self->netlist[net_idx]) == usage) {
					return_net = self->netlist[net_idx];
					break;
				}
			}
			net_idx++;
		}
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

	for(idx = 0;idx < self->num_inputs; idx++) {
		Net_SchedulePathASAP(self->input_nets[idx], 1);
	}
}

uint8_t Circuit_ScheduleALAP(circuit* self) {
	uint8_t idx;
	uint8_t ret_value = SUCCESS;
	net* cur_output = NULL;
	for(idx = 0;idx < self->num_outputs; idx++) {
		cur_output = self->output_nets[idx];
		ret_value = Net_SchedulePathALAP(cur_output, (self->latency+1));
		if(FAILURE == ret_value) {
			break;
		}
	}
	if(FAILURE != ret_value) {
		for(idx = 0;idx < self->num_nets; idx++) {
			cur_output = self->netlist[idx];
			if(0 == Net_GetTimeFrameEnd(cur_output)) {//Wasn't updated since net does not map to output
				ret_value = Net_SchedulePathALAP(cur_output, (self->latency+1));
				if(FAILURE == ret_value) {
					break;
				}
			}
		}
	}
	return ret_value;
}

void Circuit_ScheduleForceDirected(circuit* self, state_machine* sm) {

	uint8_t s_idx, cycle_idx, comp_idx, min_cycle;
	component* min_component;
	uint8_t first_component = 0;
	float min_force, self_force, suc_force, pred_force, total_force;
	uint8_t cycle_start, cycle_end;
	char log_msg[128], scheduled_net_name[8];
	net* scheduled_net;

	if(NULL != self && NULL != sm) {
		Circuit_ScheduleASAP(self);
		Circuit_ScheduleALAP(self);
		Circuit_TestPrint(self);
		Circuit_CalculateDistributionGraphs(self);
		Circuit_PrintDistributionGraph(self);
		for(s_idx = 0; s_idx < self->num_components; s_idx++) { //Cycle through every operation so that all get scheduled
			Circuit_CalculateDistributionGraphs(self);
			for(comp_idx = 0; comp_idx < self->num_components; comp_idx++) {
				if(FALSE == Component_GetIsScheduled(self->component_list[comp_idx])) { //Skip component if it's already been scheduled

					cycle_start = Component_GetTimeFrameStart(self->component_list[comp_idx]);
					cycle_end = Component_GetTimeFrameEnd(self->component_list[comp_idx]);
					for(cycle_idx = cycle_start; cycle_idx <= cycle_end; cycle_idx++) {

						self_force = Component_CalculateSelfForce(self->component_list[comp_idx], self, cycle_idx);
						suc_force = Component_CalculateSuccessorForce(self->component_list[comp_idx], self, cycle_idx);
						pred_force = Component_CalculatePredecessorForce(self->component_list[comp_idx], self, cycle_idx);
						total_force = self_force + suc_force + pred_force;
						if(0 == first_component || total_force < min_force) {
							min_force = total_force;
							min_component = self->component_list[comp_idx];
							min_cycle = cycle_idx;
							first_component = 1;
						}
					}
				}
			}
			port output_port = Component_GetOutputPort(min_component, 0);
			scheduled_net = output_port.port_net;
			Net_GetName(scheduled_net, scheduled_net_name);
			if(FALSE == Component_GetIsScheduled(min_component)) {
				sprintf(log_msg, "MSG(Circuit_ScheduleForceDirected): %s scheduled to cycle %d with force %.2f\n", scheduled_net_name, min_cycle, min_force);
				LogMessage(log_msg, MESSAGE_LEVEL);
				StateMachine_ScheduleOperation(sm, min_component, min_cycle);
			} else {
				break;
			}
			first_component = 0;
		}
	} else {
		LogMessage("ERROR(Circuit_ScheduleForceDirected): Invalid input pointers\n", ERROR_LEVEL);
	}
}

void Circuit_CalculateDistributionGraphs(circuit* self) {
	uint8_t rsrc_idx, comp_idx, cycle_idx;
	component* cur_comp = NULL;
	float probability;
	uint8_t cycle_start, cycle_end;
	for(rsrc_idx = 0; rsrc_idx < resource_none;rsrc_idx++) {
		for(cycle_idx=0;cycle_idx<self->latency;cycle_idx++) { //Zero out dg before calculating
			self->distribution_graphs[rsrc_idx][cycle_idx] = 0;
		}
		for(comp_idx=0;comp_idx < self->num_components;comp_idx++) {
			cur_comp = self->component_list[comp_idx];
			if(rsrc_idx == Component_GetResourceType(cur_comp)) {
				cycle_start = Component_GetTimeFrameStart(self->component_list[comp_idx]);
				cycle_end = Component_GetTimeFrameEnd(self->component_list[comp_idx]);
				for(cycle_idx=cycle_start;cycle_idx<=cycle_end;cycle_idx++) {
					probability = Component_GetProbability(cur_comp, (cycle_idx+1));
					self->distribution_graphs[rsrc_idx][cycle_idx] += probability;
				}
			}
		}
	}
}

float Circuit_GetDistributionGraph(circuit* self, resource_type type, uint8_t cycle) {
	float ret_value = 0.0f;
	uint8_t cycle_idx = cycle-1;
	if(NULL != self && cycle > 0 && type < resource_none) {
		if(cycle <= self->latency) {
			ret_value = self->distribution_graphs[type][cycle_idx];
		}
	} else {
		LogMessage("Error(Circuit_GetDistributionGraph): Invalid Input", ERROR_LEVEL);
	}
	return ret_value;
}

void Circuit_Destroy(circuit** self) {
	uint8_t idx = 0;
	if(NULL != (*self)) {
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

void Circuit_TestPrint(circuit* self) {
	uint8_t idx;
	char line_buffer[512];
	char type_declaration[8];
	FILE* fp;
	uint8_t asap, alap;
	if(NULL != self) {
		fp = fopen("./test/time_frame.txt", "w+");
		if(NULL == fp) {
			LogMessage("Error: Cannot open output file\n", ERROR_LEVEL);
			return;
		}
		for(idx=0;idx<self->num_components;idx++) {
			DeclareComponent(self->component_list[idx], line_buffer, idx);
			asap = Component_GetTimeFrameStart(self->component_list[idx]);
			alap = Component_GetTimeFrameEnd(self->component_list[idx]);
			fprintf(fp, "%s\tASAP: %d\n\tALAP: %d\n\n", line_buffer, asap, alap);
		}
		fclose(fp);
	}
}

void Circuit_PrintDistributionGraph(circuit* self) {
	uint8_t idx, r_idx;
	char line_buffer[512], cell_buffer[32];
	FILE* fp;
	if(NULL != self) {
		fp = fopen("./test/distribution_graph.csv", "w+");
		if(NULL == fp) {
			LogMessage("Error: Cannot open output file\n", ERROR_LEVEL);
			return;
		}
		strcpy(line_buffer, "Cycle");
		for(idx=0;idx<self->latency;idx++) {
			sprintf(cell_buffer, ",%d", (idx+1));
			strcat(line_buffer, cell_buffer);
		}
		strcat(line_buffer, "\n");
		fprintf(fp, line_buffer);

		for(r_idx=resource_multiplier;r_idx<resource_none;r_idx++) {
			switch(r_idx) {
			case resource_multiplier:
				sprintf(line_buffer, "multiplier");
				break;
			case resource_divider:
				sprintf(line_buffer, "divider");
				break;
			case resource_logical:
				sprintf(line_buffer, "logical");
				break;
			case resource_alu:
				sprintf(line_buffer, "ALU");
					break;
			default:
				sprintf(line_buffer, "none");
				break;
			}
			for(idx=0;idx<self->latency;idx++) {
				sprintf(cell_buffer, ",%.2f", self->distribution_graphs[r_idx][idx]);
				strcat(line_buffer, cell_buffer);
			}
			strcat(line_buffer, "\n");
			fprintf(fp, line_buffer);
		}
		fclose(fp);
	}
}
