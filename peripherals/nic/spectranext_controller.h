#pragma once

#include <stdint.h>
#include "libspectrum.h"
#include "memory_pages.h"
#include "spectranext.h"

extern volatile struct spectranext_controller_t spectranext_controller;

extern void spectranext_controller_init(void);

libspectrum_byte spectranext_controller_read(memory_page *page, libspectrum_word address);
void spectranext_controller_write(memory_page *page, libspectrum_word address, libspectrum_byte b);
