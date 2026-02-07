#pragma once

#include "xfs.h"
#include "libspectrum.h"
#include "memory_pages.h"
#include "compat.h"

// XFS registers structure (volatile for memory-mapped access)
extern volatile struct xfs_registers_t xfs_registers;

// XFS base directory path (used by xfs_fs.c)
extern char xfs_base_path[];

void xfs_init();

// Fuse-specific memory access functions
libspectrum_byte xfs_read( memory_page *page GCC_UNUSED, libspectrum_word address );
void xfs_write( memory_page *page GCC_UNUSED, libspectrum_word address, libspectrum_byte b );

// Note: xfs_init(), xfs_reset(), xfs_debug_enable(), and xfs_debug_is_enabled()
// are declared in xfs.h
