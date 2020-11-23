/*
 * logger.h
 *
 *  Created on: Oct 18, 2020
 *      Author: cwhos
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include <stdint.h>

void SetLogFile(char* file_path);
void SetLogLevel(uint8_t level);
void LogMessage(char* message, uint8_t level);
void CloseLog();


#endif /* LOGGER_H_ */
