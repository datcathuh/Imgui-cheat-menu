#pragma once

#include "imgui.h"
#include "imgui_internal.h"

#define IMADD_ANIMATIONS_SPEED 0.05f // Second

namespace ImAdd
{
    // Helpers
    ImVec4  HexToColorVec4(unsigned int hex_color, float alpha);

    // DoubleText
    void    DoubleText(ImVec4 color1, ImVec4 color2, const char* label1, const char* label2);

    // Separator
    void    SeparatorText(const char* label, float thickness = 1.0f);

    // Navigation
	bool    RadioFrame(const char* label, int* v, int radio_id, const ImVec2& size_arg = ImVec2(0, 0));
	bool    RadioFrameGradient(const char* label, int* v, int radio_id, const ImVec2& size_arg = ImVec2(0, 0));
    bool    RadioFrameText(const char* label, int* v, int radio_id, bool underline = true, const ImVec2& size_arg = ImVec2(0, 0));
    bool    SelectableFrame(const char* str_id, int selected, const ImVec2& size_arg = ImVec2(0, 0));

    // Custom Child
    void    BeginChild(const char* label, const ImVec2& size_arg = ImVec2(0, 0));
    void    EndChild();

    // Button
    bool    Button(const char* label, const ImVec2& size = ImVec2(0, 0));

    // Checkbox
    bool    CheckBox(const char* label, bool* v);

    // ColorPicker
    bool    ColorButton(const char* desc_id, const ImVec4& col, ImGuiColorEditFlags flags = 0, const ImVec2& size = ImVec2(0, 0));
    bool    ColorEdit4(const char* label, float col[4]);

    // Sliders
    bool    SliderScalar(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format = NULL);
    bool    SliderInt(const char* label, int* v, int v_min, int v_max, const char* format = "%d");
    bool    SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format = "%.1f");

    // Combo
    bool    Selectable(const char* label, bool selected = false, const ImVec2& size_arg = ImVec2(0, 0));
    bool    BeginCombo(const char* label, const char* preview_value, ImGuiComboFlags flags = 0);
    bool	BeginComboPopup(ImGuiID popup_id, const ImRect& bb, ImGuiComboFlags flags);
    bool    Combo(const char* label, int* current_item, const char* const items[], int items_count, int popup_max_height_in_items = -1);
    bool    Combo(const char* label, int* current_item, const char* items_separated_by_zeros, float width = -0.1f, int popup_max_height_in_items = -1);
    bool    Combo(const char* label, int* current_item, const char* (*getter)(void* user_data, int idx), void* user_data, int items_count, int popup_max_height_in_items = -1);

	bool	KeyBind(const char* str_id, int* k, float custom_width = 0);
}

const char* const szKeyNames[] =
{
	"Unknown",
	"LBUTTON",
	"RBUTTON",
	"CANCEL",
	"MBUTTON",
	"XBUTTON1",
	"XBUTTON2",
	"Unknown",
	"BACK",
	"TAB",
	"Unknown",
	"Unknown",
	"CLEAR",
	"RETURN",
	"Unknown",
	"Unknown",
	"SHIFT",
	"CONTROL",
	"MENU",
	"PAUSE",
	"CAPITAL",
	"KANA",
	"Unknown",
	"JUNJA",
	"FINAL",
	"KANJI",
	"Unknown",
	"ESCAPE",
	"CONVERT",
	"NONCONVERT",
	"ACCEPT",
	"MODECHANGE",
	"SPACE",
	"PRIOR",
	"NEXT",
	"END",
	"HOME",
	"LEFT",
	"UP",
	"RIGHT",
	"DOWN",
	"SELECT",
	"PRINT",
	"EXECUTE",
	"SNAPSHOT",
	"INSERT",
	"DELETE",
	"HELP",
	"NUMPAD 0",
	"NUMPAD 1",
	"NUMPAD 2",
	"NUMPAD 3",
	"NUMPAD 4",
	"NUMPAD 5",
	"NUMPAD 6",
	"NUMPAD 7",
	"NUMPAD 8",
	"NUMPAD 9",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"KEY A",
	"KEY B",
	"KEY C",
	"KEY D",
	"KEY E",
	"KEY F",
	"KEY G",
	"KEY H",
	"KEY I",
	"KEY J",
	"KEY K",
	"KEY L",
	"KEY M",
	"KEY N",
	"KEY O",
	"KEY P",
	"KEY Q",
	"KEY R",
	"KEY S",
	"KEY T",
	"KEY U",
	"KEY V",
	"KEY W",
	"KEY X",
	"KEY Y",
	"KEY Z",
	"LWIN",
	"RWIN",
	"APPS",
	"Unknown",
	"SLEEP",
	"NUMPAD 0",
	"NUMPAD 1",
	"NUMPAD 2",
	"NUMPAD 3",
	"NUMPAD 4",
	"NUMPAD 5",
	"NUMPAD 6",
	"NUMPAD 7",
	"NUMPAD 8",
	"NUMPAD 9",
	"MULTIPLY",
	"ADD",
	"SEPARATOR",
	"SUBTRACT",
	"DECIMAL",
	"DIVIDE",
	"F1",
	"F2",
	"F3",
	"F4",
	"F5",
	"F6",
	"F7",
	"F8",
	"F9",
	"F10",
	"F11",
	"F12",
	"F13",
	"F14",
	"F15",
	"F16",
	"F17",
	"F18",
	"F19",
	"F20",
	"F21",
	"F22",
	"F23",
	"F24",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"NUMLOCK",
	"SCROLL",
	"OEM_NEC_EQUAL",
	"OEM_FJ_MASSHOU",
	"OEM_FJ_TOUROKU",
	"OEM_FJ_LOYA",
	"OEM_FJ_ROYA",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"LSHIFT",
	"RSHIFT",
	"LCONTROL",
	"RCONTROL",
	"LMENU",
	"RMENU"
};
