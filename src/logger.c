/*
 * logger.c
 *
 *  Created on: Oct 18, 2020
 *      Author: cwhos
 */
#include <stdio.h>
#include "logger.h"

typedef struct struct_logger {
	FILE* log_file;
	uint8_t minimum_log_level;
} logger;

logger dpgen_log;

void SetLogFile(char* file_path) {
	if(NULL != file_path) {
		dpgen_log.log_file = fopen(file_path, "w+");
		if(NULL == dpgen_log.log_file) {
		   printf("Error: Logging Failure\r\n");
		   return;
		}
	} else {
		dpgen_log.log_file = stdout;
	}
}
void SetLogLevel(uint8_t level) {
	dpgen_log.minimum_log_level = level;
}
void LogMessage(char* message, uint8_t level) {
	if(level >= dpgen_log.minimum_log_level) {
		fprintf(dpgen_log.log_file, message);
	}
}

void CloseLog() {
	fclose(dpgen_log.log_file);
}
