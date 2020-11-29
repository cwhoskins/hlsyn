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

int main(int argc, char *argv[]) {

#if DEBUG_MODE == 1

	return EXIT_SUCCESS;

#else

	char* c_file = NULL;
	char* verilog_file = NULL;
	char* latency = NULL;

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
