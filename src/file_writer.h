#ifndef FILE_WRITER_H
#define FILE_WRITER_H

#include "global.h"

void PrintStateMachine(char* file_name, state_machine* sm);
void DeclareNet(net* self, char* line_buffer);
void DeclareComponent(component* self, char* line_buffer, uint8_t comp_idx);
void TestComponentDeclaration();

#endif
