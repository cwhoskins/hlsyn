/*
 * net.h
 *
 *  Created on: Oct 17, 2020
 *      Author: cwhos
 */

#ifndef NET_H_
#define NET_H_

#include <stdint.h>
#include "global.h"

net* Net_Create(char* name, net_type type, net_sign sign, uint8_t width);

void Net_GetName(net* self, char* buffer);
net_type Net_GetType(net* self);
net_sign Net_GetSign(net* self);
uint8_t Net_GetWidth(net* self);
float Net_GetDelay(net* self);

void Net_ResetDelay(net* self);
void Net_UpdatePathDelay(net* self, float path_delay_ns);

void Net_SchedulePathASAP(net* self, uint8_t cycle);
void Net_SchedulePathALAP(net* self, uint8_t cycle);
float Net_CalculateSuccessorForce(net* self, circuit* circ, uint8_t cycle);
float Net_CalculatePredecessorForce(net* self, circuit* circ, uint8_t cycle);
void Net_UpdateTimeFrameStart(net* self, uint8_t cycle);
void Net_UpdateTimeFrameEnd(net* self, uint8_t cycle);

void Net_AddReceiver(net* self, component* new_receiver);
void Net_AddDriver(net* self, component* new_driver);
void Net_Destroy(net** self);

void PrintNet(net* self);
//void PrintNet(FILE* fp, net* self)
void TestPrintNet();

#endif /* NET_H_ */
