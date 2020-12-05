#include "file_writer.h"
#include "circuit.h"
#include "net.h"
#include "component.h"
#include "logger.h"
#include "state_machine.h"
#include "state.h"
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

void PrintStateMachine(char* file_name, circuit* circ, state_machine* sm, int latency) {
	if(NULL == file_name || NULL == circ) return;

	FILE* fp;
	uint8_t idx;
	uint8_t num_nets = Circuit_GetNumNet(circ);
	uint8_t num_ins = 0;
	uint8_t num_outs = 0;
	uint8_t num_vars = 0;
	int print_return;

	char log_msg[128];
	char in_list[256] = "";
	char out_list[256] = "";
	char format[] = ", ";
	char net_name[64];
	char line_buffer[512];
	net* list_temp = NULL;
	net* temp_net = NULL;

	uint8_t init_cycle = 0;
	void* cond = NULL;
	state* curr_state = StateMachine_FindState(sm, cond, init_cycle);


	LogMessage("MSG: Writing Circuit to file\n", MESSAGE_LEVEL);

	fp = fopen(file_name, "w+");
	if(NULL == fp) {
		LogMessage("Error: Cannot open output file\n", ERROR_LEVEL);
		return;
	}

	fprintf(fp, "TODO: Implement file write\n");

	// Create inputs list
	for(idx = 0; idx < num_nets; idx++) {
		list_temp = Circuit_GetNet(circ, idx);
		if(net_input == Net_GetType(list_temp)) {
			Net_GetName(list_temp, net_name);
			strcat(net_name, format);
			strcat(in_list, net_name);
		}
		else if(net_input != Net_GetType(list_temp) && net_output != Net_GetType(list_temp)) {
			num_vars++;
		}
	}

	// Create outputs list
	num_outs = num_nets - num_ins - num_vars;

	for(idx = 0; idx < num_nets; idx++) {
		list_temp = Circuit_GetNet(circ, idx);
		if(net_output == Net_GetType(list_temp)) {
			Net_GetName(list_temp, net_name);
			if(num_outs > 1) {
				strcat(net_name, format);
				num_outs--;
			}
			strcat(out_list, net_name);
		}
	}


	fputs("'timescale 1ns/1ps\n", fp);
	fputs("'default_nettype none\n", fp);
	fprintf(fp, "module HLSM(Clk, Rst, Start, %sDone, %s);\n", in_list, out_list);
	fputs("\n", fp);
	fputs("localparam OUTPUT_WIDTH = 2*DATA_WIDTH;\n", fp);
	fputs("\n", fp);

	// List inputs
	LogMessage("MSG: Writing I/O\n", MESSAGE_LEVEL);
	fputs("\tinput Clk, Rst, Start;\n", fp);
	for(idx = 0; idx < num_nets; idx++) {
		temp_net = Circuit_GetNet(circ, idx);
		if(NULL == temp_net) {
			LogMessage("Error: Could not retrieve net\n", ERROR_LEVEL);
			break;
		}
		else if(net_input == Net_GetType(temp_net)) {
			DeclareNet(temp_net, line_buffer);
			print_return = fprintf(fp, line_buffer);
			if(0 >= print_return) {
				sprintf(log_msg, "Error: Could not print to file - %d\n", print_return);
				LogMessage(log_msg, ERROR_LEVEL);
				break;
			}
		}
	}

	fputs("\n", fp);

	// List outputs
	fputs("\tDone;\n", fp);
	for(idx = 0; idx < num_nets; idx++) {
		temp_net = Circuit_GetNet(circ, idx);
		if(NULL == temp_net) {
			LogMessage("Error: Could not retrieve net\n", ERROR_LEVEL);
			break;
		}
		else if(net_output == Net_GetType(temp_net)) {
			DeclareNet(temp_net, line_buffer);
			print_return = fprintf(fp, line_buffer);
			if(0 >= print_return) {
				sprintf(log_msg, "Error: Could not print to file - %d\n", print_return);
				LogMessage(log_msg, ERROR_LEVEL);
				break;
			}
		}
	}

	fputs("\n", fp);

	// List variables
	LogMessage("MSG: Writing internal nets\n", MESSAGE_LEVEL);
	for(idx = 0; idx < num_nets; idx++) {
		temp_net = Circuit_GetNet(circ, idx);
		if(NULL == temp_net) {
			LogMessage("Error: Could not retrieve net\n", ERROR_LEVEL);
			break;
		}
		else if(net_input != Net_GetType(list_temp) && net_output != Net_GetType(list_temp)) {
			DeclareNet(temp_net, line_buffer);
			print_return = fprintf(fp, line_buffer);
			if(0 >= print_return) {
				sprintf(log_msg, "Error: Could not print to file - %d\n", print_return);
				LogMessage(log_msg, ERROR_LEVEL);
				break;
			}
		}
	}

	fputs("\n", fp);

	fputs("\t always @(posedge clk) begin\n", fp);
	fputs("\t\t if(Rst) begin\n", fp);
	fputs("\t\t\t state <= 0;\n", fp);
	fputs("\t\t end else begin", fp);
	fputs("\t\t\t case(state)\n", fp);

	while(curr_state != NULL) {
		fprintf(fp, "\t\t 4'd%d: begin",curr_state->cycle);
		if(curr_state->cycle == 0) {
			fputs("\t\t\t\t state <= 0;", fp);
		}
		else if(curr_state->cycle == latency+1) {
			fputs("\t\t\t\t Done = 1;\n", fp);
			fputs("\t\t\t\t state <= 0;\n", fp);

		}
		else {
			for(idx = 0; idx < curr_state->num_operations; idx++) {
				fprintf(fp, "\t\t\t\t %s;\n", curr_state->operations[idx]);
			}
			fprintf(fp, "\t\t\t\t state <= %d;\n", curr_state->next_state->cycle);
		}
		fputs("\t\t\t end", fp);
		curr_state = curr_state->next_state;
	}
	fputs("\t\t\t endcase\n", fp);
	fputs("\t\t end\n", fp);
	fputs("\t end\n", fp);
	fputs("endmodule\n", fp);

	fputs("\n", fp);

	fputs("'default_nettype wire\n", fp);

	fclose(fp);
}

