#pragma once

#include "types.h"

enum {
  kSettingsFrom_FileSelect = 0,
  kSettingsFrom_Pause = 1,
};

extern int g_dev_force_overlay;
extern bool g_dev_skip_live_tileattr;
extern bool g_dev_skip_fill;

void SettingsMenu_Open(int from);
void SettingsMenu_Close(void);
bool SettingsMenu_IsOpen(void);
void SettingsMenu_Run(void);
void SettingsPauseChooser_Reset(void);
void SettingsPauseChooser_Run(void);
bool SettingsPauseChooser_IsActive(void);
void SettingsMenu_Draw(uint8 *pixel_buffer, size_t pitch, int height);

void WriteUserConfigFile(void);
void Settings_ApplyLive(void);
void Settings_ApplyVideo(void);
void Settings_ApplyAudio(void);
void Settings_PollDeferred(void);
