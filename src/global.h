/*
 * global.h
 *
 *  Created on: Oct 20, 2020
 *      Author: cwhos
 */

#ifndef GLOBAL_H_
#define GLOBAL_H_

#include <stdint.h>

#define CIRCUIT_ERROR_LEVEL 7
#define ERROR_LEVEL 5
#define WARNING_LEVEL 3
#define MESSAGE_LEVEL 1
#define NO_LOG 0

#define SUCCESS 0
#define FAILURE 1

#define FALSE 0
#define TRUE 1

#define ALU_CYCLE_DELAY 1
#define DIVIDER_CYCLE_DELAY 3
#define MULTIPLIER_CYCLE_DELAY 2
#define LOGICAL_CYCLE_DELAY 1

#define DEBUG_MODE 0

//Net
typedef enum enum_net_type {
	net_wire=0,
	net_reg,
	net_input,
	net_output,
	net_variable,
	net_conditional,
	net_error
} net_type;

typedef enum {
	net_signed=0,
	net_unsigned,
	sign_error
} net_sign;

typedef struct struct_net net;

//Component
typedef enum enum_comp_type {
	load_register=0,
	adder,
	subtractor,
	multiplier,
	comparator,
	mux2x1,
	shift_right,
	shift_left,
	divider,
	modulo,
	incrementer,
	decrementer,
	component_if_else,
	component_unknown
} component_type;

typedef enum {
	datapath_a=0,
	datapath_b,
	datapath_out,
	mux_sel,
	shift_amount,
	greater_than_out,
	less_than_out,
	equal_out,
	reg_out,
	sum_out,
	diff_out,
	prod_out,
	quot_out,
	rem_out,
	port_if,
	port_else,
	port_prev_op,
	port_error
} port_type;

typedef enum {
	resource_multiplier=0,
	resource_divider,
	resource_logical,
	resource_alu,
	resource_none,
	resource_error
} resource_type;

//Netlist Reader
typedef enum {
	VARIABLE,
	NET_DECLARATION,
	COMPONENT_DECLARATION,
	COMMENT_DECLARATION,
	WIDTH_DECLARATION,
	IF_DECLARATION,
	ELSE_DECLARATION,
	CONDITIONAL_END,
	WORD_ERROR
} word_class;

typedef struct struct_component component;

typedef struct {
	net* port_net;
	port_type type;
} port;

typedef enum {
	transition_if=0,
	transition_else,
	transition_all,
	transition_error
} transition_type;

typedef struct struct_transition transition;

typedef struct struct_condition condition;
typedef struct struct_condition {
	transition_type type;
	net* net_condition;
	condition* prev_condition;
} condition;

//Circuit
typedef struct struct_circuit circuit;

//Component Delays
extern const float reg_delays[6];
extern const float add_delays[6];
extern const float sub_delays[6];
extern const float mul_delays[6];
extern const float comp_delays[6];
extern const float mux2x1_delays[6];
extern const float shr_delays[6];
extern const float shl_delays[6];
extern const float div_delays[6];
extern const float mod_delays[6];
extern const float inc_delays[6];
extern const float dec_delays[6];

//Resource
typedef struct struct_resource resource;
typedef struct struct_state state;
typedef struct struct_state_machine state_machine;



#endif /* GLOBAL_H_ */
