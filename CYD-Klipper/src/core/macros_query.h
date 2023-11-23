#pragma once

typedef struct {
    const char** macros;
    uint32_t count;
} MACROSQUERY;

MACROSQUERY macros_query();
void macros_query_setup();