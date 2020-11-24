/*
 * component.c
 *
 *  Created on: Oct 20, 2020
 *      Author: cwhos
 */
#include <stdio.h>
#include <stdlib.h>
#include "component.h"
#include "logger.h"
#include "net.h"

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
	net_sign sign;
	uint8_t width;

	port input_ports[8];
	port output_ports[8];
	uint8_t num_inputs;
	uint8_t num_outputs;

} component;

component* Component_Create(component_type type) {
	component* new_component = NULL;
	if(component_unknown != type) {
		new_component = (component*) malloc(sizeof(component));
		if(NULL != new_component) {
			if(component_unknown != type) {
				new_component->type = type;
				new_component->width = 0;
				new_component->delay_ns = 0.0f;
				new_component->sign = net_unsigned;
				new_component->num_inputs = 0;
				new_component->num_outputs = 0;
			}
		}
	}
	return new_component;
}

void Component_UpdatePathDelay(component* self, float path_delay_ns) {
	uint8_t output_idx;
	float new_delay_ns;
	char log_msg[128];
	if(NULL != self) {
		Component_UpdateDelay(self);

		if(load_register == self->type) {
			new_delay_ns = self->delay_ns;
		} else {
			new_delay_ns = path_delay_ns + self->delay_ns;
		}

		sprintf(log_msg, "MSG: Updating downstream path delay to %.2f ns\n", new_delay_ns);
		LogMessage(log_msg, MESSAGE_LEVEL);

		for(output_idx = 0; output_idx < self->num_outputs; output_idx++) {
			Net_UpdatePathDelay(self->output_ports[output_idx].port_net, new_delay_ns);
		}
	}
}

void Component_SchedulePathASAP(component* self, uint8_t cycle) {
	uint8_t output_idx;
	uint8_t cycle_completed;
	char log_msg[128];
	if(NULL != self) {
		if(cycle > self->cycle_started_asap) {

			self->cycle_started_asap = cycle;
			cycle_completed = cycle + self->delay_cycle;
			sprintf(log_msg, "MSG: Component schedules from cycle %d to %d\n", cycle, cycle_completed);
			LogMessage(log_msg, MESSAGE_LEVEL);

			for(output_idx = 0; output_idx < self->num_outputs; output_idx++) {
				Net_SchedulePathASAP(self->output_ports[output_idx].port_net, cycle_completed);
			}
		}
	}
}

void Component_SchedulePathALAP(component* self, uint8_t cycle) {
	uint8_t output_idx;
	char log_msg[128];
	if(NULL != self) {
		self->cycle_started_alap = cycle - self->delay_cycle;
		sprintf(log_msg, "MSG: Component schedules from cycle %d to %d\n", self->cycle_started_alap, cycle);
		LogMessage(log_msg, MESSAGE_LEVEL);

		for(output_idx = 0; output_idx < self->num_outputs; output_idx++) {
			Net_SchedulePathALAP(self->output_ports[output_idx].port_net, self->cycle_started_alap);
		}
	}
}

float Component_CalculateSelfForce(component* self, circuit* circ, uint8_t cycle) {
	if(NULL == self || NULL == circ) return 0.0f;
	uint8_t idx;
	float dg, prob, sf, partial_sum;
	prob = 1 / ((float) ((self->cycle_started_alap - self->cycle_started_asap) + 1));
	sf = 0.0f;
	if(self->cycle_started_alap || self->cycle_started_asap > cycle) return 0.0f;
	for(idx = self->cycle_started_asap; idx <= self->cycle_started_alap; idx++) {
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
	float successor_force = 0.0f;
	for(idx = 0; idx < self->num_outputs; idx++) {
		successor_net = self->output_ports[idx].port_net;
		if(NULL != successor_net) {
			successor_force += Net_CalculateSuccessorForce(successor_net, circ, cycle);
		}
	}
	return successor_force;
}

uint8_t ComponentGetCycleALAP(component* self) {
	uint8_t alap_time = 0;
	if(NULL != self) {
		alap_time = self->cycle_started_alap;
	}
	return alap_time;
}

uint8_t ComponentGetCycleASAP(component* self) {
	uint8_t asap_time = 0;
	if(NULL != self) {
		asap_time = self->cycle_started_asap;
	}
	return asap_time;
}

resource_type Component_GetResourceType(component* self) {
	resource_type ret_value = resource_error;
	if(NULL != self) {
		ret_value = self->resource_class;
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


void Component_UpdateDelay(component* self) {
	uint8_t width_idx;
	if(NULL != self) {
		char log_msg[64];
		switch(self->width) {
		case 1:
			width_idx = 0;
			break;
		case 2:
			width_idx = 1;
			break;
		case 8:
			width_idx = 2;
			break;
		case 16:
			width_idx = 3;
			break;
		case 32:
			width_idx = 4;
			break;
		case 64:
			width_idx = 5;
			break;
		default:
			LogMessage("ERROR: Incorrect Component Width\r\n", ERROR_LEVEL);
			return;
			break;
		}
		switch(self->type) {
		case load_register:
			self->delay_ns = reg_delays[width_idx];
			break;
		case adder:
			self->delay_ns = add_delays[width_idx];
			break;
		case subtractor:
			self->delay_ns = sub_delays[width_idx];
			break;
		case multiplier:
			self->delay_ns = mul_delays[width_idx];
			break;
		case divider:
			self->delay_ns = div_delays[width_idx];
			break;
		case modulo:
			self->delay_ns = mod_delays[width_idx];
			break;
		case mux2x1:
			self->delay_ns = mux2x1_delays[width_idx];
			break;
		case comparator:
			self->delay_ns = comp_delays[width_idx];
			break;
		case shift_right:
			self->delay_ns = shr_delays[width_idx];
			break;
		case shift_left:
			self->delay_ns = shl_delays[width_idx];
			break;
		case incrementer:
			self->delay_ns = inc_delays[width_idx];
			break;
		case decrementer:
			self->delay_ns = dec_delays[width_idx];
			break;
		default:
			break;
		}
		sprintf(log_msg, "MSG: Component Delay set to %.2f ns\n", self->delay_ns);
		LogMessage(log_msg, MESSAGE_LEVEL);
	}
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
