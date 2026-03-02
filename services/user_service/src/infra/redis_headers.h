#pragma once

#if __has_include(<hiredis/hiredis.h>)
#include <hiredis/hiredis.h>
#elif __has_include(<hiredis.h>)
#include <hiredis.h>
#else
#error "hiredis headers not found. Please install hiredis development headers."
#endif
