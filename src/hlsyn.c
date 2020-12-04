/*
 ============================================================================
 Name        : hlsyn.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "global.h"
#include "logger.h"
#include "netlist_reader.h"
#include "circuit.h"
#include "state_machine.h"
#include "file_writer.h"

int main(int argc, char *argv[]) {

	char c_file[64];
	char verilog_file[64];

#if DEBUG_MODE == 1


	const uint8_t test_standard = TRUE;
#define num_standard_cases 7
	const uint8_t test_latency = FALSE;
#define num_latency_cases 6
	const uint8_t test_if = FALSE;
#define num_if_cases 4
	const uint8_t test_error = FALSE;
#define num_error_cases 3

	uint8_t idx;
	circuit* netlist_circuit;
	state_machine* sm;

	SetLogFile("./test/output.txt");
	SetLogLevel(MESSAGE_LEVEL);
	LogMessage("hlsyn started\n\0", MESSAGE_LEVEL);


	if(TRUE == test_standard) {
		uint8_t latency[num_standard_cases] = {4, 6, 10, 8, 11, 34, 8};
		for(idx = 0; idx < num_standard_cases; idx++) {
			//Create structs for iteration
			netlist_circuit = Circuit_Create(latency[idx]);
			sm = StateMachine_Create(latency[idx]);
			//Spell out file names
			sprintf(c_file, "./test/standard/hls_test%d.c", (idx+1));
			sprintf(verilog_file, "./test/outputs/standard%d.v", (idx+1));

			if(FAILURE != ReadNetlist(c_file, netlist_circuit)) {
				Circuit_ScheduleForceDirected(netlist_circuit, sm);
				PrintStateMachine(verilog_file, sm);
			} else {
				break;
			}
			//Destroy structs for this iteration to prep for next
			Circuit_Destroy(&netlist_circuit);
			StateMachine_Destroy(&sm);
		}
	}

	if(TRUE == test_latency) {
		uint8_t latency[num_latency_cases] = {4, 9, 2, 4, 6, 6};
		for(idx = 1; idx <= num_latency_cases; idx++) {
			netlist_circuit = Circuit_Create(latency[idx]);
			sm = StateMachine_Create(latency[idx]);
			sprintf(c_file, "./test/latency/hls_test%d.c", idx);
			sprintf(verilog_file, "./test/outputs/latency%d.v", idx);
			if(FAILURE != ReadNetlist(c_file, netlist_circuit)) {
				Circuit_ScheduleForceDirected(netlist_circuit, sm);
			}
			Circuit_Destroy(&netlist_circuit);
			StateMachine_Destroy(&sm);
		}
	}

	if(TRUE == test_if) {
		uint8_t latency[num_if_cases] = {4, 4, 8, 4};
		for(idx = 1; idx <= num_if_cases; idx++) {
			netlist_circuit = Circuit_Create(latency[idx]);
			sm = StateMachine_Create(latency[idx]);
			sprintf(c_file, "./test/if/hls_test%d.c", idx);
			sprintf(verilog_file, "./test/outputs/if%d.v", idx);
			if(FAILURE != ReadNetlist(c_file, netlist_circuit)) {
				Circuit_ScheduleForceDirected(netlist_circuit, sm);

			}
			Circuit_Destroy(&netlist_circuit);
			StateMachine_Destroy(&sm);
		}
	}

	if(TRUE == test_error) {
		uint8_t latency[num_error_cases] = {10, 10, 10}; //Doesn't matter just for function calls
		for(idx = 1; idx <= num_error_cases; idx++) {
			netlist_circuit = Circuit_Create(latency[idx]);
			sm = StateMachine_Create(latency[idx]);
			sprintf(c_file, "./test/error/hls_test%d.c", idx);
			sprintf(verilog_file, "./test/outputs/error%d.v", idx);
			if(FAILURE != ReadNetlist(c_file, netlist_circuit)) {
				Circuit_ScheduleForceDirected(netlist_circuit, sm);
			}
			Circuit_Destroy(&netlist_circuit);
			StateMachine_Destroy(&sm);
		}
	}

	CloseLog();

	return EXIT_SUCCESS;

#else

	if(argc < 4) {
		printf("ERROR: Not enough arguments.\n");
		return FAILURE;
	}
	else if (argc > 4) {
		printf("ERROR: Too many arguments.\n");
		return FAILURE;
	}
	else {
		c_file = argv[1];
		latency = argv[2];
		verilog_file = argv[3];
	}

	SetLogFile(NULL);
	SetLogLevel(CIRCUIT_ERROR_LEVEL);
	LogMessage("hlsyn started\n\0", MESSAGE_LEVEL);

	circuit* netlist_circuit = Circuit_Create();
	if(FAILURE == ReadNetlist(txt_file, netlist_circuit)) {
		Circuit_Destroy(netlist_circuit);
		return EXIT_FAILURE;
	}
	PrintFile(verilog_file, netlist_circuit);


	CloseLog();
	Circuit_Destroy(netlist_circuit);

	return EXIT_SUCCESS;

#endif
}
