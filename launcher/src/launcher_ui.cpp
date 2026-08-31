#include "launcher_ui.h"
#include <imgui.h>
#include <algorithm>
#include <cmath>

static ImVec4 LerpImVec4(const ImVec4& a, const ImVec4& b, float t) {
    return ImVec4(
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t,
        a.w + (b.w - a.w) * t
    );
}

void ApplyXMenTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_WindowBg]        = ImVec4(0.04f, 0.04f, 0.05f, 1.0f);
    colors[ImGuiCol_ChildBg]         = ImVec4(0.06f, 0.06f, 0.07f, 1.0f);
    colors[ImGuiCol_PopupBg]         = ImVec4(0.06f, 0.06f, 0.07f, 0.98f);
    colors[ImGuiCol_Border]          = ImVec4(0.55f, 0.12f, 0.12f, 0.5f);
    colors[ImGuiCol_BorderShadow]    = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_Text]            = ImVec4(0.92f, 0.92f, 0.93f, 1.0f);
    colors[ImGuiCol_TextDisabled]    = ImVec4(0.45f, 0.45f, 0.46f, 1.0f);

    colors[ImGuiCol_FrameBg]         = ImVec4(0.10f, 0.10f, 0.11f, 1.0f);
    colors[ImGuiCol_FrameBgHovered]  = ImVec4(0.18f, 0.10f, 0.10f, 1.0f);
    colors[ImGuiCol_FrameBgActive]   = ImVec4(0.28f, 0.12f, 0.12f, 1.0f);

    colors[ImGuiCol_TitleBg]         = ImVec4(0.08f, 0.04f, 0.04f, 1.0f);
    colors[ImGuiCol_TitleBgActive]   = ImVec4(0.50f, 0.10f, 0.10f, 1.0f);
    colors[ImGuiCol_TitleBgCollapsed]= ImVec4(0.05f, 0.03f, 0.03f, 1.0f);

    colors[ImGuiCol_Button]          = ImVec4(0.50f, 0.10f, 0.10f, 1.0f);
    colors[ImGuiCol_ButtonHovered]   = ImVec4(0.70f, 0.16f, 0.16f, 1.0f);
    colors[ImGuiCol_ButtonActive]    = ImVec4(0.85f, 0.22f, 0.22f, 1.0f);

    colors[ImGuiCol_Header]          = ImVec4(0.50f, 0.10f, 0.10f, 0.6f);
    colors[ImGuiCol_HeaderHovered]   = ImVec4(0.70f, 0.16f, 0.16f, 0.7f);
    colors[ImGuiCol_HeaderActive]    = ImVec4(0.85f, 0.22f, 0.22f, 0.8f);

    colors[ImGuiCol_CheckMark]       = ImVec4(0.90f, 0.75f, 0.15f, 1.0f);
    colors[ImGuiCol_SliderGrab]      = ImVec4(0.60f, 0.14f, 0.14f, 1.0f);
    colors[ImGuiCol_SliderGrabActive]= ImVec4(0.80f, 0.20f, 0.20f, 1.0f);

    colors[ImGuiCol_Separator]       = ImVec4(0.55f, 0.12f, 0.12f, 0.4f);
    colors[ImGuiCol_SeparatorHovered]= ImVec4(0.70f, 0.16f, 0.16f, 0.6f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.85f, 0.22f, 0.22f, 1.0f);

    colors[ImGuiCol_Tab]             = ImVec4(0.10f, 0.06f, 0.06f, 1.0f);
    colors[ImGuiCol_TabHovered]      = ImVec4(0.50f, 0.10f, 0.10f, 0.8f);
    colors[ImGuiCol_TabSelected]     = ImVec4(0.50f, 0.10f, 0.10f, 1.0f);
    colors[ImGuiCol_TabDimmed]       = ImVec4(0.08f, 0.05f, 0.05f, 1.0f);
    colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.40f, 0.08f, 0.08f, 1.0f);

    style.WindowRounding    = 6.0f;
    style.ChildRounding     = 4.0f;
    style.FrameRounding     = 3.0f;
    style.GrabRounding      = 3.0f;
    style.TabRounding       = 4.0f;
    style.WindowBorderSize  = 1.0f;
    style.FrameBorderSize   = 0.0f;
    style.WindowPadding     = ImVec2(14, 14);
    style.FramePadding      = ImVec2(8, 4);
    style.ItemSpacing       = ImVec2(8, 6);
}

static void TabBar() {
    if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_None)) {
        ImGui::EndTabBar();
    }
}