void DeclareNet(net* self, char* line_buffer) {
	if(NULL == self || NULL == line_buffer) return;
	net_type type = Net_GetType(self);
	uint8_t net_width = Net_GetWidth(self);
	char net_type_keyword[32];
	char net_name[64];
	Net_GetName(self, net_name);

	switch(type) {
	case net_input:
		strcpy(net_type_keyword, "input");
		break;
	case net_output:
		strcpy(net_type_keyword, "output");
		break;
	case net_wire:
		strcpy(net_type_keyword, "wire");
		break;
	case net_reg:
		strcpy(net_type_keyword, "reg");
		break;
	default:
		strcpy(net_type_keyword, "err");
		break;
	}

	if(net_signed == Net_GetSign(self)) {
		strcat(net_type_keyword, " signed");
	}

	sprintf(line_buffer, "\t%s [%d:0] %s;\n", net_type_keyword, (net_width-1), net_name);

}

void DeclareComponent(component* self, char* line_buffer, uint8_t comp_idx) {
	if(NULL == self || NULL == line_buffer) return;

	component_type type;
	net_sign component_sign;
	uint8_t width, padding_length, padding_bit;
	uint8_t io_idx, num_ports;
	port temp_port;
	char type_declaration[32];
	char component_name[128];
	char port_declaration[512] = "";
	char temp_port_declaration[128];
	char port_net_name[16];
	char port_name[64];

	if(NULL != self) {

		width = Component_GetWidth(self);
		component_sign = Component_GetSign(self);
		type = Component_GetType(self);

		num_ports = Component_GetNumOutputs(self) + Component_GetNumInputs(self);
		for(io_idx = 0; io_idx < num_ports; io_idx++) {

			if(io_idx < Component_GetNumInputs(self)) {
				temp_port = Component_GetInputPort(self, io_idx);
			} else {
				temp_port = Component_GetOutputPort(self, (io_idx - Component_GetNumInputs(self)));
			}

			Net_GetName(temp_port.port_net, port_net_name);
			if((Component_GetWidth(self) > Net_GetWidth(temp_port.port_net)) &&
					(datapath_a == temp_port.type || datapath_b == temp_port.type)) {

				padding_length = Component_GetWidth(self) - Net_GetWidth(temp_port.port_net);
				padding_bit = Net_GetWidth(temp_port.port_net) - 1;
				if(net_signed == Net_GetSign(temp_port.port_net)) {
					sprintf(port_name, "{{%d{%s[%d]}}, %s}", padding_length, port_net_name, padding_bit, port_net_name);
				} else if(net_unsigned == Net_GetSign(temp_port.port_net)) {
					sprintf(port_name, "{{%d{1'b0}}, %s}", padding_length, port_net_name);
				}
			} else {
				strcpy(port_name, port_net_name);
			}

			switch(temp_port.type) {
			case datapath_a:
				if(load_register == type) {
					sprintf(temp_port_declaration, ".d(%s)", port_name);
				} else {
					sprintf(temp_port_declaration, ".a(%s)", port_name);
				}
				break;
			case datapath_b:
				sprintf(temp_port_declaration, ".b(%s)", port_name);
				break;
			case datapath_out:
				sprintf(temp_port_declaration, ".d(%s)", port_name);
				break;
			case mux_sel:
				sprintf(temp_port_declaration, ".sel(%s)", port_name);
				break;
			case shift_amount:
				sprintf(temp_port_declaration, ".sh_amt(%s)", port_name);
				break;
			case greater_than_out:
				sprintf(temp_port_declaration, ".gt(%s)", port_name);
				break;
			case less_than_out:
				sprintf(temp_port_declaration, ".lt(%s)", port_name);
				break;
			case equal_out:
				sprintf(temp_port_declaration, ".eq(%s)", port_name);
				break;
			case reg_out:
				sprintf(temp_port_declaration, ".q(%s)", port_name);
				break;
			case sum_out:
				sprintf(temp_port_declaration, ".sum(%s)", port_name);
				break;
			case diff_out:
				sprintf(temp_port_declaration, ".diff(%s)", port_name);
				break;
			case prod_out:
				sprintf(temp_port_declaration, ".prod(%s)", port_name);
				break;
			case quot_out:
				sprintf(temp_port_declaration, ".quot(%s)", port_name);
				break;
			case rem_out:
				sprintf(temp_port_declaration, ".rem(%s)", port_name);
				break;
			default:
				break;
			}
			strcat(port_declaration, temp_port_declaration);
			if(io_idx != (num_ports - 1)) {
				strcat(port_declaration, ", ");
			}
		}

		switch(type) {
		case load_register:
			strcpy(type_declaration, "Reg");
			break;
		case adder:
			strcpy(type_declaration, "Add");
			break;
		case subtractor:
			strcpy(type_declaration, "Sub");
			break;
		case multiplier:
			strcpy(type_declaration, "Mul");
			break;
		case divider:
			strcpy(type_declaration, "Div");
			break;
		case modulo:
			strcpy(type_declaration, "Mod");
			break;
		case mux2x1:
			strcpy(type_declaration, "Mux2x1");
			break;
		case comparator:
			strcpy(type_declaration, "Comp");
			break;
		case shift_right:
			strcpy(type_declaration, "SHR");
			break;
		case shift_left:
			strcpy(type_declaration, "SHL");
			break;
		case incrementer:
			strcpy(type_declaration, "Inc");
			break;
		case decrementer:
			strcpy(type_declaration, "Dec");
			break;
		default:
			break;
		}
	}

	sprintf(component_name, "%s_%d", type_declaration, comp_idx);

	if(net_signed == component_sign) {
		sprintf(line_buffer, "\tS%s #(.DATA_WIDTH(%d)) %s (%s);\n", type_declaration, width, component_name, port_declaration);
	} else {
		sprintf(line_buffer, "\t%s #(.DATA_WIDTH(%d)) %s (%s);\n", type_declaration, width, component_name, port_declaration);
	}

}

