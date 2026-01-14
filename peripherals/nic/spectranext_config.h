#pragma once

#include <stdint.h>
#include "libspectrum.h"
#include "memory_pages.h"
#include "spectranext.h"

extern volatile struct spectranext_controller_registers_t spectranext_config;

extern void spectranext_config_init();

// Spectranext register access functions
libspectrum_byte spectranext_config_read( memory_page *page, libspectrum_word address );
void spectranext_config_write( memory_page *page, libspectrum_word address, libspectrum_byte b );
