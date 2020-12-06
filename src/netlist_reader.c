/*
 * netlist_reader.c
 *
 *  Created on: Oct 18, 2020
 *      Author: cwhos
 */

#include "netlist_reader.h"
#include "net.h"
#include "circuit.h"
#include "logger.h"
#include "component.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


static uint8_t ParseNetlistLine(char* line, circuit* netlist_circuit);
static uint8_t ParseAssignmentLine(char* first_word, circuit* netlist_circuit);
static uint8_t ParseDeclarationLine(char* first_word, circuit* netlist_circuit);
static uint8_t BufferNet(net** reg_net, circuit* netlist_circuit);

uint8_t ReadNetlist(char* file_name, circuit* netlist_circuit) {


	   FILE* fp;
	   char buff[255];
	   char message[32];
	   uint8_t line_number = 1;
	   uint8_t ret = SUCCESS;
	   fp = fopen(file_name, "r");
	   if(NULL == fp) {
		   printf("Error: File Open\n");
		   return FAILURE;
	   }

	   char* fget_rtn = fgets(&buff[0], 250, fp);
	   while(NULL != fget_rtn) {
		   //Log info
		   sprintf(message, "MSG: Parsing Line # %d\n", line_number);
		   LogMessage(&message[0], MESSAGE_LEVEL);

		   ret = ParseNetlistLine(&buff[0], netlist_circuit);
		   if(SUCCESS != ret) break;

		   fget_rtn = fgets(&buff[0], 250, fp);
		   line_number++;
	   }

	   fclose(fp);
	   return ret;
}

uint8_t ParseAssignmentLine(char* first_word, circuit* netlist_circuit) {
	//Determine inputs, outputs, and component type
	net* component_nets[4] = {NULL, NULL, NULL, NULL};
	component_type type;
	component* new_component;
	char* word = first_word;
	uint8_t word_idx = 1;
	uint8_t net_idx = 0;
	uint8_t ret = SUCCESS;
	port_type output_type = datapath_out;
	LogMessage("MSG: Parsing Variable Assignment\n", MESSAGE_LEVEL);
	while(NULL != word) {
		if(1 == word_idx || 3 == word_idx || 5 == word_idx || 7 == word_idx) { //output variable
			if(1 == word_idx) {
				component_nets[net_idx] = Circuit_FindNet(netlist_circuit, first_word);
			} else {
				component_nets[net_idx] = Circuit_FindNet(netlist_circuit, word);
			}
			if(NULL == component_nets[net_idx]) {
				if(5 == word_idx && 0 == strcmp(word, "1")) {
					if(adder == type) {
						type = incrementer;
					} else if(subtractor == type) {
						type = decrementer;
					} else {
						LogMessage("ERROR: Unknown Component\n", ERROR_LEVEL);
						ret = FAILURE;
						break;
					}
				} else {
					LogMessage("ERROR: Undeclared variable used\n", CIRCUIT_ERROR_LEVEL);
					ret = FAILURE;
					break;
				}
			}
			net_idx++;
		} else if(2 == word_idx) { //= sign
			if(0 != strcmp(word, "=")) {
				LogMessage("ERROR: Syntax - Assignment\n", ERROR_LEVEL);
				ret = FAILURE;
				break;
			} else {
				type = load_register;
			}
		} else if(4 == word_idx) {
			type = ReadComponentType(word);
			if(component_unknown == type) {
				LogMessage("ERROR: Unknown Component\n", CIRCUIT_ERROR_LEVEL);
				ret = FAILURE;
			} else if(comparator == type) {
				LogMessage("MSG: Component is comparator\n", MESSAGE_LEVEL);
				if(0 == strcmp("<", word)) {
					output_type = less_than_out;
				} else if(0 == strcmp("==", word)) {
					output_type = equal_out;
				} else if(0 == strcmp(">", word)) {
					output_type = greater_than_out;
				}
			}
		} else if(6 == word_idx) {
			if(0 != strcmp(":", word)) {
				LogMessage("ERROR: Mux Syntax\n", ERROR_LEVEL);
				ret = FAILURE;
				break;
			} else {
				LogMessage("MSG: Component is mux\n", MESSAGE_LEVEL);
			}
		} else {
			LogMessage("ERROR: Syntax\n", ERROR_LEVEL);
			ret = FAILURE;
			break;
		}
		word = strtok(NULL," ,\r\n");
		word_idx++;
	}

	new_component = Component_Create(type);
	if(NULL != new_component) {
		uint8_t output_idx = 0;
		uint8_t input_a_idx = 1;
		uint8_t input_b_idx = 2;
		uint8_t input_ctrl_idx = 2;
		port_type control_type = port_error;
		switch(type) {
		case mux2x1:
			control_type = mux_sel;
			input_b_idx = 3;
			input_a_idx = 2;
			input_ctrl_idx = 1;
			break;
		case load_register:
			LogMessage("MSG: Component is register\n", MESSAGE_LEVEL);
			output_type = reg_out;
			break;
		case shift_left:
			LogMessage("MSG: Component is SHL\n", MESSAGE_LEVEL);
			control_type = shift_amount;
			break;
		case shift_right:
			LogMessage("MSG: Component is SHR\n", MESSAGE_LEVEL);
			control_type = shift_amount;
			break;
		case adder:
			LogMessage("MSG: Component is +\n", MESSAGE_LEVEL);
			output_type = sum_out;
			break;
		case subtractor:
			LogMessage("MSG: Component is -\n", MESSAGE_LEVEL);
			output_type = diff_out;
			break;
		case multiplier:
			LogMessage("MSG: Component is *\n", MESSAGE_LEVEL);
			output_type = prod_out;
			break;
		case divider:
			LogMessage("MSG: Component is divider\n", MESSAGE_LEVEL);
			output_type = quot_out;
			break;
		case modulo:
			LogMessage("MSG: Component is modulo\n", MESSAGE_LEVEL);
			output_type = rem_out;
			break;
		default:
			break;
		}
		Component_AddInputPort(new_component, component_nets[input_a_idx], datapath_a);
		Component_AddInputPort(new_component, component_nets[input_b_idx], datapath_b);
		if(port_error != control_type) Component_AddInputPort(new_component, component_nets[input_ctrl_idx], control_type);
		Component_AddOutputPort(new_component, component_nets[output_idx], output_type);
		Circuit_AddComponent(netlist_circuit, new_component);
	} else {
		LogMessage("ERROR: Component could not be created\n", ERROR_LEVEL);
		ret = FAILURE;
	}
	return ret;
}

