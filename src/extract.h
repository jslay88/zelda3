#pragma once

// Build zelda3_assets.dat from a US ALttP ROM if it is missing.
// rom_override is an optional path from --rom; NULL searches cwd then a file prompt.
void Extract_EnsureAssets(const char *rom_override);
