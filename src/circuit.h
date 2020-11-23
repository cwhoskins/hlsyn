/*
 * circuit.h
 *
 *  Created on: Oct 20, 2020
 *      Author: cwhos
 */

#ifndef CIRCUIT_H_
#define CIRCUIT_H_


#include "global.h"

circuit* Circuit_Create();
net* Circuit_FindNet(circuit* self, char* name);
component* Circuit_GetComponent(circuit* self, uint8_t idx);
uint8_t Circuit_GetNumComponent(circuit* self);
net* Circuit_GetNet(circuit* self, uint8_t idx);
uint8_t Circuit_GetNumNet(circuit* self);
void Circuit_AddNet(circuit* self, net* new_net);
void Circuit_AddComponent(circuit* self, component* new_component);
void Circuit_CalculateDelay(circuit* self);
float Circuit_GetCriticalPath(circuit* self);
void Circuit_Destroy(circuit* self);
void PrintCircuit(circuit* self);

#endif /* CIRCUIT_H_ */
