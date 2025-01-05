#include "CGui.h"
#include "CDirectX.h"
#include "Globals.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "addons/imgui_addons.h"
#include "misc/freetype/imgui_freetype.h"

#include "../../resources/fonts/font_pixeltype.h"
#include "../../resources/fonts/font_cascadia_mono_pl_regular.h"

#include <iostream>
#include <iomanip>
#include <ctime>

CGui::CGui()
{
    m_hWnd              = 0;
    m_pDevice           = 0;
    m_pDeviceContext    = 0;
    m_pTextEditor       = 0;

    m_iCurrentPage      = 0;

    m_Tabs.push_back("Aimbot");
    m_Tabs.push_back("Visuals");
    m_Tabs.push_back("Lua");
    m_Tabs.push_back("Misc");
    m_Tabs.push_back("Configs");
    m_Tabs.push_back("Settings");

    memset(m_bSelectedBones, false, sizeof m_bSelectedBones);
}

CGui::~CGui()
{
}

bool CGui::Initialize(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* device_context)
{
    if (!hwnd)
        return false;
    
    if (!device)
        return false;
    
    if (!device_context)
        return false;

    m_hWnd = hwnd;
    m_pDevice = device;
    m_pDeviceContext = device_context;
    
    bool result = true;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    io.IniFilename = nullptr;               // Disable INI File  
    GImGui->NavDisableHighlight = true;     // Disable Highlighting

    if (!GImGui->NavDisableHighlight)
    {
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;    // Enable Gamepad Controls
    }

    // Setup Dear ImGui default style
    ImGui::StyleColorsDark();

    // Custom Styles
    style.WindowRounding    = 3;
    style.ChildRounding     = 2;
    style.FrameRounding     = 0;
    style.PopupRounding     = 0;
    style.ScrollbarRounding = 0;

    style.ButtonTextAlign   = { 0.5f, 0.5f };
    style.WindowTitleAlign  = { 0.5f, 0.5f };
    style.FramePadding      = { 6.0f, 6.0f };
    style.WindowPadding     = { 10.0f, 10.0f };
    style.ItemSpacing       = { 10.0f, 6.0f };
    style.ItemInnerSpacing  = { style.WindowPadding.x, 2 };

    style.WindowBorderSize  = 1;
    style.FrameBorderSize   = 1;

    style.ScrollbarSize     = 7.f;
    style.GrabMinSize       = 1.f;
    style.DisabledAlpha     = 0.5f;

    style.WindowMinSize     = ImVec2(400, 450);
    
    // Custom Colors
    style.Colors[ImGuiCol_WindowBg]             = ImAdd::HexToColorVec4(0x0c0c0c, 1.00f);
    style.Colors[ImGuiCol_ChildBg]              = ImAdd::HexToColorVec4(0x121212, 1.00f);
    style.Colors[ImGuiCol_PopupBg]              = ImAdd::HexToColorVec4(0x0d0d0d, 1.00f);   
    
    style.Colors[ImGuiCol_Text]                 = ImAdd::HexToColorVec4(0xb4b4b4, 1.00f);
    style.Colors[ImGuiCol_TextDisabled]         = ImAdd::HexToColorVec4(0x8c8c8c, 1.00f);
    
    style.Colors[ImGuiCol_SliderGrab]           = ImAdd::HexToColorVec4(0x5e7aa6, 1.00f);
    style.Colors[ImGuiCol_SliderGrabActive]     = ImAdd::HexToColorVec4(0x3e5780, 1.00f);
    
    style.Colors[ImGuiCol_SeparatorHovered]     = style.Colors[ImGuiCol_SliderGrab];
    style.Colors[ImGuiCol_SeparatorActive]      = style.Colors[ImGuiCol_SliderGrabActive];

    style.Colors[ImGuiCol_BorderShadow]         = ImAdd::HexToColorVec4(0x000000, 1.00f);
    style.Colors[ImGuiCol_Border]               = ImAdd::HexToColorVec4(0x333333, 1.00f);
    style.Colors[ImGuiCol_Separator]            = style.Colors[ImGuiCol_Border];
    
    style.Colors[ImGuiCol_Button]               = ImAdd::HexToColorVec4(0x2a2a2a, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered]        = ImAdd::HexToColorVec4(0x3f3f3f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive]         = ImAdd::HexToColorVec4(0x191919, 1.00f);
    
    style.Colors[ImGuiCol_FrameBg]              = style.Colors[ImGuiCol_Button];
    style.Colors[ImGuiCol_FrameBgHovered]       = style.Colors[ImGuiCol_ButtonHovered];
    style.Colors[ImGuiCol_FrameBgActive]        = style.Colors[ImGuiCol_ButtonActive];
    
    style.Colors[ImGuiCol_Header]               = ImAdd::HexToColorVec4(0x2a2a2a, 1.00f);
    style.Colors[ImGuiCol_HeaderHovered]        = ImAdd::HexToColorVec4(0x3f3f3f, 1.00f);
    style.Colors[ImGuiCol_HeaderActive]         = ImAdd::HexToColorVec4(0x191919, 1.00f);

    style.Colors[ImGuiCol_ScrollbarBg]          = style.Colors[ImGuiCol_Border];
    style.Colors[ImGuiCol_ScrollbarGrab]        = ImAdd::HexToColorVec4(0x414141, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = style.Colors[ImGuiCol_ScrollbarGrab];
    style.Colors[ImGuiCol_ScrollbarGrabActive]  = style.Colors[ImGuiCol_ScrollbarGrab];
    
    style.Colors[ImGuiCol_ResizeGrip]           = ImVec4();
    style.Colors[ImGuiCol_ResizeGripHovered]    = ImVec4();
    style.Colors[ImGuiCol_ResizeGripActive]     = ImVec4();
    style.Colors[ImGuiCol_SeparatorActive]      = ImVec4();
    style.Colors[ImGuiCol_SeparatorHovered]     = ImVec4();

    // Setup Font
    ImFontConfig cfg;
    //cfg.OversampleH = 1;
    //cfg.OversampleV = 1;
    cfg.FontBuilderFlags = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_ForceAutoHint;
    cfg.GlyphOffset = ImVec2(0, 1);
    io.Fonts->AddFontFromMemoryCompressedTTF(font_pixeltype_compressed_data, font_pixeltype_compressed_size, 11, &cfg, io.Fonts->GetGlyphRangesDefault());

    ImFontConfig cfg_code;
    cfg_code.OversampleH = 1;
    cfg_code.OversampleV = 1;
    cfg_code.FontBuilderFlags = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_ForceAutoHint;

    io.Fonts->AddFontFromMemoryCompressedTTF(font_cascadia_mono_pl_regular_compressed_data, font_cascadia_mono_pl_regular_compressed_size, 12, &cfg_code, io.Fonts->GetGlyphRangesDefault());

    // Setup Platform/Renderer backends
    result &= ImGui_ImplWin32_Init(hwnd);
    result &= ImGui_ImplDX11_Init(device, device_context);

    m_pTextEditor = new CTextEditor();
    m_pTextEditor->SetLanguageDefinition(CTextEditor::LanguageDefinitionId::Lua);
    m_pTextEditor->SetPalette(CTextEditor::PaletteId::Dark);

	return result;
}

void CGui::Shutdown()
{
    if (m_pTextEditor != nullptr) {
        delete m_pTextEditor;
        m_pTextEditor = 0;
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void CGui::Render(const char* title, POINTS size, bool* b_open)
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImVec2 MenuSize = ImVec2(590, 455);

    float HeaderHeight = ImGui::GetFontSize() + style.WindowPadding.y * 2;
    float SideBarWidth = 120;

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    {
        ImGui::SetNextWindowPos(io.DisplaySize / 2 - MenuSize / 2, ImGuiCond_Once);
        ImGui::SetNextWindowSize(MenuSize, ImGuiCond_Once);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
        ImGui::Begin(title, b_open, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground);
        ImGui::PopStyleVar(2);
        // Rendering
        {
            ImVec2 pos = ImGui::GetWindowPos();
            ImVec2 size = ImGui::GetWindowSize();
            ImDrawList* pDrawList = ImGui::GetWindowDrawList();

            float fGlowAlpha = 0.14f;

            pDrawList->AddRectFilled(pos, pos + ImVec2(size.x, HeaderHeight), ImGui::GetColorU32(ImGuiCol_ChildBg), style.WindowRounding, ImDrawFlags_RoundCornersTop);
            pDrawList->AddRectFilled(pos + ImVec2(0, HeaderHeight), pos + size, ImGui::GetColorU32(ImGuiCol_WindowBg), style.WindowRounding, ImDrawFlags_RoundCornersBottomRight);
            pDrawList->AddRectFilled(pos + ImVec2(0, HeaderHeight), pos + ImVec2(SideBarWidth, size.y), ImGui::GetColorU32(ImGuiCol_ChildBg), style.WindowRounding, ImDrawFlags_RoundCornersBottomLeft);

            pDrawList->AddRectFilledMultiColor(pos + ImVec2(style.WindowBorderSize, style.WindowBorderSize + style.WindowRounding), pos + ImVec2(size.x - style.WindowBorderSize, HeaderHeight), ImGui::GetColorU32(ImGuiCol_SliderGrab, 0.0f), ImGui::GetColorU32(ImGuiCol_SliderGrab, 0.0f), ImGui::GetColorU32(ImGuiCol_SliderGrab, fGlowAlpha), ImGui::GetColorU32(ImGuiCol_SliderGrab, fGlowAlpha));

            if (style.WindowBorderSize > 0)
            {
                pDrawList->AddRect(pos, pos + size, ImGui::GetColorU32(ImGuiCol_Border), style.WindowRounding);
                pDrawList->AddLine(pos + ImVec2(style.WindowBorderSize, HeaderHeight), pos + ImVec2(size.x - style.WindowBorderSize, HeaderHeight), ImGui::GetColorU32(ImGuiCol_SliderGrab));
                pDrawList->AddLine(pos + ImVec2(SideBarWidth, HeaderHeight + style.WindowBorderSize), pos + ImVec2(SideBarWidth, size.y - style.WindowBorderSize), ImGui::GetColorU32(ImGuiCol_Border), style.WindowBorderSize);
            }

            pDrawList->AddText(pos + style.WindowPadding, ImGui::GetColorU32(ImGuiCol_Text), "imgui");
            pDrawList->AddText(pos + style.WindowPadding + ImVec2(ImGui::CalcTextSize("imgui").x + 1, 0), ImGui::GetColorU32(ImGuiCol_SliderGrab), ".space");
        }
        // Content
        {
            ImGui::SetCursorScreenPos(ImGui::GetWindowPos() + ImVec2(0, HeaderHeight));
            ImGui::BeginChild("SideBar", ImVec2(SideBarWidth, 0), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoBackground);
            {
                for (int i = 0; i < m_Tabs.size(); i++)
                {
                    ImAdd::RadioFrameGradient(m_Tabs[i], &m_iCurrentPage, i, ImVec2(-0.1f, 0));
                }
            }
            ImGui::EndChild();

            ImGui::SetCursorScreenPos(ImGui::GetWindowPos() + ImVec2(SideBarWidth + style.WindowBorderSize, HeaderHeight));
            ImGui::BeginChild("Main", ImVec2(0, 0), ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_NoBackground);
            {
                float fGroupWidth = (ImGui::GetWindowWidth() - style.ItemSpacing.x - style.WindowPadding.x * 2) / 2;

                if (m_iCurrentPage == ImPage_Aimbot)
                {
                    ImGui::BeginGroup(); // left groups
                    {
                        ImAdd::BeginChild("General",
                            ImVec2(fGroupWidth,
                                ImGui::GetFontSize() * 4.5f + // 4 checkbox height = to font size + child title height (same as text/font size) / 2
                                style.ItemSpacing.y * 3 + // spacing between each item
                                style.WindowPadding.y * 2 // window padding
                            )
                        );
                        {
                            static int iAimlockKey = VK_F1;
                            static int iSilentKey = VK_F2;
                            static int iBacktrackKey = VK_F3;
                            static int iTriggetbotKey = VK_F4;

                            static bool bAimlock = false;
                            static bool bSilent = false;
                            static bool bBacktrack = false;
                            static bool bTriggerbot = false;

                            ImAdd::CheckBox("AimBot", &bAimlock);
                            ImGui::SameLine(ImGui::GetWindowWidth() - style.WindowPadding.x - ImGui::CalcTextSize(szKeyNames[iAimlockKey]).x);
                            ImAdd::KeyBind("Aimbot", &iAimlockKey, ImGui::CalcTextSize(szKeyNames[iAimlockKey]).x);

                           // ImAdd::CheckBox("Silent", &bSilent);
                           // ImGui::SameLine(ImGui::GetWindowWidth() - style.WindowPadding.x - ImGui::CalcTextSize(szKeyNames[iSilentKey]).x);
                           // ImAdd::KeyBind("SilentKey", &iSilentKey, ImGui::CalcTextSize(szKeyNames[iSilentKey]).x);

                            //ImAdd::CheckBox("Backtrack", &bBacktrack);
                       //     ImGui::SameLine(ImGui::GetWindowWidth() - style.WindowPadding.x - ImGui::CalcTextSize(szKeyNames[iBacktrackKey]).x);
                         //  ImAdd::KeyBind("BacktrqckKey", &iBacktrackKey, ImGui::CalcTextSize(szKeyNames[iBacktrackKey]).x);
                            
                            ImAdd::CheckBox("Triggerbot", &bTriggerbot);
                            ImGui::SameLine(ImGui::GetWindowWidth() - style.WindowPadding.x - ImGui::CalcTextSize(szKeyNames[iTriggetbotKey]).x);
                            ImAdd::KeyBind("TriggetbotKey", &iTriggetbotKey, ImGui::CalcTextSize(szKeyNames[iTriggetbotKey]).x);
                        }
                        ImAdd::EndChild();

                        ImAdd::BeginChild("Filters", ImVec2(fGroupWidth, ImGui::GetFontSize() * 5.5f + style.ItemSpacing.y * 4 + style.WindowPadding.y * 2));
                        {
                            static bool bIgnoreTeammates = false;
                            static bool bIgnoreWalls = false;
                            static bool bIgnoreProps = false;
                            static bool bIgnoreVehicles = false;
                            static bool bIgnoreNPCs = false;
                            static bool bIgnoreDeads = false;

                            ImAdd::CheckBox("Ignore Teammates", &bIgnoreTeammates);
                            ImAdd::CheckBox("Ignore Walls", &bIgnoreWalls);
                            ImAdd::CheckBox("Ignore Props", &bIgnoreProps);
                            ImAdd::CheckBox("Ignore Vehicles", &bIgnoreVehicles);
                            ImAdd::CheckBox("Ignore Deads", &bIgnoreNPCs);
                        }
                        ImAdd::EndChild();

                        ImAdd::BeginChild("Configs", ImVec2(fGroupWidth, ImGui::GetFontSize() * 8.5f + style.ItemSpacing.y * 3 + style.ItemInnerSpacing.y * 4 + style.WindowPadding.y * 2));
                        {
                            static int iFOV = 90;
                            static int iSmoothness = 5;
                            static int iAimbotHitchance = 10;
                            static int iSilentHitchance = 2;

                            ImGui::PushItemWidth(ImGui::GetWindowWidth() - style.WindowPadding.x * 2);
                            {
                                ImAdd::SliderInt("Field of View", &iFOV, 1, 360);
                                ImAdd::SliderInt("Smoothness", &iSmoothness, 0, 100, "%d px/ms"); // pixel per ms ?
                                ImAdd::SliderInt("Aimlock Hitchance", &iAimbotHitchance, 0, 100, "%d%%");
                                ImAdd::SliderInt("prediction", &iSilentHitchance, 0, 100, "%d%%");
                            }
                            ImGui::PopItemWidth();
                        }
                        ImAdd::EndChild();
                    }
                    ImGui::EndGroup();
                    ImGui::SameLine();
                    ImGui::BeginGroup(); // right groups
                    {
                        ImAdd::BeginChild("Overlay", ImVec2(fGroupWidth, ImGui::GetFontSize() * 3.5f + style.ItemSpacing.y * 2 + style.WindowPadding.y * 2));
                        {
                            static bool bDrawFOVCircle = false;
                            static bool bDrawTargetLine = false;
                            static bool bDrawRecoilDots = false;

                            static ImVec4 v4FOVCircleBorder = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                            static ImVec4 v4FOVCircleFilled = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                            static ImVec4 v4TargetLine = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                            static ImVec4 v4RecoilDots = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

                            ImAdd::CheckBox("Draw FOV Circle", &bDrawFOVCircle);
                            ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::GetFontSize() * 4 - style.ItemSpacing.y - style.WindowPadding.x);
                            ImAdd::ColorEdit4("##FOV Circle Border", (float*)&v4FOVCircleBorder);
                            ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::GetFontSize() * 2 - style.WindowPadding.x);
                            ImAdd::ColorEdit4("##FOV Circle Filled", (float*)&v4FOVCircleFilled);

                            ImAdd::CheckBox("Draw Target Line", &bDrawTargetLine);
                            ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::GetFontSize() * 2 - style.WindowPadding.x);
                            ImAdd::ColorEdit4("##Target Line", (float*)&v4TargetLine);

                            ImAdd::CheckBox("Draw Recoil Dots", &bDrawRecoilDots);
                            ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::GetFontSize() * 2 - style.WindowPadding.x);
                            ImAdd::ColorEdit4("##Recoil Dots", (float*)&v4RecoilDots);
                        }
                        ImAdd::EndChild();

                        ImAdd::BeginChild("Target Bones", ImVec2(0, 0));
                        {
                            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
                            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
                            ImGui::BeginChild("InternalFrame", ImVec2(), ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                            {
                                float ArmLeg_Width = (float)(int)(ImGui::GetWindowWidth() / 6);
                                float ArmLeg_Height = (float)(int)(ArmLeg_Width * 2.5f);
                                float Head_Size = (float)(int)((ArmLeg_Width / 3) * 4);

                                ImGui::SetCursorScreenPos(ImGui::GetWindowPos() + ImVec2(0, ImGui::GetWindowHeight() / 2 - (HeaderHeight + ArmLeg_Height * 2) / 2));
                                ImGui::BeginGroup();
                                {
                                    ImGui::SetCursorPosX((float)(int)(ImGui::GetWindowWidth() / 2 - Head_Size / 2));
                                    if (ImAdd::SelectableFrame(":)Head", m_bSelectedBones[Bone_Head], ImVec2(Head_Size, Head_Size)))
                                    {
                                        m_bSelectedBones[Bone_Head] = !m_bSelectedBones[Bone_Head];
                                    }

                                    ImGui::SetCursorPosX((float)(int)(ImGui::GetWindowWidth() / 2 - ArmLeg_Width * 2));
                                    ImGui::BeginGroup();
                                    {
                                        if (ImAdd::SelectableFrame("LeftArm", m_bSelectedBones[Bone_LeftArm], ImVec2(ArmLeg_Width, ArmLeg_Height)))
                                        {
                                            m_bSelectedBones[Bone_LeftArm] = !m_bSelectedBones[Bone_LeftArm];
                                        }
                                        ImGui::SameLine();
                                        if (ImAdd::SelectableFrame("Belly", m_bSelectedBones[Bone_Belly], ImVec2(ArmLeg_Width * 2, ArmLeg_Height)))
                                        {
                                            m_bSelectedBones[Bone_Belly] = !m_bSelectedBones[Bone_Belly];
                                        }
                                        ImGui::SameLine();
                                        if (ImAdd::SelectableFrame("RightArm", m_bSelectedBones[Bone_RightArm], ImVec2(ArmLeg_Width, ArmLeg_Height)))
                                        {
                                            m_bSelectedBones[Bone_RightArm] = !m_bSelectedBones[Bone_RightArm];
                                        }
                                    }
                                    ImGui::EndGroup();

                                    ImGui::SetCursorPosX((float)(int)(ImGui::GetWindowWidth() / 2 - ArmLeg_Width));
                                    ImGui::BeginGroup();
                                    {
                                        if (ImAdd::SelectableFrame("LeftLeg", m_bSelectedBones[Bone_LeftLeg], ImVec2(ArmLeg_Width, ArmLeg_Height)))
                                        {
                                            m_bSelectedBones[Bone_LeftLeg] = !m_bSelectedBones[Bone_LeftLeg];
                                        }
                                        ImGui::SameLine();
                                        if (ImAdd::SelectableFrame("RightLeg", m_bSelectedBones[Bone_RightLeg], ImVec2(ArmLeg_Width, ArmLeg_Height)))
                                        {
                                            m_bSelectedBones[Bone_RightLeg] = !m_bSelectedBones[Bone_RightLeg];
                                        }
                                    }
                                    ImGui::EndGroup();
                                }
                                ImGui::EndGroup();
                            }
                            ImGui::PopStyleVar(2);
                        }
                        ImAdd::EndChild();
                    }
                    ImGui::EndGroup();
                }
                else if (m_iCurrentPage == ImPage_Lua)
                {
                    ImGui::BeginChild("CodeFrame", ImVec2(0, ImGui::GetWindowHeight() - style.WindowPadding.x * 2 - style.ItemSpacing.y - ImGui::GetFrameHeight()), ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground);
                    ImGui::PopStyleColor();
                    {
                        ImGui::PushFont(io.Fonts->Fonts[ImFont_Code]);
                        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4());
                        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, style.FramePadding.y));
                        m_pTextEditor->Render("CodeEditor", false, ImVec2(0, 0), true);
                        ImGui::PopStyleVar();
                        ImGui::PopStyleColor();
                        ImGui::PopFont();
                    }
                    ImGui::EndChild();

                    ImGui::PushStyleColor(ImGuiCol_BorderShadow, style.Colors[ImGuiCol_Border]);
                    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4());
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, style.ChildRounding);
                    {
                        ImAdd::Button("Clear");
                        ImGui::SameLine();
                        ImAdd::Button("Execute");
                    }
                    ImGui::PopStyleVar();
                    ImGui::PopStyleColor(2);
                }
                else if (m_iCurrentPage == ImPage_Configs)
                {
                    ImGui::BeginGroup();
                    {
                        ImAdd::BeginChild("Menu", ImVec2(fGroupWidth, ImGui::GetFontSize() * 4.5f + style.ItemSpacing.y * 3 + style.WindowPadding.y * 2));
                        {
                            static int iToggleKey = VK_INSERT;
                            static bool bAlwaysVisible = false;
                            static bool bDimBackground = false;
                            static bool bParticlesBackground = false;

                            ImGui::TextDisabled("Menu Toggle Key");
                            ImGui::SameLine(ImGui::GetWindowWidth() - style.WindowPadding.x - ImGui::CalcTextSize(szKeyNames[iToggleKey]).x);
                            ImAdd::KeyBind("MenuKey", &iToggleKey, ImGui::CalcTextSize(szKeyNames[iToggleKey]).x);

                            ImAdd::CheckBox("Always Visible", &bAlwaysVisible);
                            ImAdd::CheckBox("Dim Background", &bDimBackground);
                            ImAdd::CheckBox("Particles Background", &bParticlesBackground);
                        }
                        ImAdd::EndChild();
                        ImAdd::BeginChild("Watermark", ImVec2(fGroupWidth, ImGui::GetFontSize() * 3.5f + style.ItemSpacing.y * 2 + style.WindowPadding.y * 2));
                        {
                            static bool bDrawFPS = true;
                            static bool bDrawTime = true;
                            static bool bDrawPing = true;

                            ImAdd::CheckBox("Draw FPS", &bDrawFPS);
                            ImAdd::CheckBox("Draw Time", &bDrawTime);
                            ImAdd::CheckBox("Draw Ping", &bDrawPing);
                        }
                        ImAdd::EndChild();
                    }
                    ImGui::EndGroup();
                    ImGui::SameLine();
                    ImGui::BeginGroup();
                    {
                        ImAdd::BeginChild("Configs", ImVec2(0, ImGui::GetFontSize() * 1.5f + ImGui::GetFrameHeight() * 3 + style.ItemSpacing.y * 3 + style.WindowPadding.y * 2));
                        {
                            float fBtnWidth = (ImGui::GetWindowWidth() - style.ItemSpacing.x * 2 - style.WindowPadding.x * 2) / 3;
                            static int iSelectedConfig = 0;
                            static char cNewConfigName[20] = "";

                            ImGui::PushItemWidth(ImGui::GetWindowWidth() - style.WindowPadding.x * 2);
                            ImAdd::Combo("##ConfigsCombo", &iSelectedConfig, "config1\0config2\0etc\0");
                            ImGui::PopItemWidth();
                            ImAdd::Button("Reset", ImVec2(fBtnWidth, 0));
                            ImGui::SameLine();
                            ImAdd::Button("Save", ImVec2(fBtnWidth, 0));
                            ImGui::SameLine();
                            ImAdd::Button("Delete", ImVec2(-0.1f, 0)); // u can use PushDisabled PopDisabled if u are not selecting a valid config

                            ImAdd::SeparatorText("New Config");
                            ImGui::PushItemWidth(ImGui::GetWindowWidth() - style.ItemSpacing.x - style.WindowPadding.x * 2 - ImGui::CalcTextSize("Add").x - style.FramePadding.x * 2);
                            {
                                ImGui::PushStyleColor(ImGuiCol_BorderShadow, ImVec4());
                                ImGui::InputText("##NewCfgName", cNewConfigName, IM_ARRAYSIZE(cNewConfigName));
                                ImGui::PopStyleColor();
                                ImGui::SameLine();
                                ImAdd::Button("Add");
                            }
                            ImGui::PopItemWidth();
                        }
                        ImAdd::EndChild();
                    }
                    ImGui::EndGroup();
                }
                else
                {
                    ImGui::SetCursorPos(ImGui::GetWindowSize() / 2 - ImGui::CalcTextSize("this page does not exist") / 2);
                    ImGui::TextDisabled("this page does not exist");
                }
            }
            ImGui::EndChild();
        }
        ImGui::End();
    }
    ImGui::Render();

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool CGui::MsgProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    return ImGui_ImplWin32_WndProcHandler(hwnd, umsg, wparam, lparam);
}