void RenderLauncherUI(LaunchConfig& config, bool& shouldLaunch, bool& shouldExit) {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->Pos);
    ImGui::SetNextWindowSize(vp->Size);
    ImGui::Begin("##launcher", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    float totalW = vp->Size.x;
    float totalH = vp->Size.y;

    // --- Title bar ---
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.03f, 0.03f, 1.0f));
    ImGui::BeginChild("##titlebar", ImVec2(totalW, 60), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);
    ImGui::SetWindowFontScale(1.6f);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.75f, 0.15f, 1.0f));
    ImGui::Text("X-MEN DESTINY");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Text(" PC Port");
    ImGui::PopStyleColor();
    ImGui::SetWindowFontScale(1.0f);
    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::Separator();

    // --- Main content area ---
    float contentH = totalH - 60 - 70;
    ImGui::BeginChild("##content", ImVec2(totalW, contentH), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);

    if (ImGui::BeginTabBar("##settings_tabs", ImGuiTabBarFlags_None)) {

        // === Graphics tab ===
        if (ImGui::BeginTabItem("Graphics")) {
            ImGui::Spacing();

            // Resolution scale
            ImGui::Text("Render Resolution Scale");
            const char* scale_labels[] = { "1x (Native)", "2x", "3x", "4x" };
            int scale_values[] = { 1, 2, 3, 4 };
            int cur_idx = 0;
            for (int i = 0; i < 4; i++) if (config.resolution_scale == scale_values[i]) cur_idx = i;
            for (int i = 0; i < 4; i++) {
                if (i > 0) ImGui::SameLine();
                std::string label = std::string(scale_labels[i]) + " (" + ScaleResolution(scale_values[i]) + ")";
                if (ImGui::RadioButton(label.c_str(), &cur_idx, i)) {
                    config.resolution_scale = scale_values[cur_idx];
                }
            }
            ImGui::Spacing();

            // Anti-aliasing
            ImGui::Text("Anti-Aliasing");
            ImGui::Checkbox("2x MSAA", &config.native_2x_msaa);
            ImGui::SameLine(200);
            ImGui::Checkbox("Present Dither", &config.present_dither);
            ImGui::Spacing();

            // Present effect
            ImGui::Text("Post-Processing");
            const char* effects[] = { "bilinear", "fxaa" };
            int effect_idx = (config.present_effect == "fxaa") ? 1 : 0;
            ImGui::Combo("Present Effect", &effect_idx, effects, 2);
            config.present_effect = effects[effect_idx];
            ImGui::Spacing();

            // Anisotropic
            ImGui::Text("Anisotropic Filtering");
            const char* aniso_labels[] = { "Off", "2x", "4x", "8x", "16x" };
            int aniso_values[] = { 0, 2, 4, 8, 16 };
            int aniso_idx = 0;
            for (int i = 0; i < 5; i++) if (config.anisotropic_override == aniso_values[i]) aniso_idx = i;
            ImGui::Combo("Anisotropic", &aniso_idx, aniso_labels, 5);
            config.anisotropic_override = aniso_values[aniso_idx];
            ImGui::Spacing();

            // Display
            ImGui::Text("Display");
            ImGui::Checkbox("Fullscreen", &config.fullscreen);
            ImGui::SameLine(200);
            ImGui::Checkbox("VSync", &config.vsync);
            ImGui::Spacing();

            ImGui::Text("Monitor: %s", GetMonitorResolution().c_str());

            ImGui::EndTabItem();
        }

        // === Input tab ===
        if (ImGui::BeginTabItem("Input")) {
            ImGui::Spacing();
            ImGui::TextWrapped("Controllers are auto-detected via SDL3. DualSense, DualShock, Xbox, and generic gamepads are all supported.");
            ImGui::Spacing();
            ImGui::Text("Keyboard & Mouse is always enabled as a virtual controller.");
            ImGui::Spacing();
            ImGui::TextWrapped("Controls:\n"
                "  WASD       - Movement\n"
                "  Mouse      - Camera / Right stick\n"
                "  Space      - Jump / Confirm\n"
                "  F          - Heavy attack / Cancel\n"
                "  Mouse Left - Light attack\n"
                "  E          - Grab / Context\n"
                "  Q          - Block / Parry\n"
                "  Shift      - Modifier / D-Pad\n"
                "  Escape     - Pause\n"
                "  Tab        - Menu");
            ImGui::EndTabItem();
        }

        // === Advanced tab ===
        if (ImGui::BeginTabItem("Advanced")) {
            ImGui::Spacing();

            ImGui::Text("Clock Scaling");
            ImGui::Checkbox("No Clock Scaling (real-time)", &config.clock_no_scaling);
            ImGui::Spacing();

            ImGui::Text("Logging");
            const char* log_levels[] = { "error", "warn", "info", "debug", "trace" };
            int log_idx = 2;
            for (int i = 0; i < 5; i++) if (config.log_level == log_levels[i]) log_idx = i;
            ImGui::Combo("Log Level", &log_idx, log_levels, 5);
            config.log_level = log_levels[log_idx];
            ImGui::Spacing();

            ImGui::Checkbox("Debug Overlay (F3)", &config.debug_overlay);
            ImGui::Spacing();

            ImGui::TextWrapped("Log file: %s", config.log_file.c_str());

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::EndChild();

    ImGui::Separator();

    // --- Bottom bar with Play button ---
    ImGui::BeginChild("##bottombar", ImVec2(totalW, 60), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);

    float btnW = 180;
    float btnH = 40;
    ImGui::SetCursorPosX((totalW - btnW) * 0.5f);
    ImGui::SetCursorPosY(10);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.70f, 0.14f, 0.14f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.22f, 0.22f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.95f, 0.30f, 0.30f, 1.0f));
    ImGui::PushFont(ImGui::GetFont());
    ImGui::SetWindowFontScale(1.3f);
    if (ImGui::Button("PLAY", ImVec2(btnW, btnH))) {
        shouldLaunch = true;
    }
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    ImGui::SetCursorPosX(totalW - 100);
    ImGui::SetCursorPosY(15);
    if (ImGui::Button("Exit", ImVec2(80, 30))) {
        shouldExit = true;
    }

    ImGui::EndChild();

    ImGui::End();
}
