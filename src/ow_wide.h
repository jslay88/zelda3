#pragma once

#include "types.h"

void OwWide_OnOverworldLoaded(void);
void OwWide_PrefetchCamera(void);
void OwWide_FillFrame(uint8 *pixel_buffer, size_t pitch, int height);