uint8_t ParseDeclarationLine(char* first_word, circuit* netlist_circuit) {
	//Determine width, net type, names
	char* word;
	net_type declare_type;
	uint8_t declare_width;
	net_sign declare_sign;
	net* new_net;
	uint8_t ret = SUCCESS;

	LogMessage("MSG: Parsing Net Declaration\n", MESSAGE_LEVEL);

	//Get Declaration Type (i.e. reg, wire, input, output)
	declare_type = ReadNetType(first_word);
	if(net_error == declare_type) {
		LogMessage("ERROR: Unknown Net Type\n", ERROR_LEVEL);
		return FAILURE;
	}

	//Determine Declaration sign
	word = strtok (NULL," ,\r\n");
	declare_sign = ReadNetSign(word);

	//Determine Declaration Width
	declare_width = ReadNetWidth(word);
	if(0 == declare_width) {
		LogMessage("ERROR: Unknown Net Width\n", ERROR_LEVEL);
		return FAILURE;
	}

	//Get all variable declarations in current line
	word = strtok (NULL," ,\r\n\t");
	while(NULL != word) {
		if(VARIABLE != CheckWordType(word)) break;
		if(NULL != Circuit_FindNet(netlist_circuit, word)) { //Declared Variable already exists
			LogMessage("ERROR: Variable redefined\n", ERROR_LEVEL);
			ret = FAILURE;
			break;
		}
		new_net = Net_Create(word, declare_type, declare_sign, declare_width);
		Circuit_AddNet(netlist_circuit, new_net);
		word = strtok (NULL," ,\r\n\t");
	}
	return ret;
}

uint8_t ParseNetlistLine(char* line, circuit* netlist_circuit) {

	uint8_t ret = SUCCESS;
	char* word;
	word_class word_type;
	if(NULL != line) {
	   word = strtok(line," ,\r\n");

	   //First Word determines reading behavior
	   word_type = CheckWordType(word); //Determine what type of word is (Variable, Net Declarative, Component Declarative, Width Declarative)
	   switch(word_type) {
	   case VARIABLE:
		   //If var exists, assignment, otherwise error
		   if(NULL != Circuit_FindNet(netlist_circuit, word)) {
			   ret = ParseAssignmentLine(line, netlist_circuit);
		   } else {
			   LogMessage("ERROR: Undeclared variable used\n", CIRCUIT_ERROR_LEVEL);
			   ret = FAILURE;
		   }
		   break;
	   case NET_DECLARATION:
		   ret = ParseDeclarationLine(line, netlist_circuit);
		   break;
	   case COMMENT_DECLARATION:
		   LogMessage("MSG: Line Ignored - Comment\n", MESSAGE_LEVEL);
		   break;
	   default://Error
		   LogMessage("ERROR: Unknown Line\n", ERROR_LEVEL);
		   ret = FAILURE;
		   break;
	   }
	}
	return ret;
}

