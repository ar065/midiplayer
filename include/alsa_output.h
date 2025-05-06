#ifndef ALSA_OUTPUT_H
#define ALSA_OUTPUT_H

#include <stdint.h>
#include <stdbool.h>

bool alsa_initialize(const char* port_string);
void alsa_send(uint32_t message);
void alsa_shutdown(void);

#endif // ALSA_OUTPUT_H
