/*
 * netlist_reader.h
 *
 *  Created on: Oct 18, 2020
 *      Author: cwhos
 */

#ifndef NETLIST_READER_H_
#define NETLIST_READER_H_

#include "global.h"

uint8_t ReadNetlist(char* file_name, circuit* netlist_circuit);

word_class CheckWordType(char* word);
component_type ReadComponentType(char* word);
uint8_t ReadNetWidth(char* word);
net_type ReadNetType(char* word);
net_sign ReadNetSign(char* word);

void ClearConditionalStack();

void TestNetlistReader();
void TestComponentParsing();
void TestDeclarations();

#endif /* NETLIST_READER_H_ */