uint8_t BufferNet(net** reg_net, circuit* netlist_circuit) {

	uint8_t ret_value = SUCCESS;
	char new_net_name[64], old_net_name[64];
	net* buffered_net = *reg_net;
	net* unbuffered_net = NULL;
	component* new_reg;
	if(NULL != buffered_net && NULL != netlist_circuit) {
		Net_GetName(buffered_net, old_net_name);
		strcpy(new_net_name, "reg_in_");
		strcat(new_net_name, old_net_name);
		unbuffered_net = Net_Create(new_net_name, net_wire, Net_GetSign(buffered_net), Net_GetWidth(buffered_net));
		if(NULL != unbuffered_net) {
			Circuit_AddNet(netlist_circuit, unbuffered_net);
			new_reg =  Component_Create(load_register);
			if(NULL != new_reg) {
				Component_AddOutputPort(new_reg, buffered_net, reg_out);
				Component_AddInputPort(new_reg, unbuffered_net, datapath_a);
				Circuit_AddComponent(netlist_circuit, new_reg);
				*reg_net = unbuffered_net;
			} else {
				//Error
				ret_value = FAILURE;
			}
		} else {
			//Error
			ret_value = FAILURE;
		}
	}
	return ret_value;
}

word_class CheckWordType(char* word) {

	word_class ret_value = WORD_ERROR;
	if(NULL == word) return COMMENT_DECLARATION;
	if(0 == strcmp(word, "input") || 0 == strcmp(word, "output") || 0 == strcmp(word, "wire") || 0 == strcmp(word, "register") || 0 == strcmp(word, "variable")) {
		ret_value = NET_DECLARATION;
	} else if(component_unknown != ReadComponentType(word)) {
		ret_value = COMPONENT_DECLARATION;
	} else if(('/' == word[0] && '/' == word[1]) || '\n' == word[0] || '\r' == word[0]) {
		ret_value = COMMENT_DECLARATION;
	} else if(0 != ReadNetWidth(word)) {
		ret_value = WIDTH_DECLARATION;
	} else { //Word is not a keyword
		ret_value = VARIABLE;
	}
	return ret_value;
}

component_type ReadComponentType(char* word) {

	component_type ret_value = component_unknown;

	if(0 == strcmp(word, "+")) {
		ret_value = adder;
	} else if(0 == strcmp(word, "-")) {
		ret_value = subtractor;
	} else if(0 == strcmp(word, "*")) {
		ret_value = multiplier;
	} else if(0 == strcmp(word, "%")) {
		ret_value = modulo;
	} else if(0 == strcmp(word, "/")) {
		ret_value = divider;
	} else if(0 == strcmp(word, "<<")) {
		ret_value = shift_left;
	} else if(0 == strcmp(word, ">>")) {
		ret_value = shift_right;
	} else if(0 == strcmp(word, "?")) {
		ret_value = mux2x1;
	} else if(0 == strcmp(word, "<") || 0 == strcmp(word, ">") || 0 == strcmp(word, "==")) {
		ret_value = comparator;
	} else if(0 == strcmp(word, "if")) {
		ret_value = transition_if;
	} else if(0 == strcmp(word, "else")) {
		ret_value = transition_else;
	}

	return ret_value;
}

uint8_t ReadNetWidth(char* word) {
	uint8_t return_width = 0;
	//Determine Declaration Width
	if(0 == strcmp(word, "Int1\0") || 0 == strcmp(word, "UInt1\0")) {
		return_width = 1;
	} else if(0 == strcmp(word, "Int2\0") || 0 == strcmp(word, "UInt2\0")) {
		return_width = 2;
	} else if(0 == strcmp(word, "Int8\0") || 0 == strcmp(word, "UInt8\0")) {
		return_width = 8;
	}  else if(0 == strcmp(word, "Int16\0") || 0 == strcmp(word, "UInt16\0")) {
		return_width = 16;
	}  else if(0 == strcmp(word, "Int32\0") || 0 == strcmp(word, "UInt32\0")) {
		return_width = 32;
	}  else if(0 == strcmp(word, "Int64\0") || 0 == strcmp(word, "UInt64\0")) {
		return_width = 64;
	}
	return return_width;
}

net_type ReadNetType(char* word) {
	net_type type = net_error;
	if(0 == strcmp(word, "input\0")) {
		type = net_input;
	} else if(0 == strcmp(word, "output\0")) {
		type = net_output;
	} else if(0 == strcmp(word, "wire\0")) {
		type = net_wire;
	} else if(0 == strcmp(word, "register\0")) {
		type = net_reg;
	} else if(0 == strcmp(word, "variable")) {
		type = net_variable;
	}
	return type;
}

net_sign ReadNetSign(char* word) {
	net_sign sign = sign_error;

	if(0 <= strcmp(word, "Int1") && 0 >= strcmp(word, "Int8")) { //Signed
		sign = net_signed;
	} else if(0 <= strcmp(word, "UInt1") && 0 >= strcmp(word, "UInt8")) { //Unsigned
		sign = net_unsigned;
	}
	return sign;
}