void TestComponentDeclaration() {
	char comp_line[1024];
	component_type uut_type;
	component* uut;
	net* a;
	net* b;
	net* o;
	net* comp_o;
	net* sel;

	a = Net_Create("a", net_input, net_signed, 8);
	b = Net_Create("b", net_input, net_unsigned, 2);
	sel = Net_Create("sel", net_input, net_unsigned, 1);
	o = Net_Create("o", net_output, net_signed, 8);
	comp_o = Net_Create("gt", net_output, net_unsigned, 1);
	uint8_t comp_idx = 0;
	for(uut_type = load_register; uut_type < component_unknown; uut_type++) {
		uut = Component_Create(uut_type);
		Component_AddInputPort(uut, a, datapath_a);
		switch(uut_type) {
		case load_register:
			Component_AddOutputPort(uut, o, reg_out);
			break;
		case adder:
			Component_AddInputPort(uut, b, datapath_b);
			Component_AddOutputPort(uut, o, sum_out);
			break;
		case subtractor:
			Component_AddInputPort(uut, b, datapath_b);
			Component_AddOutputPort(uut, o, diff_out);
			break;
		case multiplier:
			Component_AddInputPort(uut, b, datapath_b);
			Component_AddOutputPort(uut, o, prod_out);
			break;
		case divider:
			Component_AddInputPort(uut, b, datapath_b);
			Component_AddOutputPort(uut, o, quot_out);
			break;
		case modulo:
			Component_AddInputPort(uut, b, datapath_b);
			Component_AddOutputPort(uut, o, rem_out);
			break;
		case mux2x1:
			Component_AddInputPort(uut, b, datapath_b);
			Component_AddInputPort(uut, sel, mux_sel);
			Component_AddOutputPort(uut, o, datapath_out);
			break;
		case comparator:
			Component_AddInputPort(uut, b, datapath_b);
			Component_AddOutputPort(uut, comp_o, greater_than_out);
			break;
		case shift_right:
			Component_AddInputPort(uut, b, shift_amount);
			Component_AddOutputPort(uut, o, datapath_out);
			break;
		case shift_left:
			Component_AddInputPort(uut, b, shift_amount);
			Component_AddOutputPort(uut, o, datapath_out);
			break;
		case incrementer:
			Component_AddOutputPort(uut, o, datapath_out);
			break;
		case decrementer:
			Component_AddOutputPort(uut, o, datapath_out);
			break;
		default:
			break;
		}
		DeclareComponent(uut, comp_line, comp_idx);
		printf(comp_line);
		Component_Destroy(&uut);
		comp_idx++;
	}
}
