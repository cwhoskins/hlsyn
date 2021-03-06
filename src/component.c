/*
 * component.c
 *
 *  Created on: Oct 20, 2020
 *      Author: cwhos
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "component.h"
#include "logger.h"
#include "net.h"
#include "circuit.h"

const uint8_t max_dp_inputs = 2;
const uint8_t max_ctrl_inputs = 1;
const uint8_t max_dp_outputs = 1;
const uint8_t max_ctrl_outputs = 3;

typedef struct struct_component {
	component_type type;
	float delay_ns;
	resource_type resource_class;
	uint8_t delay_cycle;
	uint8_t cycle_started_asap;
	uint8_t cycle_started_alap;
	uint8_t time_frame[2];
	uint8_t cycle_scheduled;
	uint8_t is_scheduled;
	net_sign sign;
	uint8_t width;

	port input_ports[8];
	port output_ports[8];
	uint8_t num_inputs;
	uint8_t num_outputs;
	condition conditional;
} component;

component* Component_Create(component_type type) {
	component* new_component = NULL;
	if(component_unknown != type) {
		new_component = (component*) malloc(sizeof(component));
		if(NULL != new_component) {
			if(component_unknown != type) {
				new_component->conditional.type = transition_all;
				new_component->conditional.net_condition = NULL;
				new_component->type = type;
				new_component->width = 0;
				new_component->delay_ns = 0.0f;
				new_component->sign = net_unsigned;
				new_component->num_inputs = 0;
				new_component->num_outputs = 0;
				new_component->is_scheduled = FALSE;
				new_component->cycle_scheduled = 0;
				new_component->cycle_started_alap = 255;
				new_component->cycle_started_asap = 0;
				new_component->time_frame[0] = 0;
				new_component->time_frame[1] = 0;
				if(adder == type || subtractor == type) {
					new_component->resource_class = resource_alu;
					new_component->delay_cycle = ALU_CYCLE_DELAY;
				} else if(divider == type || modulo == type) {
					new_component->resource_class = resource_divider;
					new_component->delay_cycle = DIVIDER_CYCLE_DELAY;
				} else if(multiplier == type) {
					new_component->resource_class = resource_multiplier;
					new_component->delay_cycle = MULTIPLIER_CYCLE_DELAY;
				} else if(mux2x1 == type || comparator == type || shift_left == type || shift_right == type) {
					new_component->resource_class = resource_logical;
					new_component->delay_cycle = LOGICAL_CYCLE_DELAY;
				} else if(component_if_else == type) {
					new_component->is_scheduled = TRUE;
					new_component->resource_class = resource_none;
					new_component->delay_cycle = 1;
				}
			}
		}
	}
	return new_component;
}

void Component_SchedulePathASAP(component* self, uint8_t cycle) {
	uint8_t output_idx;
	uint8_t cycle_completed;
	char log_msg[128];
	if(NULL != self) {
		if(cycle > self->cycle_started_asap) {
			self->cycle_started_asap = cycle;
			self->time_frame[0] = self->cycle_started_asap;
			cycle_completed = cycle + self->delay_cycle;
			sprintf(log_msg, "MSG: Component scheduled from cycle %d to %d\n", cycle, cycle_completed);
			LogMessage(log_msg, MESSAGE_LEVEL);

			for(output_idx = 0; output_idx < self->num_outputs; output_idx++) {
				Net_SchedulePathASAP(self->output_ports[output_idx].port_net, cycle_completed);
			}
		}
	}
}

uint8_t Component_SchedulePathALAP(component* self, uint8_t cycle) {
	uint8_t ret_value = SUCCESS;
	uint8_t input_idx;
	char log_msg[128];
	if(NULL != self) {
		uint8_t cycle_started = cycle - self->delay_cycle;
		if(cycle <= self->delay_cycle) {
			LogMessage("Error(Component_SchedulePathALAP): Circuit cannot meet latency\n", CIRCUIT_ERROR_LEVEL);
			ret_value = FAILURE;
		} else if(cycle_started < self->cycle_started_alap){
			self->cycle_started_alap = cycle_started;
			self->time_frame[1] = self->cycle_started_alap;
			sprintf(log_msg, "MSG(Component_SchedulePathALAP): Component scheduled from cycle %d to %d\n", self->cycle_started_alap, cycle);
			LogMessage(log_msg, MESSAGE_LEVEL);

			for(input_idx = 0; input_idx < self->num_inputs; input_idx++) {
				ret_value = Net_SchedulePathALAP(self->input_ports[input_idx].port_net, self->cycle_started_alap);
				if(FAILURE == ret_value) {
					break;
				}
			}
		}
	}
	return ret_value;
}

void Component_SchedulePathFDS(component* self, uint8_t cycle) {
	uint8_t idx;
	if(NULL != self) {
		if(cycle > self->time_frame[1] || cycle < self->time_frame[0]) {
			LogMessage("ERROR: Component scheduled outside of time frame\n", ERROR_LEVEL);
		} else if(self->is_scheduled == FALSE || self->type == component_if_else) {
			self->time_frame[0] = cycle;
			self->time_frame[1] = cycle;
			self->cycle_scheduled = cycle;
			self->is_scheduled = TRUE;
			for(idx = 0; idx < self->num_inputs;idx++) { //Update time frames of predecessors
				Net_UpdateTimeFrameEnd(self->input_ports[idx].port_net, cycle);
			}
			for(idx = 0; idx < self->num_outputs; idx++) { //Update time frames of successors
				Net_UpdateTimeFrameStart(self->output_ports[idx].port_net, (self->delay_cycle + cycle));
			}
		}
	}

}

float Component_CalculateSelfForce(component* self, circuit* circ, uint8_t cycle) {
	if(NULL == self || NULL == circ) return 0.0f;
	uint8_t idx;
	float dg, prob, sf, partial_sum;
	prob = 1.0f / ((float) ((self->time_frame[1] - self->time_frame[0]) + 1.0f));
	sf = 0.0f;
	if(cycle > self->time_frame[1] || cycle < self->time_frame[0]) return 0.0f;
	for(idx = self->time_frame[0]; idx <= self->time_frame[1]; idx++) {
		dg = Circuit_GetDistributionGraph(circ, self->resource_class, idx);
		if(cycle == idx) {
			partial_sum = dg * (1.0f - prob);
		} else {
			partial_sum = dg * (0.0f - prob);
		}
		sf += partial_sum;
	}
	return sf;
}

float Component_CalculateSuccessorForce(component* self, circuit* circ, uint8_t cycle) {
	if(NULL == self || NULL == circ) return 0.0f;
	uint8_t idx;
	net* successor_net = NULL;
	uint8_t net_cycle = cycle + self->delay_cycle;
	float successor_force = 0.0f;
	if(cycle > self->cycle_started_asap) {
		successor_force = Component_CalculateSelfForce(self, circ, cycle);
		for(idx = 0; idx < self->num_outputs; idx++) {
			successor_net = self->output_ports[idx].port_net;
			if(NULL != successor_net) {
				successor_force += Net_CalculateSuccessorForce(successor_net, circ, net_cycle);
			}
		}
	}
	return successor_force;
}

float Component_CalculatePredecessorForce(component* self, circuit* circ, uint8_t cycle) {
	if(NULL == self || NULL == circ) return 0.0f;
	uint8_t idx;
	net* predecessor_net = NULL;
	uint8_t net_cycle = cycle + self->delay_cycle;
	float predecessor_force = 0.0f;
	if(cycle < self->cycle_started_alap) {
		predecessor_force = Component_CalculateSelfForce(self, circ, cycle);
		for(idx = 0; idx < self->num_inputs; idx++) {
			predecessor_net = self->input_ports[idx].port_net;
			if(NULL != predecessor_net) {
				predecessor_force += Net_CalculatePredecessorForce(predecessor_net, circ, net_cycle);
			}
		}
	}
	return predecessor_force;
}

void Component_UpdateTimeFrameStart(component* self, uint8_t cycle) {
	if(NULL != self) {
		if(FALSE == self->is_scheduled && cycle > self->time_frame[0]) {
			self->time_frame[0] = cycle;
			uint8_t idx;
			for(idx = 0; idx < self->num_outputs; idx++) {
				Net_UpdateTimeFrameStart(self->output_ports[idx].port_net, (cycle+self->delay_cycle));
			}
		}
	}
}

void Component_UpdateTimeFrameEnd(component* self, uint8_t cycle) {
	uint8_t new_cycle = cycle - self->delay_cycle;
	uint8_t idx;
	if(NULL != self) {
		if((FALSE == self->is_scheduled || component_if_else == self->type) && new_cycle < self->time_frame[1]) {
			self->time_frame[1] = new_cycle;
			for(idx = 0; idx < self->num_inputs; idx++) {
				Net_UpdateTimeFrameEnd(self->input_ports[idx].port_net, new_cycle);
			}
		}
	}
}

uint8_t Component_GetTimeFrameEnd(component* self) {
	uint8_t time = 0;
	if(NULL != self) {
		time = self->time_frame[1];
	}
	return time;
}

uint8_t Component_GetTimeFrameStart(component* self) {
	uint8_t time = 0;
	if(NULL != self) {
		time = self->time_frame[0];
	}
	return time;
}

uint8_t Component_GetDelayCycle(component* self) {
	uint8_t ret_value = 0;
	if(NULL != self) {
		ret_value = self->delay_cycle;
	}
	return ret_value;
}

uint8_t Component_GetIsScheduled(component* self) {
	uint8_t ret_value = TRUE;
	if(NULL != self) {
		ret_value = self->is_scheduled;
	}
	return ret_value;
}

resource_type Component_GetResourceType(component* self) {
	resource_type ret_value = resource_error;
	if(NULL != self) {
		ret_value = self->resource_class;
	}
	return ret_value;
}

float Component_GetProbability(component* self, uint8_t cycle) {
	float ret_value = 0.0f;
	if(NULL != self) {
		if(cycle >= self->time_frame[0] && cycle <= self->time_frame[1]) {
			ret_value = 1.0f / (((float)(self->time_frame[1] - self->time_frame[0])) + 1.0f);
		}
	}
	return ret_value;
}

uint8_t Component_AddInputPort(component* self, net* input, port_type type) {
	uint8_t ret_value = SUCCESS;
	if(NULL != self && NULL != input) {
		if(8 > self->num_inputs) {
			self->input_ports[self->num_inputs].port_net = input;
			self->input_ports[self->num_inputs].type = type;
			self->num_inputs++;
			Net_AddReceiver(input, self);
			if(datapath_a == type || datapath_b == type) {
				if(net_signed == Net_GetSign(input)) {
					if(self->type != load_register && self->type != mux2x1 && self->type != shift_left && self->type != shift_right) {
						self->sign = net_signed;
					}
				}
			} else if(port_if == type || port_else == type) {
				component* cond_comp = Net_GetDriver(input);
				port temp_port;
				temp_port = Component_GetInputPort(cond_comp, 0);
				self->conditional.net_condition = temp_port.port_net;
				if(port_if == type)
					self->conditional.type = transition_if;
				else
					self->conditional.type = transition_else;
			}
			if(Net_GetWidth(input) > self->width && comparator == self->type) {
				self->width = Net_GetWidth(input);
			}
		} else {
			ret_value = FAILURE;
		}
	} else {
		ret_value = FAILURE;
	}
	return ret_value;
}

uint8_t Component_AddOutputPort(component* self, net* output, port_type type) {
	uint8_t ret_value = SUCCESS;
	if(NULL != self && NULL != output) {
		if(8 > self->num_outputs) {
			self->output_ports[self->num_outputs].port_net = output;
			self->output_ports[self->num_outputs].type = type;
			self->num_outputs++;
			Net_AddDriver(output, self);
			if(self->type != comparator) {
				self->width = Net_GetWidth(output);

			}
		} else {
			ret_value = FAILURE;
		}
	} else {
		ret_value = FAILURE;
	}
	return ret_value;
}

port Component_GetInputPort(component* self, uint8_t idx) {
	port ret_value;
	ret_value.port_net = NULL;
	ret_value.type = port_error;
	if(NULL != self) {
		if(idx < self->num_inputs) ret_value = self->input_ports[idx];
	}
	return ret_value;
}

port Component_GetOutputPort(component* self, uint8_t idx) {
	port ret_value;
	ret_value.port_net = NULL;
	ret_value.type = port_error;
	if(NULL != self) {
		if(idx < self->num_outputs) ret_value = self->output_ports[idx];
	}
	return ret_value;
}

uint8_t Component_GetNumInputs(component* self) {
	uint8_t ret_value = 0;
	if(NULL != self) {
		ret_value = self->num_inputs;
	}
	return ret_value;
}

uint8_t Component_GetNumOutputs(component* self) {
	uint8_t ret_value = 0;
	if(NULL != self) {
		ret_value = self->num_outputs;
	}
	return ret_value;
}

condition Component_GetCondition(component* self) {
	condition ret_value = {.net_condition = NULL, .type = transition_all};
	if(NULL != self) {
		ret_value = self->conditional;
	}
	return ret_value;
}

component_type Component_GetType(component* self) {
	component_type type = component_unknown;
	if(NULL != self) {
		type = self->type;
	}
	return type;
}

uint8_t Component_GetWidth(component* self) {
	uint8_t width = 0;
	if(NULL != self) {
		width = self->width;
	}
	return width;
}

net_sign Component_GetSign(component* self) {
	net_sign ret_value = net_error;
	if(NULL != self) {
		ret_value = self->sign;
	}
	return ret_value;
}

void Component_Destroy(component** self) {
	if(NULL != *self) {
		free((*self));
		*self = NULL;
	}
}

uint8_t Component_PrintOperation(component* op, char* equ) {
	int idx = 0;
	char o[16];
	char a[16];
	char b[16];
	char sh[16];
	char sel[16];
	char eqn[64];
	if(NULL == op || NULL == equ) return 0;

	//Get Inputs
	for(idx=0;idx<op->num_inputs;idx++) {
		switch(op->input_ports[idx].type) {
		case datapath_a:
			Net_GetName(op->input_ports[idx].port_net, a);
			break;
		case datapath_b:
			Net_GetName(op->input_ports[idx].port_net, b);
			break;
		case mux_sel:
			Net_GetName(op->input_ports[idx].port_net, sel);
			break;
		case shift_amount:
			Net_GetName(op->input_ports[idx].port_net, sh);
			break;
		default:
			break;
		}
	}
	//Get output name
	Net_GetName(op->output_ports[0].port_net, o);

	switch(op->type) {
	case adder:
		sprintf(eqn,"%s <= %s + %s;", o, a, b);
		break;
	case subtractor:
		sprintf(eqn,"%s <= %s - %s;", o, a, b);
		break;
	case multiplier:
		sprintf(eqn,"%s <= %s * %s;", o, a, b);
		break;
	case comparator:
		if(greater_than_out == op->output_ports[0].type) {
			sprintf(eqn,"%s <= %s > %s;", o, a, b);
		} else if(less_than_out == op->output_ports[0].type) {
			sprintf(eqn,"%s <= %s < %s;", o, a, b);
		} else if(equal_out == op->output_ports[0].type) {
			sprintf(eqn,"%s <= %s == %s;", o, a, b);
		}
		break;
	case mux2x1:
		sprintf(eqn,"%s <= %s ? %s : %s;", o, sel, a, b);
		break;
	case shift_right:
		sprintf(eqn,"%s <= %s >> %s;", o, a, sh);
		break;
	case shift_left:
		sprintf(eqn,"%s <= %s << %s;", o, a, sh);
		break;
	case divider:
		sprintf(eqn,"%s <= %s / %s;", o, a, b);
		break;
	case modulo:
		sprintf(eqn,"%s <= %s %% %s;", o, a, b);
		break;
	default:
		return 0;
		break;
	}
	strcpy(equ, eqn);
	return strlen(eqn);

}
