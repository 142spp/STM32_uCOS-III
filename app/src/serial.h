#ifndef SERIAL_H
#define SERIAL_H

#include  <includes.h>

void Serial_Init(void);
CPU_BOOLEAN Serial_ReadChar(char *ch);
void Serial_WriteChar(char ch);
void Serial_WriteString(const char *str);

#endif
