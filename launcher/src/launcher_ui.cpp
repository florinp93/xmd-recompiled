#include "launcher_ui.h"
#include "version.h"
#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <sstream>

static ImVec4 LerpImVec4(const ImVec4& a, const ImVec4& b, float t) {
    return ImVec4(
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t,
        a.w + (b.w - a.w) * t
    );
}

static ImU32 ColWithAlpha(const ImVec4& c, float a) {
    return ImGui::GetColorU32(ImVec4(c.x, c.y, c.z, c.w * a));
}

void ApplyXMenTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_WindowBg]        = ImVec4(0.025f, 0.025f, 0.03f, 1.0f);
    colors[ImGuiCol_ChildBg]         = ImVec4(0.045f, 0.045f, 0.05f, 1.0f);
    colors[ImGuiCol_PopupBg]         = ImVec4(0.06f, 0.06f, 0.07f, 0.98f);
    colors[ImGuiCol_Border]          = ImVec4(0.55f, 0.12f, 0.12f, 0.35f);
    colors[ImGuiCol_BorderShadow]    = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_Text]            = ImVec4(0.94f, 0.94f, 0.95f, 1.0f);
    colors[ImGuiCol_TextDisabled]    = ImVec4(0.45f, 0.45f, 0.46f, 1.0f);

    colors[ImGuiCol_FrameBg]         = ImVec4(0.10f, 0.10f, 0.12f, 1.0f);
    colors[ImGuiCol_FrameBgHovered]  = ImVec4(0.18f, 0.10f, 0.12f, 1.0f);
    colors[ImGuiCol_FrameBgActive]   = ImVec4(0.30f, 0.12f, 0.14f, 1.0f);

    colors[ImGuiCol_TitleBg]         = ImVec4(0.05f, 0.02f, 0.02f, 1.0f);
    colors[ImGuiCol_TitleBgActive]   = ImVec4(0.45f, 0.08f, 0.10f, 1.0f);
    colors[ImGuiCol_TitleBgCollapsed]= ImVec4(0.03f, 0.01f, 0.01f, 1.0f);

    colors[ImGuiCol_Button]          = ImVec4(0.65f, 0.08f, 0.10f, 1.0f);
    colors[ImGuiCol_ButtonHovered]   = ImVec4(0.85f, 0.12f, 0.14f, 1.0f);
    colors[ImGuiCol_ButtonActive]    = ImVec4(0.95f, 0.18f, 0.20f, 1.0f);

    colors[ImGuiCol_Header]          = ImVec4(0.50f, 0.10f, 0.10f, 0.6f);
    colors[ImGuiCol_HeaderHovered]   = ImVec4(0.70f, 0.16f, 0.16f, 0.7f);
    colors[ImGuiCol_HeaderActive]    = ImVec4(0.85f, 0.22f, 0.22f, 0.8f);

    colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.06f, 0.06f, 0.07f, 1.0f);
    colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.55f, 0.12f, 0.12f, 0.8f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.80f, 0.20f, 0.20f, 0.9f);
    colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.95f, 0.30f, 0.30f, 1.0f);

    colors[ImGuiCol_CheckMark]       = ImVec4(0.95f, 0.75f, 0.15f, 1.0f);
    colors[ImGuiCol_SliderGrab]      = ImVec4(0.75f, 0.14f, 0.16f, 1.0f);
    colors[ImGuiCol_SliderGrabActive]= ImVec4(0.90f, 0.20f, 0.22f, 1.0f);

    colors[ImGuiCol_Separator]       = ImVec4(0.55f, 0.12f, 0.12f, 0.3f);
    colors[ImGuiCol_SeparatorHovered]= ImVec4(0.70f, 0.16f, 0.16f, 0.5f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.85f, 0.22f, 0.22f, 1.0f);

    colors[ImGuiCol_Tab]             = ImVec4(0.08f, 0.05f, 0.05f, 1.0f);
    colors[ImGuiCol_TabHovered]      = ImVec4(0.60f, 0.10f, 0.12f, 0.9f);
    colors[ImGuiCol_TabSelected]     = ImVec4(0.70f, 0.12f, 0.14f, 1.0f);
    colors[ImGuiCol_TabDimmed]       = ImVec4(0.06f, 0.04f, 0.04f, 1.0f);
    colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.45f, 0.10f, 0.12f, 1.0f);

    style.WindowRounding    = 8.0f;
    style.ChildRounding     = 8.0f;
    style.FrameRounding     = 6.0f;
    style.GrabRounding      = 4.0f;
    style.TabRounding       = 6.0f;
    style.ButtonTextAlign   = ImVec2(0.5f, 0.5f);
    style.WindowBorderSize  = 1.0f;
    style.FrameBorderSize   = 0.0f;
    style.WindowPadding     = ImVec2(16, 16);
    style.FramePadding      = ImVec2(10, 6);
    style.ItemSpacing       = ImVec2(10, 8);
    style.GrabMinSize       = 16.0f;
    style.ScrollbarSize     = 12.0f;
    style.ScrollbarRounding = 6.0f;
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
    float t = (float)ImGui::GetTime();

    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Reserve space for two separators between title, content, and footer
    float footerH = 130.0f;
    float contentH = totalH - 90.0f - footerH - 32.0f;
    if (contentH < 240.0f) contentH = 240.0f;

    // --- Header / title bar ---
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::BeginChild("##header", ImVec2(totalW, 90), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);
    {
        ImVec2 h0 = ImGui::GetWindowPos();
        ImVec2 h1 = ImVec2(h0.x + totalW, h0.y + 90);

        // Animated gradient header
        float grad_t = (std::sin(t * 0.3f) + 1.0f) * 0.5f;
        ImU32 c0 = ImGui::GetColorU32(ImVec4(0.05f + 0.03f * grad_t, 0.01f, 0.01f, 1.0f));
        ImU32 c1 = ImGui::GetColorU32(ImVec4(0.14f + 0.06f * grad_t, 0.02f, 0.03f, 1.0f));
        dl->AddRectFilledMultiColor(h0, h1, c0, c1, c1, c0);

        // Animated top highlight line
        float line_w = 120.0f;
        float line_x = h0.x + (totalW * 0.5f) - (line_w * 0.5f) + (totalW * 0.3f) * std::sin(t * 0.4f);
        dl->AddRectFilled(ImVec2(line_x, h0.y), ImVec2(line_x + line_w, h0.y + 3),
            ColWithAlpha(ImVec4(0.95f, 0.30f, 0.30f, 1.0f), 0.8f), 2.0f);

        ImVec2 titlePos = ImVec2(h0.x + 26, h0.y + 20);
        dl->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 1.8f, titlePos,
            ImGui::GetColorU32(ImVec4(0.95f, 0.85f, 0.20f, 1.0f)), "X-MEN DESTINY");

        ImVec2 subPos = ImVec2(titlePos.x + 4, titlePos.y + 38);
        dl->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 1.05f, subPos,
            ImGui::GetColorU32(ImVec4(0.6f, 0.6f, 0.65f, 1.0f)), "PC Port");

        ImVec2 verPos = ImVec2(h1.x - 16 - ImGui::CalcTextSize("v" XMD_VERSION_STRING).x, h0.y + 60);
        dl->AddText(ImGui::GetFont(), ImGui::GetFontSize() * 0.95f, verPos,
            ImGui::GetColorU32(ImVec4(0.55f, 0.55f, 0.55f, 1.0f)), "v" XMD_VERSION_STRING);
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();

    ImGui::Separator();

    // --- Main content ---
    ImGui::BeginChild("##content", ImVec2(totalW, contentH), ImGuiChildFlags_None, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    {
        if (ImGui::BeginTabBar("##settings_tabs", ImGuiTabBarFlags_None)) {

            // === Graphics tab ===
            if (ImGui::BeginTabItem("Graphics")) {
                ImGui::Spacing();

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

                ImGui::Text("Anti-Aliasing");
                ImGui::Checkbox("2x MSAA", &config.native_2x_msaa);
                ImGui::SameLine(220);
                ImGui::Checkbox("Present Dither", &config.present_dither);
                ImGui::Spacing();

                ImGui::Text("Post-Processing");
                const char* effects[] = { "bilinear", "fxaa" };
                int effect_idx = (config.present_effect == "fxaa") ? 1 : 0;
                ImGui::Combo("Present Effect", &effect_idx, effects, 2);
                config.present_effect = effects[effect_idx];
                ImGui::Spacing();

                ImGui::Text("Anisotropic Filtering");
                const char* aniso_labels[] = { "Off", "2x", "4x", "8x", "16x" };
                int aniso_values[] = { 0, 2, 4, 8, 16 };
                int aniso_idx = 0;
                for (int i = 0; i < 5; i++) if (config.anisotropic_override == aniso_values[i]) aniso_idx = i;
                ImGui::Combo("Anisotropic", &aniso_idx, aniso_labels, 5);
                config.anisotropic_override = aniso_values[aniso_idx];
                ImGui::Spacing();

                ImGui::Text("Display");
                ImGui::Checkbox("Fullscreen", &config.fullscreen);
                ImGui::SameLine(220);
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
    }
    ImGui::EndChild();

    ImGui::Separator();

    // --- Footer / Play + Exit ---
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::BeginChild("##footer", ImVec2(totalW, footerH), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);
    {
        float availW = ImGui::GetContentRegionAvail().x;

        // Exit button (top-right of footer, vertically centered)
        const ImVec2 exitSize = ImVec2(90, 36);
        ImGui::SetCursorPosX(availW - exitSize.x - 20);
        ImGui::SetCursorPosY((footerH - exitSize.y) * 0.5f);
        if (ImGui::Button("Exit", exitSize)) {
            shouldExit = true;
        }

        // Play button (centered, large)
        const ImVec2 playSize = ImVec2(280, 64);
        ImGui::SetCursorPosX((availW - playSize.x) * 0.5f);
        ImGui::SetCursorPosY((footerH - playSize.y) * 0.5f);

        ImVec2 p0 = ImGui::GetCursorScreenPos();
        ImVec2 p1 = ImVec2(p0.x + playSize.x, p0.y + playSize.y);

        bool hover = ImGui::IsMouseHoveringRect(p0, p1);
        float pulse = (std::sin(t * 2.2f) + 1.0f) * 0.5f;
        float glowAlpha = 0.25f + 0.20f * pulse + (hover ? 0.20f : 0.0f);
        ImVec4 glowCol = hover ? ImVec4(1.0f, 0.35f, 0.35f, glowAlpha)
                                : ImVec4(0.95f, 0.20f, 0.20f, glowAlpha);

        ImVec2 gp0 = ImVec2(p0.x - 6, p0.y - 6);
        ImVec2 gp1 = ImVec2(p1.x + 6, p1.y + 6);
        dl->AddRectFilled(gp0, gp1, ColWithAlpha(glowCol, glowCol.w), 14.0f);

        // Active (mouse-down) inner flash
        if (ImGui::IsMouseDown(0) && hover) {
            dl->AddRectFilled(ImVec2(p0.x - 2, p0.y - 2), ImVec2(p1.x + 2, p1.y + 2),
                ColWithAlpha(ImVec4(1.0f, 0.6f, 0.6f, 0.45f), 1.0f), 14.0f);
        }

        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.68f, 0.08f, 0.10f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.90f, 0.14f, 0.16f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(1.00f, 0.22f, 0.24f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::SetWindowFontScale(1.5f);
        if (ImGui::Button("PLAY", playSize)) {
            shouldLaunch = true;
        }
        ImGui::SetWindowFontScale(1.0f);
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();

    ImGui::End();
}

void RenderUpdateModal(const UpdateInfo& info, bool& show, bool& trigger_download) {
    if (!show) return;

    ImGui::OpenPopup("Update Available");
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(560, 420), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("Update Available", &show,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {

        ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.2f, 1.0f), "A new version is available:");
        ImGui::Text("%s", info.version.c_str());
        ImGui::Separator();

        ImGui::TextWrapped("Changelog:");
        ImGui::Spacing();
        ImGui::BeginChild("##changelog", ImVec2(0, 240), ImGuiChildFlags_None, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        std::string line;
        std::istringstream ss(info.changelog);
        while (std::getline(ss, line)) {
            if (line.empty()) {
                ImGui::Spacing();
            } else {
                ImGui::TextWrapped("%s", line.c_str());
            }
        }
        ImGui::EndChild();

        ImGui::Spacing();
        ImVec2 btnSize(120, 36);
        float w = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX((w - btnSize.x * 2 - 20) * 0.5f);
        if (ImGui::Button("Update Now", btnSize)) {
            trigger_download = true;
            show = false;
        }
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20);
        if (ImGui::Button("Later", btnSize)) {
            show = false;
        }

        ImGui::EndPopup();
    }
}
