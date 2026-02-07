#pragma once

#include <stdint.h>

// Handle vFile and vSpectranext packets
// Returns static string response (must be sent immediately, not freed)
extern char* vfile_handle_v(const char* name, const char* args, uint32_t n);
