#ifndef debug_h
#define debug_h

#ifdef SERIAL_LOG_MAIN_ON

#include "debug.cpp"

void LOG_BEGIN();

void LOG(String msg);

#endif
#endif