#pragma once

#include "config.h"
#include "updater.h"

void RenderLauncherUI(LaunchConfig& config, bool& shouldLaunch, bool& shouldExit);
void RenderUpdateModal(const UpdateInfo& info, bool& show, bool& trigger_download);
void ApplyXMenTheme();
