#include <algorithm>
#include <cassert>
#include <string>
#include <set>

#include "CTextEditor.h"

#define IMGUI_SCROLLBAR_WIDTH 14.0f
#define POS_TO_COORDS_COLUMN_OFFSET 0.33f

#include "imgui.h"

// --------------------------------------- //
// ------------- Exposed API ------------- //

CTextEditor::CTextEditor()
{
	m_iUndoIndex = 0;

	m_iTabSize = 4;
	m_fLineSpacing = 1.0f;
	m_bOverwrite = false;
	m_bReadOnly = false;
	m_bAutoIndent = true;
	m_bShowWhitespaces = false;
	m_bShowLineNumbers = true;
	m_bShortTabs = false;

	m_iSetViewAtLine = -1;
	m_iEnsureCursorVisible = -1;
	m_bEnsureCursorVisibleStartToo = false;
	m_bScrollToTop = false;

	m_fTextStart = 20.0f; // position (in pixels) where a code line starts relative to the left of the CTextEditor.
	m_iLeftMargin = 10;
	m_fCurrentSpaceHeight = 20.0f;
	m_fCurrentSpaceWidth = 20.0f;
	m_fLastClickTime = -1.0f;
	m_iFirstVisibleLine = 0;
	m_iLastVisibleLine = 0;
	m_iVisibleLineCount = 0;
	m_iFirstVisibleColumn = 0;
	m_iLastVisibleColumn = 0;
	m_iVisibleColumnCount = 0;
	m_fContentWidth = 0.0f;
	m_fContentHeight = 0.0f;
	m_fScrollX = 0.0f;
	m_fScrollY = 0.0f;
	m_bPanning = false;
	m_bDraggingSelection = false;
	m_bCursorPositionChanged = false;
	m_bCursorOnBracket = false;

	m_iColorRangeMin = 0;
	m_iColorRangeMax = 0;
	m_bCheckComments = true;
	m_pLanguageDefinition = nullptr;

	SetPalette(defaultPalette);
	m_Lines.push_back(Line());
}

CTextEditor::~CTextEditor()
{
}

void CTextEditor::SetPalette(PaletteId Value)
{
	m_PaletteId = Value;
	const Palette* palletteBase = nullptr;
	switch (m_PaletteId)
	{
	case PaletteId::Dark:
		palletteBase = &(GetDarkPalette());
		break;
	case PaletteId::Light:
		palletteBase = &(GetLightPalette());
		break;
	default:
		palletteBase = &(GetDarkPalette());
		break;
	}
	/* Update palette with the current alpha from style */
	for (int i = 0; i < (int)PaletteIndex::Max; ++i)
	{
		ImVec4 color = U32ColorToVec4((*palletteBase)[i]);
		color.w *= ImGui::GetStyle().Alpha;
		m_Palette[i] = ImGui::ColorConvertFloat4ToU32(color);
	}
}

void CTextEditor::SetLanguageDefinition(LanguageDefinitionId Value)
{
	m_LanguageDefinitionId = Value;
	switch (m_LanguageDefinitionId)
	{
	case LanguageDefinitionId::None:
		m_pLanguageDefinition = nullptr;
		return;
	case LanguageDefinitionId::Cpp:
		m_pLanguageDefinition = &(LanguageDefinition::Cpp());
		break;
	case LanguageDefinitionId::C:
		m_pLanguageDefinition = &(LanguageDefinition::C());
		break;
	case LanguageDefinitionId::Cs:
		m_pLanguageDefinition = &(LanguageDefinition::Cs());
		break;
	case LanguageDefinitionId::Python:
		m_pLanguageDefinition = &(LanguageDefinition::Python());
		break;
	case LanguageDefinitionId::Lua:
		m_pLanguageDefinition = &(LanguageDefinition::Lua());
		break;
	case LanguageDefinitionId::Json:
		m_pLanguageDefinition = &(LanguageDefinition::Json());
		break;
	case LanguageDefinitionId::Sql:
		m_pLanguageDefinition = &(LanguageDefinition::Sql());
		break;
	case LanguageDefinitionId::AngelScript:
		m_pLanguageDefinition = &(LanguageDefinition::AngelScript());
		break;
	case LanguageDefinitionId::Glsl:
		m_pLanguageDefinition = &(LanguageDefinition::Glsl());
		break;
	case LanguageDefinitionId::Hlsl:
		m_pLanguageDefinition = &(LanguageDefinition::Hlsl());
		break;
	}

	m_RegexList.clear();
	for (const auto& r : m_pLanguageDefinition->m_TokenRegexStrings)
		m_RegexList.push_back(std::make_pair(boost::regex(r.first, boost::regex_constants::optimize), r.second));

	Colorize();
}

const char* CTextEditor::GetLanguageDefinitionName() const
{
	return m_pLanguageDefinition != nullptr ? m_pLanguageDefinition->m_Name.c_str() : "None";
}

void CTextEditor::SetTabSize(int iValue)
{
	m_iTabSize = Max(1, Min(8, iValue));
}

void CTextEditor::SetLineSpacing(float fValue)
{
	m_fLineSpacing = Max(1.0f, Min(2.0f, fValue));
}

void CTextEditor::SelectAll()
{
	ClearSelections();
	ClearExtraCursors();
	MoveTop();
	MoveBottom(true);
}

void CTextEditor::SelectLine(int iLine)
{
	ClearSelections();
	ClearExtraCursors();
	SetSelection({ iLine, 0 }, { iLine, GetLineMaxColumn(iLine) });
}

void CTextEditor::SelectRegion(int iStartLine, int iStartChar, int iEndLine, int iEndChar)
{
	ClearSelections();
	ClearExtraCursors();
	SetSelection(iStartLine, iStartChar, iEndLine, iEndChar);
}

void CTextEditor::SelectNextOccurrenceOf(const char* Text, int iTextSize, bool bCaseSensitive)
{
	ClearSelections();
	ClearExtraCursors();
	SelectNextOccurrenceOf(Text, iTextSize, -1, bCaseSensitive);
}

void CTextEditor::SelectAllOccurrencesOf(const char* Text, int iTextSize, bool bCaseSensitive)
{
	ClearSelections();
	ClearExtraCursors();
	SelectNextOccurrenceOf(Text, iTextSize, -1, bCaseSensitive);
	Coordinates startPos = m_State.m_Cursors[m_State.GetLastAddedCursorIndex()].m_InteractiveEnd;
	while (true)
	{
		AddCursorForNextOccurrence(bCaseSensitive);
		Coordinates lastAddedPos = m_State.m_Cursors[m_State.GetLastAddedCursorIndex()].m_InteractiveEnd;
		if (lastAddedPos == startPos)
			break;
	}
}

bool CTextEditor::AnyCursorHasSelection() const
{
	for (int c = 0; c <= m_State.m_iCurrentCursor; c++)
		if (m_State.m_Cursors[c].HasSelection())
			return true;
	return false;
}

bool CTextEditor::AllCursorsHaveSelection() const
{
	for (int c = 0; c <= m_State.m_iCurrentCursor; c++)
		if (!m_State.m_Cursors[c].HasSelection())
			return false;
	return true;
}

void CTextEditor::ClearExtraCursors()
{
	m_State.m_iCurrentCursor = 0;
}

void CTextEditor::ClearSelections()
{
	for (int c = m_State.m_iCurrentCursor; c > -1; c--)
		m_State.m_Cursors[c].m_InteractiveEnd =
		m_State.m_Cursors[c].m_InteractiveStart =
		m_State.m_Cursors[c].GetSelectionEnd();
}

void CTextEditor::SetCursorPosition(int iLine, int iCharIndex)
{
	SetCursorPosition({ iLine, GetCharacterColumn(iLine, iCharIndex) }, -1, true);
}

int CTextEditor::GetFirstVisibleLine()
{
	return m_iFirstVisibleLine;
}

int CTextEditor::GetLastVisibleLine()
{
	return m_iLastVisibleLine;
}

void CTextEditor::SetViewAtLine(int iLine, SetViewAtLineMode Mode)
{
	m_iSetViewAtLine = iLine;
	m_SetViewAtLineMode = Mode;
}

void CTextEditor::Copy()
{
	if (AnyCursorHasSelection())
	{
		std::string clipboardText = GetClipboardText();
		ImGui::SetClipboardText(clipboardText.c_str());
	}
	else
	{
		if (!m_Lines.empty())
		{
			std::string str;
			auto& line = m_Lines[GetActualCursorCoordinates().m_iLine];
			for (auto& g : line)
				str.push_back(g.m_cChar);
			ImGui::SetClipboardText(str.c_str());
		}
	}
}

void CTextEditor::Cut()
{
	if (m_bReadOnly)
	{
		Copy();
	}
	else
	{
		if (AnyCursorHasSelection())
		{
			UndoRecord u;
			u.m_Before = m_State;

			Copy();
			for (int c = m_State.m_iCurrentCursor; c > -1; c--)
			{
				u.m_Operations.push_back({ GetSelectedText(c), m_State.m_Cursors[c].GetSelectionStart(), m_State.m_Cursors[c].GetSelectionEnd(), UndoOperationType::Delete });
				DeleteSelection(c);
			}

			u.m_After = m_State;
			AddUndo(u);
		}
	}
}

void CTextEditor::Paste()
{
	if (m_bReadOnly)
		return;

	if (ImGui::GetClipboardText() == nullptr)
		return; // something other than text in the clipboard

	// check if we should do multicursor paste
	std::string clipText = ImGui::GetClipboardText();
	bool canPasteToMultipleCursors = false;
	std::vector<std::pair<int, int>> clipTextLines;
	if (m_State.m_iCurrentCursor > 0)
	{
		clipTextLines.push_back({ 0,0 });
		for (int i = 0; i < clipText.length(); i++)
		{
			if (clipText[i] == '\n')
			{
				clipTextLines.back().second = i;
				clipTextLines.push_back({ i + 1, 0 });
			}
		}
		clipTextLines.back().second = static_cast<int>(clipText.length());
		canPasteToMultipleCursors = clipTextLines.size() == m_State.m_iCurrentCursor + 1;
	}

	if (clipText.length() > 0)
	{
		UndoRecord u;
		u.m_Before = m_State;

		if (AnyCursorHasSelection())
		{
			for (int c = m_State.m_iCurrentCursor; c > -1; c--)
			{
				u.m_Operations.push_back({ GetSelectedText(c), m_State.m_Cursors[c].GetSelectionStart(), m_State.m_Cursors[c].GetSelectionEnd(), UndoOperationType::Delete });
				DeleteSelection(c);
			}
		}

		for (int c = m_State.m_iCurrentCursor; c > -1; c--)
		{
			Coordinates start = GetActualCursorCoordinates(c);
			if (canPasteToMultipleCursors)
			{
				std::string clipSubText = clipText.substr(clipTextLines[c].first, clipTextLines[c].second - clipTextLines[c].first);
				InsertTextAtCursor(clipSubText.c_str(), c);
				u.m_Operations.push_back({ clipSubText, start, GetActualCursorCoordinates(c), UndoOperationType::Add });
			}
			else
			{
				InsertTextAtCursor(clipText.c_str(), c);
				u.m_Operations.push_back({ clipText, start, GetActualCursorCoordinates(c), UndoOperationType::Add });
			}
		}

		u.m_After = m_State;
		AddUndo(u);
	}
}

void CTextEditor::Undo(int iSteps)
{
	while (CanUndo() && iSteps-- > 0)
		m_UndoBuffer[--m_iUndoIndex].Undo(this);
}

void CTextEditor::Redo(int iSteps)
{
	while (CanRedo() && iSteps-- > 0)
		m_UndoBuffer[m_iUndoIndex++].Redo(this);
}

void CTextEditor::SetText(const std::string& Text)
{
	m_Lines.clear();
	m_Lines.emplace_back(Line());
	for (auto chr : Text)
	{
		if (chr == '\r')
			continue;

		if (chr == '\n')
			m_Lines.emplace_back(Line());
		else
		{
			m_Lines.back().emplace_back(Glyph(chr, PaletteIndex::Default));
		}
	}

	m_bScrollToTop = true;

	m_UndoBuffer.clear();
	m_iUndoIndex = 0;

	Colorize();
}

std::string CTextEditor::GetText() const
{
	auto lastLine = (int)m_Lines.size() - 1;
	auto lastLineLength = GetLineMaxColumn(lastLine);
	Coordinates startCoords = Coordinates();
	Coordinates endCoords = Coordinates(lastLine, lastLineLength);
	return startCoords < endCoords ? GetText(startCoords, endCoords) : "";
}

void CTextEditor::SetTextLines(const std::vector<std::string>& Lines)
{
	m_Lines.clear();

	if (Lines.empty())
		m_Lines.emplace_back(Line());
	else
	{
		m_Lines.resize(Lines.size());

		for (size_t i = 0; i < Lines.size(); ++i)
		{
			const std::string& Line = Lines[i];

			m_Lines[i].reserve(Line.size());
			for (size_t j = 0; j < Line.size(); ++j)
				m_Lines[i].emplace_back(Glyph(Line[j], PaletteIndex::Default));
		}
	}

	m_bScrollToTop = true;

	m_UndoBuffer.clear();
	m_iUndoIndex = 0;

	Colorize();
}

std::vector<std::string> CTextEditor::GetTextLines() const
{
	std::vector<std::string> result;

	result.reserve(m_Lines.size());

	for (auto& line : m_Lines)
	{
		std::string text;

		text.resize(line.size());

		for (size_t i = 0; i < line.size(); ++i)
			text[i] = line[i].m_cChar;

		result.emplace_back(std::move(text));
	}

	return result;
}

bool CTextEditor::Render(const char* Title, bool bParentIsFocused, const ImVec2& Size, bool bBorder)
{
	if (m_bCursorPositionChanged)
		OnCursorPositionChanged();
	m_bCursorPositionChanged = false;

	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4(m_Palette[(int)PaletteIndex::Background]));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

	ImGui::BeginChild(Title, Size, bBorder ? ImGuiChildFlags_Border : ImGuiChildFlags_None, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNavInputs);

	bool isFocused = ImGui::IsWindowFocused();
	HandleKeyboardInputs(bParentIsFocused);
	HandleMouseInputs();
	ColorizeInternal();
	Render(bParentIsFocused);

	ImGui::EndChild();

	ImGui::PopStyleVar();
	ImGui::PopStyleColor();

	return isFocused;
}

// ------------------------------------ //
// ---------- Generic utils ----------- //

// https://en.wikipedia.org/wiki/UTF-8
// We assume that the char is a standalone character (<128) or a leading byte of an UTF-8 code sequence (non-10xxxxxx code)
static int UTF8CharLength(char c)
{
	if ((c & 0xFE) == 0xFC)
		return 6;
	if ((c & 0xFC) == 0xF8)
		return 5;
	if ((c & 0xF8) == 0xF0)
		return 4;
	else if ((c & 0xF0) == 0xE0)
		return 3;
	else if ((c & 0xE0) == 0xC0)
		return 2;
	return 1;
}

// "Borrowed" from ImGui source
static inline int Im_TextCharToUtf8(char* buf, int buf_size, unsigned int c)
{
	if (c < 0x80)
	{
		buf[0] = (char)c;
		return 1;
	}
	if (c < 0x800)
	{
		if (buf_size < 2) return 0;
		buf[0] = (char)(0xc0 + (c >> 6));
		buf[1] = (char)(0x80 + (c & 0x3f));
		return 2;
	}
	if (c >= 0xdc00 && c < 0xe000)
	{
		return 0;
	}
	if (c >= 0xd800 && c < 0xdc00)
	{
		if (buf_size < 4) return 0;
		buf[0] = (char)(0xf0 + (c >> 18));
		buf[1] = (char)(0x80 + ((c >> 12) & 0x3f));
		buf[2] = (char)(0x80 + ((c >> 6) & 0x3f));
		buf[3] = (char)(0x80 + ((c) & 0x3f));
		return 4;
	}
	//else if (c < 0x10000)
	{
		if (buf_size < 3) return 0;
		buf[0] = (char)(0xe0 + (c >> 12));
		buf[1] = (char)(0x80 + ((c >> 6) & 0x3f));
		buf[2] = (char)(0x80 + ((c) & 0x3f));
		return 3;
	}
}

static inline bool CharIsWordChar(char ch)
{
	int sizeInBytes = UTF8CharLength(ch);
	return sizeInBytes > 1 ||
		ch >= 'a' && ch <= 'z' ||
		ch >= 'A' && ch <= 'Z' ||
		ch >= '0' && ch <= '9' ||
		ch == '_';
}

// ------------------------------------ //
// ------------- Internal ------------- //


// ---------- Editor state functions --------- //

void CTextEditor::EditorState::AddCursor()
{
	// vector is never resized to smaller size, mCurrentCursor points to last available cursor in vector
	m_iCurrentCursor++;
	m_Cursors.resize(m_iCurrentCursor + 1);
	m_iLastAddedCursor = m_iCurrentCursor;
}

int CTextEditor::EditorState::GetLastAddedCursorIndex()
{
	return m_iLastAddedCursor > m_iCurrentCursor ? 0 : m_iLastAddedCursor;
}

void CTextEditor::EditorState::SortCursorsFromTopToBottom()
{
	Coordinates lastAddedCursorPos = m_Cursors[GetLastAddedCursorIndex()].m_InteractiveEnd;
	std::sort(m_Cursors.begin(), m_Cursors.begin() + (m_iCurrentCursor + 1), [](const Cursor& a, const Cursor& b) -> bool
		{
			return a.GetSelectionStart() < b.GetSelectionStart();
		});
	// update last added cursor index to be valid after sort
	for (int c = m_iCurrentCursor; c > -1; c--)
		if (m_Cursors[c].m_InteractiveEnd == lastAddedCursorPos)
			m_iLastAddedCursor = c;
}

// ---------- Undo record functions --------- //

CTextEditor::UndoRecord::UndoRecord(const std::vector<UndoOperation>& Operations,
	CTextEditor::EditorState& Before, CTextEditor::EditorState& After)
{
	m_Operations = Operations;
	m_Before = Before;
	m_After = After;
	for (const UndoOperation& o : m_Operations)
		assert(o.m_Start <= o.m_End);
}

void CTextEditor::UndoRecord::Undo(CTextEditor* pTextEditor)
{
	for (auto i = m_Operations.size() - 1; i > -1; i--)
	{
		const UndoOperation& operation = m_Operations[i];
		if (!operation.m_Text.empty())
		{
			switch (operation.m_Type)
			{
			case UndoOperationType::Delete:
			{
				auto start = operation.m_Start;
				pTextEditor->InsertTextAt(start, operation.m_Text.c_str());
				pTextEditor->Colorize(operation.m_Start.m_iLine - 1, operation.m_End.m_iLine - operation.m_Start.m_iLine + 2);
				break;
			}
			case UndoOperationType::Add:
			{
				pTextEditor->DeleteRange(operation.m_Start, operation.m_End);
				pTextEditor->Colorize(operation.m_Start.m_iLine - 1, operation.m_End.m_iLine - operation.m_Start.m_iLine + 2);
				break;
			}
			}
		}
	}

	pTextEditor->m_State = m_Before;
	pTextEditor->EnsureCursorVisible();
}

void CTextEditor::UndoRecord::Redo(CTextEditor* pTextEditor)
{
	for (int i = 0; i < m_Operations.size(); i++)
	{
		const UndoOperation& operation = m_Operations[i];
		if (!operation.m_Text.empty())
		{
			switch (operation.m_Type)
			{
			case UndoOperationType::Delete:
			{
				pTextEditor->DeleteRange(operation.m_Start, operation.m_End);
				pTextEditor->Colorize(operation.m_Start.m_iLine - 1, operation.m_End.m_iLine - operation.m_Start.m_iLine + 1);
				break;
			}
			case UndoOperationType::Add:
			{
				auto start = operation.m_Start;
				pTextEditor->InsertTextAt(start, operation.m_Text.c_str());
				pTextEditor->Colorize(operation.m_Start.m_iLine - 1, operation.m_End.m_iLine - operation.m_Start.m_iLine + 1);
				break;
			}
			}
		}
	}

	pTextEditor->m_State = m_After;
	pTextEditor->EnsureCursorVisible();
}

// ---------- Text editor internal functions --------- //

std::string CTextEditor::GetText(const Coordinates& Start, const Coordinates& End) const
{
	assert(Start < End);

	std::string result;
	auto lstart = Start.m_iLine;
	auto lend = End.m_iLine;
	auto istart = GetCharacterIndexR(Start);
	auto iend = GetCharacterIndexR(End);
	size_t s = 0;

	for (size_t i = lstart; i < lend; i++)
		s += m_Lines[i].size();

	result.reserve(s + s / 8);

	while (istart < iend || lstart < lend)
	{
		if (lstart >= (int)m_Lines.size())
			break;

		auto& line = m_Lines[lstart];
		if (istart < (int)line.size())
		{
			result += line[istart].m_cChar;
			istart++;
		}
		else
		{
			istart = 0;
			++lstart;
			result += '\n';
		}
	}

	return result;
}

std::string CTextEditor::GetClipboardText() const
{
	std::string result;
	for (int c = 0; c <= m_State.m_iCurrentCursor; c++)
	{
		if (m_State.m_Cursors[c].GetSelectionStart() < m_State.m_Cursors[c].GetSelectionEnd())
		{
			if (result.length() != 0)
				result += '\n';
			result += GetText(m_State.m_Cursors[c].GetSelectionStart(), m_State.m_Cursors[c].GetSelectionEnd());
		}
	}
	return result;
}

std::string CTextEditor::GetSelectedText(int iCursor) const
{
	if (iCursor == -1)
		iCursor = m_State.m_iCurrentCursor;

	return GetText(m_State.m_Cursors[iCursor].GetSelectionStart(), m_State.m_Cursors[iCursor].GetSelectionEnd());
}

void CTextEditor::SetCursorPosition(const Coordinates& Position, int iCursor, bool bClearSelection)
{
	if (iCursor == -1)
		iCursor = m_State.m_iCurrentCursor;

	m_bCursorPositionChanged = true;
	if (bClearSelection)
		m_State.m_Cursors[iCursor].m_InteractiveStart = Position;
	if (m_State.m_Cursors[iCursor].m_InteractiveEnd != Position)
	{
		m_State.m_Cursors[iCursor].m_InteractiveEnd = Position;
		EnsureCursorVisible();
	}
}

int CTextEditor::InsertTextAt(Coordinates& /* inout */ Where, const char* Value)
{
	assert(!m_bReadOnly);

	int cindex = GetCharacterIndexR(Where);
	int totalLines = 0;
	while (*Value != '\0')
	{
		assert(!m_Lines.empty());

		if (*Value == '\r')
		{
			// skip
			++Value;
		}
		else if (*Value == '\n')
		{
			if (cindex < (int)m_Lines[Where.m_iLine].size())
			{
				auto& newLine = InsertLine(Where.m_iLine + 1);
				auto& line = m_Lines[Where.m_iLine];
				AddGlyphsToLine(Where.m_iLine + 1, 0, line.begin() + cindex, line.end());
				RemoveGlyphsFromLine(Where.m_iLine, cindex);
			}
			else
			{
				InsertLine(Where.m_iLine + 1);
			}
			++Where.m_iLine;
			Where.m_iColumn = 0;
			cindex = 0;
			++totalLines;
			++Value;
		}
		else
		{
			auto& line = m_Lines[Where.m_iLine];
			auto d = UTF8CharLength(*Value);
			while (d-- > 0 && *Value != '\0')
				AddGlyphToLine(Where.m_iLine, cindex++, Glyph(*Value++, PaletteIndex::Default));
			Where.m_iColumn = GetCharacterColumn(Where.m_iLine, cindex);
		}
	}

	return totalLines;
}

void CTextEditor::InsertTextAtCursor(const char* Value, int iCursor)
{
	if (Value == nullptr)
		return;
	if (iCursor == -1)
		iCursor = m_State.m_iCurrentCursor;

	auto pos = GetActualCursorCoordinates(iCursor);
	auto start = std::min(pos, m_State.m_Cursors[iCursor].GetSelectionStart());
	int totalLines = pos.m_iLine - start.m_iLine;

	totalLines += InsertTextAt(pos, Value);

	SetCursorPosition(pos, iCursor);
	Colorize(start.m_iLine - 1, totalLines + 2);
}

bool CTextEditor::Move(int& iLine, int& iCharIndex, bool bLeft, bool bLockLine) const
{
	// assumes given char index is not in the middle of utf8 sequence
	// char index can be line.length()

	// invalid line
	if (iLine >= m_Lines.size())
		return false;

	if (bLeft)
	{
		if (iCharIndex == 0)
		{
			if (bLockLine || iLine == 0)
				return false;
			iLine--;
			iCharIndex = static_cast<int>(m_Lines[iLine].size());
		}
		else
		{
			iCharIndex--;
			while (iCharIndex > 0 && IsUTFSequence(m_Lines[iLine][iCharIndex].m_cChar))
				iCharIndex--;
		}
	}
	else // right
	{
		if (iCharIndex == m_Lines[iLine].size())
		{
			if (bLockLine || iLine == m_Lines.size() - 1)
				return false;
			iLine++;
			iCharIndex = 0;
		}
		else
		{
			int seqLength = UTF8CharLength(m_Lines[iLine][iCharIndex].m_cChar);
			iCharIndex = std::min(iCharIndex + seqLength, (int)m_Lines[iLine].size());
		}
	}
	return true;
}

void CTextEditor::MoveCharIndexAndColumn(int iLine, int& iCharIndex, int& iColumn) const
{
	assert(iLine < m_Lines.size());
	assert(iCharIndex < m_Lines[iLine].size());
	char c = m_Lines[iLine][iCharIndex].m_cChar;
	iCharIndex += UTF8CharLength(c);
	if (c == '\t')
		iColumn = (iColumn / m_iTabSize) * m_iTabSize + m_iTabSize;
	else
		iColumn++;
}

void CTextEditor::MoveCoords(Coordinates& Coords, MoveDirection Direction, bool bWordMode, int iLineCount) const
{
	int charIndex = GetCharacterIndexR(Coords);
	int lineIndex = Coords.m_iLine;
	switch (Direction)
	{
	case MoveDirection::Right:
		if (charIndex >= m_Lines[lineIndex].size())
		{
			if (lineIndex < m_Lines.size() - 1)
			{
				Coords.m_iLine = std::max(0, std::min((int)m_Lines.size() - 1, lineIndex + 1));
				Coords.m_iColumn = 0;
			}
		}
		else
		{
			Move(lineIndex, charIndex);
			int oneStepRightColumn = GetCharacterColumn(lineIndex, charIndex);
			if (bWordMode)
			{
				Coords = FindWordEnd(Coords);
				Coords.m_iColumn = std::max(Coords.m_iColumn, oneStepRightColumn);
			}
			else
				Coords.m_iColumn = oneStepRightColumn;
		}
		break;
	case MoveDirection::Left:
		if (charIndex == 0)
		{
			if (lineIndex > 0)
			{
				Coords.m_iLine = lineIndex - 1;
				Coords.m_iColumn = GetLineMaxColumn(Coords.m_iLine);
			}
		}
		else
		{
			Move(lineIndex, charIndex, true);
			Coords.m_iColumn = GetCharacterColumn(lineIndex, charIndex);
			if (bWordMode)
				Coords = FindWordStart(Coords);
		}
		break;
	case MoveDirection::Up:
		Coords.m_iLine = std::max(0, lineIndex - iLineCount);
		break;
	case MoveDirection::Down:
		Coords.m_iLine = std::max(0, std::min((int)m_Lines.size() - 1, lineIndex + iLineCount));
		break;
	}
}

void CTextEditor::MoveUp(int iAmount, bool bSelect)
{
	for (int c = 0; c <= m_State.m_iCurrentCursor; c++)
	{
		Coordinates newCoords = m_State.m_Cursors[c].m_InteractiveEnd;
		MoveCoords(newCoords, MoveDirection::Up, false, iAmount);
		SetCursorPosition(newCoords, c, !bSelect);
	}
	EnsureCursorVisible();
}

void CTextEditor::MoveDown(int iAmount, bool bSelect)
{
	for (int c = 0; c <= m_State.m_iCurrentCursor; c++)
	{
		assert(m_State.m_Cursors[c].m_InteractiveEnd.m_iColumn >= 0);
		Coordinates newCoords = m_State.m_Cursors[c].m_InteractiveEnd;
		MoveCoords(newCoords, MoveDirection::Down, false, iAmount);
		SetCursorPosition(newCoords, c, !bSelect);
	}
	EnsureCursorVisible();
}

void CTextEditor::MoveLeft(bool bSelect, bool bWordMode)
{
	if (m_Lines.empty())
		return;

	if (AnyCursorHasSelection() && !bSelect && !bWordMode)
	{
		for (int c = 0; c <= m_State.m_iCurrentCursor; c++)
			SetCursorPosition(m_State.m_Cursors[c].GetSelectionStart(), c);
	}
	else
	{
		for (int c = 0; c <= m_State.m_iCurrentCursor; c++)
		{
			Coordinates newCoords = m_State.m_Cursors[c].m_InteractiveEnd;
			MoveCoords(newCoords, MoveDirection::Left, bWordMode);
			SetCursorPosition(newCoords, c, !bSelect);
		}
	}
	EnsureCursorVisible();
}

void CTextEditor::MoveRight(bool bSelect, bool bWordMode)
{
	if (m_Lines.empty())
		return;

	if (AnyCursorHasSelection() && !bSelect && !bWordMode)
	{
		for (int c = 0; c <= m_State.m_iCurrentCursor; c++)
			SetCursorPosition(m_State.m_Cursors[c].GetSelectionEnd(), c);
	}
	else
	{
		for (int c = 0; c <= m_State.m_iCurrentCursor; c++)
		{
			Coordinates newCoords = m_State.m_Cursors[c].m_InteractiveEnd;
			MoveCoords(newCoords, MoveDirection::Right, bWordMode);
			SetCursorPosition(newCoords, c, !bSelect);
		}
	}
	EnsureCursorVisible();
}

void CTextEditor::MoveTop(bool bSelect)
{
	SetCursorPosition(Coordinates(0, 0), m_State.m_iCurrentCursor, !bSelect);
}

void CTextEditor::CTextEditor::MoveBottom(bool bSelect)
{
	int maxLine = (int)m_Lines.size() - 1;
	Coordinates newPos = Coordinates(maxLine, GetLineMaxColumn(maxLine));
	SetCursorPosition(newPos, m_State.m_iCurrentCursor, !bSelect);
}

void CTextEditor::MoveHome(bool bSelect)
{
	for (int c = 0; c <= m_State.m_iCurrentCursor; c++)
		SetCursorPosition(Coordinates(m_State.m_Cursors[c].m_InteractiveEnd.m_iLine, 0), c, !bSelect);
}

void CTextEditor::MoveEnd(bool bSelect)
{
	for (int c = 0; c <= m_State.m_iCurrentCursor; c++)
	{
		int lindex = m_State.m_Cursors[c].m_InteractiveEnd.m_iLine;
		SetCursorPosition(Coordinates(lindex, GetLineMaxColumn(lindex)), c, !bSelect);
	}
}

void CTextEditor::EnterCharacter(ImWchar aChar, bool aShift)
{
	assert(!m_bReadOnly);

	bool hasSelection = AnyCursorHasSelection();
	bool anyCursorHasMultilineSelection = false;
	for (int c = m_State.m_iCurrentCursor; c > -1; c--)
		if (m_State.m_Cursors[c].GetSelectionStart().m_iLine != m_State.m_Cursors[c].GetSelectionEnd().m_iLine)
		{
			anyCursorHasMultilineSelection = true;
			break;
		}
	bool isIndentOperation = hasSelection && anyCursorHasMultilineSelection && aChar == '\t';
	if (isIndentOperation)
	{
		ChangeCurrentLinesIndentation(!aShift);
		return;
	}

	UndoRecord u;
	u.m_Before = m_State;

	if (hasSelection)
	{
		for (int c = m_State.m_iCurrentCursor; c > -1; c--)
		{
			u.m_Operations.push_back({ GetSelectedText(c), m_State.m_Cursors[c].GetSelectionStart(), m_State.m_Cursors[c].GetSelectionEnd(), UndoOperationType::Delete });
			DeleteSelection(c);
		}
	}

	std::vector<Coordinates> coords;
	for (int c = m_State.m_iCurrentCursor; c > -1; c--) // order important here for typing \n in the same line at the same time
	{
		auto coord = GetActualCursorCoordinates(c);
		coords.push_back(coord);
		UndoOperation added;
		added.m_Type = UndoOperationType::Add;
		added.m_Start = coord;

		assert(!m_Lines.empty());

		if (aChar == '\n')
		{
			InsertLine(coord.m_iLine + 1);
			auto& line = m_Lines[coord.m_iLine];
			auto& newLine = m_Lines[coord.m_iLine + 1];

			added.m_Text = "";
			added.m_Text += (char)aChar;
			if (m_bAutoIndent)
				for (int i = 0; i < line.size() && isascii(line[i].m_cChar) && isblank(line[i].m_cChar); ++i)
				{
					newLine.push_back(line[i]);
					added.m_Text += line[i].m_cChar;
				}

			const size_t whitespaceSize = newLine.size();
			auto cindex = GetCharacterIndexR(coord);
			AddGlyphsToLine(coord.m_iLine + 1, static_cast<int>(newLine.size()), line.begin() + cindex, line.end());
			RemoveGlyphsFromLine(coord.m_iLine, cindex);
			SetCursorPosition(Coordinates(coord.m_iLine + 1, GetCharacterColumn(coord.m_iLine + 1, (int)whitespaceSize)), c);
		}
		else
		{
			char buf[7];
			int e = Im_TextCharToUtf8(buf, 7, aChar);
			if (e > 0)
			{
				buf[e] = '\0';
				auto& line = m_Lines[coord.m_iLine];
				auto cindex = GetCharacterIndexR(coord);

				if (m_bOverwrite && cindex < (int)line.size())
				{
					auto d = UTF8CharLength(line[cindex].m_cChar);

					UndoOperation removed;
					removed.m_Type = UndoOperationType::Delete;
					removed.m_Start = m_State.m_Cursors[c].m_InteractiveEnd;
					removed.m_End = Coordinates(coord.m_iLine, GetCharacterColumn(coord.m_iLine, cindex + d));

					while (d-- > 0 && cindex < (int)line.size())
					{
						removed.m_Text += line[cindex].m_cChar;
						RemoveGlyphsFromLine(coord.m_iLine, cindex, cindex + 1);
					}
					u.m_Operations.push_back(removed);
				}

				for (auto p = buf; *p != '\0'; p++, ++cindex)
					AddGlyphToLine(coord.m_iLine, cindex, Glyph(*p, PaletteIndex::Default));
				added.m_Text = buf;

				SetCursorPosition(Coordinates(coord.m_iLine, GetCharacterColumn(coord.m_iLine, cindex)), c);
			}
			else
				continue;
		}

		added.m_End = GetActualCursorCoordinates(c);
		u.m_Operations.push_back(added);
	}

	u.m_After = m_State;
	AddUndo(u);

	for (const auto& coord : coords)
		Colorize(coord.m_iLine - 1, 3);
	EnsureCursorVisible();
}

void CTextEditor::Backspace(bool bWordMode)
{
	assert(!m_bReadOnly);

	if (m_Lines.empty())
		return;

	if (AnyCursorHasSelection())
		Delete(bWordMode);
	else
	{
		EditorState stateBeforeDeleting = m_State;
		MoveLeft(true, bWordMode);
		if (!AllCursorsHaveSelection()) // can't do backspace if any cursor at {0,0}
		{
			if (AnyCursorHasSelection())
				MoveRight();
			return;
		}
			
		OnCursorPositionChanged(); // might combine cursors
		Delete(bWordMode, &stateBeforeDeleting);
	}
}

void CTextEditor::Delete(bool bWordMode, const EditorState* pEditorState)
{
	assert(!m_bReadOnly);

	if (m_Lines.empty())
		return;

	if (AnyCursorHasSelection())
	{
		UndoRecord u;
		u.m_Before = pEditorState == nullptr ? m_State : *pEditorState;
		for (int c = m_State.m_iCurrentCursor; c > -1; c--)
		{
			if (!m_State.m_Cursors[c].HasSelection())
				continue;
			u.m_Operations.push_back({ GetSelectedText(c), m_State.m_Cursors[c].GetSelectionStart(), m_State.m_Cursors[c].GetSelectionEnd(), UndoOperationType::Delete });
			DeleteSelection(c);
		}
		u.m_After = m_State;
		AddUndo(u);
	}
	else
	{
		EditorState stateBeforeDeleting = m_State;
		MoveRight(true, bWordMode);
		if (!AllCursorsHaveSelection()) // can't do delete if any cursor at end of last line
		{
			if (AnyCursorHasSelection())
				MoveLeft();
			return;
		}

		OnCursorPositionChanged(); // might combine cursors
		Delete(bWordMode, &stateBeforeDeleting);
	}
}

void CTextEditor::SetSelection(Coordinates Start, Coordinates End, int aCursor)
{
	if (aCursor == -1)
		aCursor = m_State.m_iCurrentCursor;

	Coordinates minCoords = Coordinates(0, 0);
	int maxLine = (int)m_Lines.size() - 1;
	Coordinates maxCoords = Coordinates(maxLine, GetLineMaxColumn(maxLine));
	if (Start < minCoords)
		Start = minCoords;
	else if (Start > maxCoords)
		Start = maxCoords;
	if (End < minCoords)
		End = minCoords;
	else if (End > maxCoords)
		End = maxCoords;

	m_State.m_Cursors[aCursor].m_InteractiveStart = Start;
	SetCursorPosition(End, aCursor, false);
}

void CTextEditor::SetSelection(int StartLine, int StartChar, int EndLine, int EndChar, int aCursor)
{
	Coordinates startCoords = { StartLine, GetCharacterColumn(StartLine, StartChar) };
	Coordinates endCoords = { EndLine, GetCharacterColumn(EndLine, EndChar) };
	SetSelection(startCoords, endCoords, aCursor);
}

void CTextEditor::SelectNextOccurrenceOf(const char* aText, int aTextSize, int aCursor, bool aCaseSensitive)
{
	if (aCursor == -1)
		aCursor = m_State.m_iCurrentCursor;
	Coordinates nextStart, nextEnd;
	FindNextOccurrence(aText, aTextSize, m_State.m_Cursors[aCursor].m_InteractiveEnd, nextStart, nextEnd, aCaseSensitive);
	SetSelection(nextStart, nextEnd, aCursor);
	EnsureCursorVisible(aCursor, true);
}

void CTextEditor::AddCursorForNextOccurrence(bool aCaseSensitive)
{
	const Cursor& currentCursor = m_State.m_Cursors[m_State.GetLastAddedCursorIndex()];
	if (currentCursor.GetSelectionStart() == currentCursor.GetSelectionEnd())
		return;

	std::string selectionText = GetText(currentCursor.GetSelectionStart(), currentCursor.GetSelectionEnd());
	Coordinates nextStart, nextEnd;
	if (!FindNextOccurrence(selectionText.c_str(), static_cast<int>(selectionText.length()), currentCursor.GetSelectionEnd(), nextStart, nextEnd, aCaseSensitive))
		return;

	m_State.AddCursor();
	SetSelection(nextStart, nextEnd, m_State.m_iCurrentCursor);
	m_State.SortCursorsFromTopToBottom();
	MergeCursorsIfPossible();
	EnsureCursorVisible(-1, true);
}

bool CTextEditor::FindNextOccurrence(const char* aText, int aTextSize, const Coordinates& aFrom, Coordinates& outStart, Coordinates& outEnd, bool aCaseSensitive)
{
	assert(aTextSize > 0);
	bool fmatches = false;
	int fline, ifline;
	int findex, ifindex;

	ifline = fline = aFrom.m_iLine;
	ifindex = findex = GetCharacterIndexR(aFrom);

	while (true)
	{
		bool matches;
		{ // match function
			int lineOffset = 0;
			int currentCharIndex = findex;
			int i = 0;
			for (; i < aTextSize; i++)
			{
				if (currentCharIndex == m_Lines[fline + lineOffset].size())
				{
					if (aText[i] == '\n' && fline + lineOffset + 1 < m_Lines.size())
					{
						currentCharIndex = 0;
						lineOffset++;
					}
					else
						break;
				}
				else
				{
					char toCompareA = m_Lines[fline + lineOffset][currentCharIndex].m_cChar;
					char toCompareB = aText[i];
					toCompareA = (!aCaseSensitive && toCompareA >= 'A' && toCompareA <= 'Z') ? toCompareA - 'A' + 'a' : toCompareA;
					toCompareB = (!aCaseSensitive && toCompareB >= 'A' && toCompareB <= 'Z') ? toCompareB - 'A' + 'a' : toCompareB;
					if (toCompareA != toCompareB)
						break;
					else
						currentCharIndex++;
				}
			}
			matches = i == aTextSize;
			if (matches)
			{
				outStart = { fline, GetCharacterColumn(fline, findex) };
				outEnd = { fline + lineOffset, GetCharacterColumn(fline + lineOffset, currentCharIndex) };
				return true;
			}
		}

		// move forward
		if (findex == m_Lines[fline].size()) // need to consider line breaks
		{
			if (fline == m_Lines.size() - 1)
			{
				fline = 0;
				findex = 0;
			}
			else
			{
				fline++;
				findex = 0;
			}
		}
		else
			findex++;

		// detect complete scan
		if (findex == ifindex && fline == ifline)
			return false;
	}

	return false;
}

bool CTextEditor::FindMatchingBracket(int aLine, int aCharIndex, Coordinates& out)
{
	if (aLine > m_Lines.size() - 1)
		return false;
	int maxCharIndex = static_cast<int>(m_Lines[aLine].size()) - 1;
	if (aCharIndex > maxCharIndex)
		return false;

	int currentLine = aLine;
	int currentCharIndex = aCharIndex;
	int counter = 1;
	if (CLOSE_TO_OPEN_CHAR.find(m_Lines[aLine][aCharIndex].m_cChar) != CLOSE_TO_OPEN_CHAR.end())
	{
		char closeChar = m_Lines[aLine][aCharIndex].m_cChar;
		char openChar = CLOSE_TO_OPEN_CHAR.at(closeChar);
		while (Move(currentLine, currentCharIndex, true))
		{
			if (currentCharIndex < m_Lines[currentLine].size())
			{
				char currentChar = m_Lines[currentLine][currentCharIndex].m_cChar;
				if (currentChar == openChar)
				{
					counter--;
					if (counter == 0)
					{
						out = { currentLine, GetCharacterColumn(currentLine, currentCharIndex) };
						return true;
					}
				}
				else if (currentChar == closeChar)
					counter++;
			}
		}
	}
	else if (OPEN_TO_CLOSE_CHAR.find(m_Lines[aLine][aCharIndex].m_cChar) != OPEN_TO_CLOSE_CHAR.end())
	{
		char openChar = m_Lines[aLine][aCharIndex].m_cChar;
		char closeChar = OPEN_TO_CLOSE_CHAR.at(openChar);
		while (Move(currentLine, currentCharIndex))
		{
			if (currentCharIndex < m_Lines[currentLine].size())
			{
				char currentChar = m_Lines[currentLine][currentCharIndex].m_cChar;
				if (currentChar == closeChar)
				{
					counter--;
					if (counter == 0)
					{
						out = { currentLine, GetCharacterColumn(currentLine, currentCharIndex) };
						return true;
					}
				}
				else if (currentChar == openChar)
					counter++;
			}
		}
	}
	return false;
}

void CTextEditor::ChangeCurrentLinesIndentation(bool aIncrease)
{
	assert(!m_bReadOnly);

	UndoRecord u;
	u.m_Before = m_State;

	for (int c = m_State.m_iCurrentCursor; c > -1; c--)
	{
		for (int currentLine = m_State.m_Cursors[c].GetSelectionEnd().m_iLine; currentLine >= m_State.m_Cursors[c].GetSelectionStart().m_iLine; currentLine--)
		{
			if (Coordinates{ currentLine, 0 } == m_State.m_Cursors[c].GetSelectionEnd() && m_State.m_Cursors[c].GetSelectionEnd() != m_State.m_Cursors[c].GetSelectionStart()) // when selection ends at line start
				continue;

			if (aIncrease)
			{
				if (m_Lines[currentLine].size() > 0)
				{
					Coordinates lineStart = { currentLine, 0 };
					Coordinates insertionEnd = lineStart;
					InsertTextAt(insertionEnd, "\t"); // sets insertion end
					u.m_Operations.push_back({ "\t", lineStart, insertionEnd, UndoOperationType::Add });
					Colorize(lineStart.m_iLine, 1);
				}
			}
			else
			{
				Coordinates start = { currentLine, 0 };
				Coordinates end = { currentLine, m_iTabSize };
				int charIndex = GetCharacterIndexL(end) - 1;
				while (charIndex > -1 && (m_Lines[currentLine][charIndex].m_cChar == ' ' || m_Lines[currentLine][charIndex].m_cChar == '\t')) charIndex--;
				bool onlySpaceCharactersFound = charIndex == -1;
				if (onlySpaceCharactersFound)
				{
					u.m_Operations.push_back({ GetText(start, end), start, end, UndoOperationType::Delete });
					DeleteRange(start, end);
					Colorize(currentLine, 1);
				}
			}
		}
	}

	if (u.m_Operations.size() > 0)
		AddUndo(u);
}

void CTextEditor::MoveUpCurrentLines()
{
	assert(!m_bReadOnly);

	UndoRecord u;
	u.m_Before = m_State;

	std::set<int> affectedLines;
	int minLine = -1;
	int maxLine = -1;
	for (int c = m_State.m_iCurrentCursor; c > -1; c--) // cursors are expected to be sorted from top to bottom
	{
		for (int currentLine = m_State.m_Cursors[c].GetSelectionEnd().m_iLine; currentLine >= m_State.m_Cursors[c].GetSelectionStart().m_iLine; currentLine--)
		{
			if (Coordinates{ currentLine, 0 } == m_State.m_Cursors[c].GetSelectionEnd() && m_State.m_Cursors[c].GetSelectionEnd() != m_State.m_Cursors[c].GetSelectionStart()) // when selection ends at line start
				continue;
			affectedLines.insert(currentLine);
			minLine = minLine == -1 ? currentLine : (currentLine < minLine ? currentLine : minLine);
			maxLine = maxLine == -1 ? currentLine : (currentLine > maxLine ? currentLine : maxLine);
		}
	}
	if (minLine == 0) // can't move up anymore
		return;

	Coordinates start = { minLine - 1, 0 };
	Coordinates end = { maxLine, GetLineMaxColumn(maxLine) };
	u.m_Operations.push_back({ GetText(start, end), start, end, UndoOperationType::Delete });

	for (int line : affectedLines) // lines should be sorted here
		std::swap(m_Lines[line - 1], m_Lines[line]);
	for (int c = m_State.m_iCurrentCursor; c > -1; c--)
	{
		m_State.m_Cursors[c].m_InteractiveStart.m_iLine -= 1;
		m_State.m_Cursors[c].m_InteractiveEnd.m_iLine -= 1;
		// no need to set m_bCursorPositionChanged as cursors will remain sorted
	}

	end = { maxLine, GetLineMaxColumn(maxLine) }; // this line is swapped with line above, need to find new max column
	u.m_Operations.push_back({ GetText(start, end), start, end, UndoOperationType::Add });
	u.m_After = m_State;
	AddUndo(u);
}

void CTextEditor::MoveDownCurrentLines()
{
	assert(!m_bReadOnly);

	UndoRecord u;
	u.m_Before = m_State;

	std::set<int> affectedLines;
	int minLine = -1;
	int maxLine = -1;
	for (int c = 0; c <= m_State.m_iCurrentCursor; c++) // cursors are expected to be sorted from top to bottom
	{
		for (int currentLine = m_State.m_Cursors[c].GetSelectionEnd().m_iLine; currentLine >= m_State.m_Cursors[c].GetSelectionStart().m_iLine; currentLine--)
		{
			if (Coordinates{ currentLine, 0 } == m_State.m_Cursors[c].GetSelectionEnd() && m_State.m_Cursors[c].GetSelectionEnd() != m_State.m_Cursors[c].GetSelectionStart()) // when selection ends at line start
				continue;
			affectedLines.insert(currentLine);
			minLine = minLine == -1 ? currentLine : (currentLine < minLine ? currentLine : minLine);
			maxLine = maxLine == -1 ? currentLine : (currentLine > maxLine ? currentLine : maxLine);
		}
	}
	if (maxLine == m_Lines.size() - 1) // can't move down anymore
		return;

	Coordinates start = { minLine, 0 };
	Coordinates end = { maxLine + 1, GetLineMaxColumn(maxLine + 1)};
	u.m_Operations.push_back({ GetText(start, end), start, end, UndoOperationType::Delete });

	std::set<int>::reverse_iterator rit;
	for (rit = affectedLines.rbegin(); rit != affectedLines.rend(); rit++) // lines should be sorted here
		std::swap(m_Lines[*rit + 1], m_Lines[*rit]);
	for (int c = m_State.m_iCurrentCursor; c > -1; c--)
	{
		m_State.m_Cursors[c].m_InteractiveStart.m_iLine += 1;
		m_State.m_Cursors[c].m_InteractiveEnd.m_iLine += 1;
		// no need to set m_bCursorPositionChanged as cursors will remain sorted
	}

	end = { maxLine + 1, GetLineMaxColumn(maxLine + 1) }; // this line is swapped with line below, need to find new max column
	u.m_Operations.push_back({ GetText(start, end), start, end, UndoOperationType::Add });
	u.m_After = m_State;
	AddUndo(u);
}

void CTextEditor::ToggleLineComment()
{
	assert(!m_bReadOnly);
	if (m_pLanguageDefinition == nullptr)
		return;
	const std::string& commentString = m_pLanguageDefinition->m_SingleLineComment;

	UndoRecord u;
	u.m_Before = m_State;

	bool shouldAddComment = false;
	std::unordered_set<int> affectedLines;
	for (int c = m_State.m_iCurrentCursor; c > -1; c--)
	{
		for (int currentLine = m_State.m_Cursors[c].GetSelectionEnd().m_iLine; currentLine >= m_State.m_Cursors[c].GetSelectionStart().m_iLine; currentLine--)
		{
			if (Coordinates{ currentLine, 0 } == m_State.m_Cursors[c].GetSelectionEnd() && m_State.m_Cursors[c].GetSelectionEnd() != m_State.m_Cursors[c].GetSelectionStart()) // when selection ends at line start
				continue;
			affectedLines.insert(currentLine);
			int currentIndex = 0;
			while (currentIndex < m_Lines[currentLine].size() && (m_Lines[currentLine][currentIndex].m_cChar == ' ' || m_Lines[currentLine][currentIndex].m_cChar == '\t')) currentIndex++;
			if (currentIndex == m_Lines[currentLine].size())
				continue;
			int i = 0;
			while (i < commentString.length() && currentIndex + i < m_Lines[currentLine].size() && m_Lines[currentLine][currentIndex + i].m_cChar == commentString[i]) i++;
			bool matched = i == commentString.length();
			shouldAddComment |= !matched;
		}
	}

	if (shouldAddComment)
	{
		for (int currentLine : affectedLines) // order doesn't matter as changes are not multiline
		{
			Coordinates lineStart = { currentLine, 0 };
			Coordinates insertionEnd = lineStart;
			InsertTextAt(insertionEnd, (commentString + ' ').c_str()); // sets insertion end
			u.m_Operations.push_back({ (commentString + ' ') , lineStart, insertionEnd, UndoOperationType::Add });
			Colorize(lineStart.m_iLine, 1);
		}
	}
	else
	{
		for (int currentLine : affectedLines) // order doesn't matter as changes are not multiline
		{
			int currentIndex = 0;
			while (currentIndex < m_Lines[currentLine].size() && (m_Lines[currentLine][currentIndex].m_cChar == ' ' || m_Lines[currentLine][currentIndex].m_cChar == '\t')) currentIndex++;
			if (currentIndex == m_Lines[currentLine].size())
				continue;
			int i = 0;
			while (i < commentString.length() && currentIndex + i < m_Lines[currentLine].size() && m_Lines[currentLine][currentIndex + i].m_cChar == commentString[i]) i++;
			bool matched = i == commentString.length();
			assert(matched);
			if (currentIndex + i < m_Lines[currentLine].size() && m_Lines[currentLine][currentIndex + i].m_cChar == ' ')
				i++;

			Coordinates start = { currentLine, GetCharacterColumn(currentLine, currentIndex) };
			Coordinates end = { currentLine, GetCharacterColumn(currentLine, currentIndex + i) };
			u.m_Operations.push_back({ GetText(start, end) , start, end, UndoOperationType::Delete});
			DeleteRange(start, end);
			Colorize(currentLine, 1);
		}
	}

	u.m_After = m_State;
	AddUndo(u);
}

void CTextEditor::RemoveCurrentLines()
{
	UndoRecord u;
	u.m_Before = m_State;

	if (AnyCursorHasSelection())
	{
		for (int c = m_State.m_iCurrentCursor; c > -1; c--)
		{
			if (!m_State.m_Cursors[c].HasSelection())
				continue;
			u.m_Operations.push_back({ GetSelectedText(c), m_State.m_Cursors[c].GetSelectionStart(), m_State.m_Cursors[c].GetSelectionEnd(), UndoOperationType::Delete });
			DeleteSelection(c);
		}
	}
	MoveHome();
	OnCursorPositionChanged(); // might combine cursors

	for (int c = m_State.m_iCurrentCursor; c > -1; c--)
	{
		int currentLine = m_State.m_Cursors[c].m_InteractiveEnd.m_iLine;
		int nextLine = currentLine + 1;
		int prevLine = currentLine - 1;

		Coordinates toDeleteStart, toDeleteEnd;
		if (m_Lines.size() > nextLine) // next line exists
		{
			toDeleteStart = Coordinates(currentLine, 0);
			toDeleteEnd = Coordinates(nextLine, 0);
			SetCursorPosition({ m_State.m_Cursors[c].m_InteractiveEnd.m_iLine, 0 }, c);
		}
		else if (prevLine > -1) // previous line exists
		{
			toDeleteStart = Coordinates(prevLine, GetLineMaxColumn(prevLine));
			toDeleteEnd = Coordinates(currentLine, GetLineMaxColumn(currentLine));
			SetCursorPosition({ prevLine, 0 }, c);
		}
		else
		{
			toDeleteStart = Coordinates(currentLine, 0);
			toDeleteEnd = Coordinates(currentLine, GetLineMaxColumn(currentLine));
			SetCursorPosition({ currentLine, 0 }, c);
		}

		u.m_Operations.push_back({ GetText(toDeleteStart, toDeleteEnd), toDeleteStart, toDeleteEnd, UndoOperationType::Delete });

		std::unordered_set<int> handledCursors = { c };
		if (toDeleteStart.m_iLine != toDeleteEnd.m_iLine)
			RemoveLine(currentLine, &handledCursors);
		else
			DeleteRange(toDeleteStart, toDeleteEnd);
	}

	u.m_After = m_State;
	AddUndo(u);
}

float CTextEditor::TextDistanceToLineStart(const Coordinates& aFrom, bool aSanitizeCoords) const
{
	if (aSanitizeCoords)
		return SanitizeCoordinates(aFrom).m_iColumn * m_CharAdvance.x;
	else
		return aFrom.m_iColumn * m_CharAdvance.x;
}

void CTextEditor::EnsureCursorVisible(int aCursor, bool StartToo)
{
	if (aCursor == -1)
		aCursor = m_State.GetLastAddedCursorIndex();

	m_iEnsureCursorVisible = aCursor;
	m_bEnsureCursorVisibleStartToo = StartToo;
	return;
}

CTextEditor::Coordinates CTextEditor::SanitizeCoordinates(const Coordinates& aValue) const
{
	auto line = aValue.m_iLine;
	auto column = aValue.m_iColumn;
	if (line >= (int) m_Lines.size())
	{
		if (m_Lines.empty())
		{
			line = 0;
			column = 0;
		}
		else
		{
			line = (int) m_Lines.size() - 1;
			column = GetLineMaxColumn(line);
		}
		return Coordinates(line, column);
	}
	else
	{
		column = m_Lines.empty() ? 0 : GetLineMaxColumn(line, column);
		return Coordinates(line, column);
	}
}

CTextEditor::Coordinates CTextEditor::GetActualCursorCoordinates(int aCursor, bool Start) const
{
	if (aCursor == -1)
		return SanitizeCoordinates(Start ? m_State.m_Cursors[m_State.m_iCurrentCursor].m_InteractiveStart : m_State.m_Cursors[m_State.m_iCurrentCursor].m_InteractiveEnd);
	else
		return SanitizeCoordinates(Start ? m_State.m_Cursors[aCursor].m_InteractiveStart : m_State.m_Cursors[aCursor].m_InteractiveEnd);
}

CTextEditor::Coordinates CTextEditor::ScreenPosToCoordinates(const ImVec2& aPosition, bool aInsertionMode, bool* isOverLineNumber) const
{
	ImVec2 origin = ImGui::GetCursorScreenPos();
	ImVec2 local(aPosition.x - origin.x + 3.0f, aPosition.y - origin.y);

	if (isOverLineNumber != nullptr)
		*isOverLineNumber = local.x < m_fTextStart;

	Coordinates out = {
		Max(0, (int)floor(local.y / m_CharAdvance.y)),
		Max(0, (int)floor((local.x - m_fTextStart) / m_CharAdvance.x))
	};
	int charIndex = GetCharacterIndexL(out);
	if (charIndex > -1 && charIndex < m_Lines[out.m_iLine].size() && m_Lines[out.m_iLine][charIndex].m_cChar == '\t')
	{
		int columnToLeft = GetCharacterColumn(out.m_iLine, charIndex);
		int columnToRight = GetCharacterColumn(out.m_iLine, GetCharacterIndexR(out));
		if (out.m_iColumn - columnToLeft < columnToRight - out.m_iColumn)
			out.m_iColumn = columnToLeft;
		else
			out.m_iColumn = columnToRight;
	}
	else
		out.m_iColumn = Max(0, (int)floor((local.x - m_fTextStart + POS_TO_COORDS_COLUMN_OFFSET * m_CharAdvance.x) / m_CharAdvance.x));
	return SanitizeCoordinates(out);
}

CTextEditor::Coordinates CTextEditor::FindWordStart(const Coordinates& aFrom) const
{
	if (aFrom.m_iLine >= (int)m_Lines.size())
		return aFrom;

	int lineIndex = aFrom.m_iLine;
	auto& line = m_Lines[lineIndex];
	int charIndex = GetCharacterIndexL(aFrom);

	if (charIndex > (int)line.size() || line.size() == 0)
		return aFrom;
	if (charIndex == (int)line.size())
		charIndex--;

	bool initialIsWordChar = CharIsWordChar(line[charIndex].m_cChar);
	bool initialIsSpace = isspace(line[charIndex].m_cChar);
	char initialChar = line[charIndex].m_cChar;
	while (Move(lineIndex, charIndex, true, true))
	{
		bool isWordChar = CharIsWordChar(line[charIndex].m_cChar);
		bool isSpace = isspace(line[charIndex].m_cChar);
		if (initialIsSpace && !isSpace ||
			initialIsWordChar && !isWordChar ||
			!initialIsWordChar && !initialIsSpace && initialChar != line[charIndex].m_cChar)
		{
			Move(lineIndex, charIndex, false, true); // one step to the right
			break;
		}
	}
	return { aFrom.m_iLine, GetCharacterColumn(aFrom.m_iLine, charIndex) };
}

CTextEditor::Coordinates CTextEditor::FindWordEnd(const Coordinates& aFrom) const
{
	if (aFrom.m_iLine >= (int)m_Lines.size())
		return aFrom;

	int lineIndex = aFrom.m_iLine;
	auto& line = m_Lines[lineIndex];
	auto charIndex = GetCharacterIndexL(aFrom);

	if (charIndex >= (int)line.size())
		return aFrom;

	bool initialIsWordChar = CharIsWordChar(line[charIndex].m_cChar);
	bool initialIsSpace = isspace(line[charIndex].m_cChar);
	char initialChar = line[charIndex].m_cChar;
	while (Move(lineIndex, charIndex, false, true))
	{
		if (charIndex == line.size())
			break;
		bool isWordChar = CharIsWordChar(line[charIndex].m_cChar);
		bool isSpace = isspace(line[charIndex].m_cChar);
		if (initialIsSpace && !isSpace ||
			initialIsWordChar && !isWordChar ||
			!initialIsWordChar && !initialIsSpace && initialChar != line[charIndex].m_cChar)
			break;
	}
	return { lineIndex, GetCharacterColumn(aFrom.m_iLine, charIndex) };
}

int CTextEditor::GetCharacterIndexL(const Coordinates& Coords) const
{
	if (Coords.m_iLine >= m_Lines.size())
		return -1;

	auto& line = m_Lines[Coords.m_iLine];
	int c = 0;
	int i = 0;
	int tabCoordsLeft = 0;

	for (; i < line.size() && c < Coords.m_iColumn;)
	{
		if (line[i].m_cChar == '\t')
		{
			if (tabCoordsLeft == 0)
				tabCoordsLeft = TabSizeAtColumn(c);
			if (tabCoordsLeft > 0)
				tabCoordsLeft--;
			c++;
		}
		else
			++c;
		if (tabCoordsLeft == 0)
			i += UTF8CharLength(line[i].m_cChar);
	}
	return i;
}

int CTextEditor::GetCharacterIndexR(const Coordinates& Coords) const
{
	if (Coords.m_iLine >= m_Lines.size())
		return -1;
	int c = 0;
	int i = 0;
	for (; i < m_Lines[Coords.m_iLine].size() && c < Coords.m_iColumn;)
		MoveCharIndexAndColumn(Coords.m_iLine, i, c);
	return i;
}

int CTextEditor::GetCharacterColumn(int aLine, int aIndex) const
{
	if (aLine >= m_Lines.size())
		return 0;
	int c = 0;
	int i = 0;
	while (i < aIndex && i < m_Lines[aLine].size())
		MoveCharIndexAndColumn(aLine, i, c);
	return c;
}

int CTextEditor::GetFirstVisibleCharacterIndex(int aLine) const
{
	if (aLine >= m_Lines.size())
		return 0;
	int c = 0;
	int i = 0;
	while (c < m_iFirstVisibleColumn && i < m_Lines[aLine].size())
		MoveCharIndexAndColumn(aLine, i, c);
	if (c > m_iFirstVisibleColumn)
		i--;
	return i;
}

int CTextEditor::GetLineMaxColumn(int aLine, int aLimit) const
{
	if (aLine >= m_Lines.size())
		return 0;
	int c = 0;
	if (aLimit == -1)
	{
		for (int i = 0; i < m_Lines[aLine].size(); )
			MoveCharIndexAndColumn(aLine, i, c);
	}
	else
	{
		for (int i = 0; i < m_Lines[aLine].size(); )
		{
			MoveCharIndexAndColumn(aLine, i, c);
			if (c > aLimit)
				return aLimit;
		}
	}
	return c;
}

CTextEditor::Line& CTextEditor::InsertLine(int aIndex)
{
	assert(!m_bReadOnly);
	auto& result = *m_Lines.insert(m_Lines.begin() + aIndex, Line());

	for (int c = 0; c <= m_State.m_iCurrentCursor; c++) // handle multiple cursors
	{
		if (m_State.m_Cursors[c].m_InteractiveEnd.m_iLine >= aIndex)
			SetCursorPosition({ m_State.m_Cursors[c].m_InteractiveEnd.m_iLine + 1, m_State.m_Cursors[c].m_InteractiveEnd.m_iColumn }, c);
	}

	return result;
}

void CTextEditor::RemoveLine(int aIndex, const std::unordered_set<int>* aHandledCursors)
{
	assert(!m_bReadOnly);
	assert(m_Lines.size() > 1);

	m_Lines.erase(m_Lines.begin() + aIndex);
	assert(!m_Lines.empty());

	// handle multiple cursors
	for (int c = 0; c <= m_State.m_iCurrentCursor; c++)
	{
		if (m_State.m_Cursors[c].m_InteractiveEnd.m_iLine >= aIndex)
		{
			if (aHandledCursors == nullptr || aHandledCursors->find(c) == aHandledCursors->end()) // move up if has not been handled already
				SetCursorPosition({ m_State.m_Cursors[c].m_InteractiveEnd.m_iLine - 1, m_State.m_Cursors[c].m_InteractiveEnd.m_iColumn }, c);
		}
	}
}

void CTextEditor::RemoveLines(int Start, int End)
{
	assert(!m_bReadOnly);
	assert(End >= Start);
	assert(m_Lines.size() > (size_t)(End - Start));

	m_Lines.erase(m_Lines.begin() + Start, m_Lines.begin() + End);
	assert(!m_Lines.empty());

	// handle multiple cursors
	for (int c = 0; c <= m_State.m_iCurrentCursor; c++)
	{
		if (m_State.m_Cursors[c].m_InteractiveEnd.m_iLine >= Start)
		{
			int targetLine = m_State.m_Cursors[c].m_InteractiveEnd.m_iLine - (End - Start);
			targetLine = targetLine < 0 ? 0 : targetLine;
			m_State.m_Cursors[c].m_InteractiveEnd.m_iLine = targetLine;
		}
		if (m_State.m_Cursors[c].m_InteractiveStart.m_iLine >= Start)
		{
			int targetLine = m_State.m_Cursors[c].m_InteractiveStart.m_iLine - (End - Start);
			targetLine = targetLine < 0 ? 0 : targetLine;
			m_State.m_Cursors[c].m_InteractiveStart.m_iLine = targetLine;
		}
	}
}

void CTextEditor::DeleteRange(const Coordinates& Start, const Coordinates& End)
{
	assert(End >= Start);
	assert(!m_bReadOnly);

	if (End == Start)
		return;

	auto start = GetCharacterIndexL(Start);
	auto end = GetCharacterIndexR(End);

	if (Start.m_iLine == End.m_iLine)
	{
		auto n = GetLineMaxColumn(Start.m_iLine);
		if (End.m_iColumn >= n)
			RemoveGlyphsFromLine(Start.m_iLine, start); // from start to end of line
		else
			RemoveGlyphsFromLine(Start.m_iLine, start, end);
	}
	else
	{
		RemoveGlyphsFromLine(Start.m_iLine, start); // from start to end of line
		RemoveGlyphsFromLine(End.m_iLine, 0, end);
		auto& firstLine = m_Lines[Start.m_iLine];
		auto& lastLine = m_Lines[End.m_iLine];

		if (Start.m_iLine < End.m_iLine)
		{
			AddGlyphsToLine(Start.m_iLine, static_cast<int>(firstLine.size()), lastLine.begin(), lastLine.end());
			for (int c = 0; c <= m_State.m_iCurrentCursor; c++) // move up cursors in line that is being moved up
			{
				// if cursor is selecting the same range we are deleting, it's because this is being called from
				// DeleteSelection which already sets the cursor position after the range is deleted
				if (m_State.m_Cursors[c].GetSelectionStart() == Start && m_State.m_Cursors[c].GetSelectionEnd() == End)
					continue;
				if (m_State.m_Cursors[c].m_InteractiveEnd.m_iLine > End.m_iLine)
					break;
				else if (m_State.m_Cursors[c].m_InteractiveEnd.m_iLine != End.m_iLine)
					continue;
				int otherCursorEndCharIndex = GetCharacterIndexR(m_State.m_Cursors[c].m_InteractiveEnd);
				int otherCursorStartCharIndex = GetCharacterIndexR(m_State.m_Cursors[c].m_InteractiveStart);
				int otherCursorNewEndCharIndex = GetCharacterIndexR(Start) + otherCursorEndCharIndex;
				int otherCursorNewStartCharIndex = GetCharacterIndexR(Start) + otherCursorStartCharIndex;
				auto targetEndCoords = Coordinates(Start.m_iLine, GetCharacterColumn(Start.m_iLine, otherCursorNewEndCharIndex));
				auto targetStartCoords = Coordinates(Start.m_iLine, GetCharacterColumn(Start.m_iLine, otherCursorNewStartCharIndex));
				SetCursorPosition(targetStartCoords, c, true);
				SetCursorPosition(targetEndCoords, c, false);
			}
			RemoveLines(Start.m_iLine + 1, End.m_iLine + 1);
		}
	}
}

void CTextEditor::DeleteSelection(int aCursor)
{
	if (aCursor == -1)
		aCursor = m_State.m_iCurrentCursor;

	if (m_State.m_Cursors[aCursor].GetSelectionEnd() == m_State.m_Cursors[aCursor].GetSelectionStart())
		return;

	Coordinates newCursorPos = m_State.m_Cursors[aCursor].GetSelectionStart();
	DeleteRange(newCursorPos, m_State.m_Cursors[aCursor].GetSelectionEnd());
	SetCursorPosition(newCursorPos, aCursor);
	Colorize(newCursorPos.m_iLine, 1);
}

void CTextEditor::RemoveGlyphsFromLine(int aLine, int StartChar, int EndChar)
{
	int column = GetCharacterColumn(aLine, StartChar);
	auto& line = m_Lines[aLine];
	OnLineChanged(true, aLine, column, EndChar - StartChar, true);
	line.erase(line.begin() + StartChar, EndChar == -1 ? line.end() : line.begin() + EndChar);
	OnLineChanged(false, aLine, column, EndChar - StartChar, true);
}

void CTextEditor::AddGlyphsToLine(int aLine, int aTargetIndex, Line::iterator aSourceStart, Line::iterator aSourceEnd)
{
	int targetColumn = GetCharacterColumn(aLine, aTargetIndex);
	int charsInserted = static_cast<int>(std::distance(aSourceStart, aSourceEnd));
	auto& line = m_Lines[aLine];
	OnLineChanged(true, aLine, targetColumn, charsInserted, false);
	line.insert(line.begin() + aTargetIndex, aSourceStart, aSourceEnd);
	OnLineChanged(false, aLine, targetColumn, charsInserted, false);
}

void CTextEditor::AddGlyphToLine(int aLine, int aTargetIndex, Glyph aGlyph)
{
	int targetColumn = GetCharacterColumn(aLine, aTargetIndex);
	auto& line = m_Lines[aLine];
	OnLineChanged(true, aLine, targetColumn, 1, false);
	line.insert(line.begin() + aTargetIndex, aGlyph);
	OnLineChanged(false, aLine, targetColumn, 1, false);
}

ImU32 CTextEditor::GetGlyphColor(const Glyph& aGlyph) const
{
	if (m_pLanguageDefinition == nullptr)
		return m_Palette[(int)PaletteIndex::Default];
	if (aGlyph.m_bComment)
		return m_Palette[(int)PaletteIndex::Comment];
	if (aGlyph.m_bMultiLineComment)
		return m_Palette[(int)PaletteIndex::MultiLineComment];
	auto const color = m_Palette[(int)aGlyph.m_ColorIndex];
	if (aGlyph.m_bPreprocessor)
	{
		const auto ppcolor = m_Palette[(int)PaletteIndex::Preprocessor];
		const int c0 = ((ppcolor & 0xff) + (color & 0xff)) / 2;
		const int c1 = (((ppcolor >> 8) & 0xff) + ((color >> 8) & 0xff)) / 2;
		const int c2 = (((ppcolor >> 16) & 0xff) + ((color >> 16) & 0xff)) / 2;
		const int c3 = (((ppcolor >> 24) & 0xff) + ((color >> 24) & 0xff)) / 2;
		return ImU32(c0 | (c1 << 8) | (c2 << 16) | (c3 << 24));
	}
	return color;
}

void CTextEditor::HandleKeyboardInputs(bool aParentIsFocused)
{
	if (ImGui::IsWindowFocused() || aParentIsFocused)
	{
		if (ImGui::IsWindowHovered())
			ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);
		//ImGui::CaptureKeyboardFromApp(true);

		ImGuiIO& io = ImGui::GetIO();
		auto isOSX = io.ConfigMacOSXBehaviors;
		auto alt = io.KeyAlt;
		auto ctrl = io.KeyCtrl;
		auto shift = io.KeyShift;
		auto super = io.KeySuper;

		auto isShortcut = (isOSX ? (super && !ctrl) : (ctrl && !super)) && !alt && !shift;
		auto isShiftShortcut = (isOSX ? (super && !ctrl) : (ctrl && !super)) && shift && !alt;
		auto isWordmoveKey = isOSX ? alt : ctrl;
		auto isAltOnly = alt && !ctrl && !shift && !super;
		auto isCtrlOnly = ctrl && !alt && !shift && !super;
		auto isShiftOnly = shift && !alt && !ctrl && !super;

		io.WantCaptureKeyboard = true;
		io.WantTextInput = true;

		if (!m_bReadOnly && isShortcut && ImGui::IsKeyPressed(ImGuiKey_Z))
			Undo();
		else if (!m_bReadOnly && isAltOnly && ImGui::IsKeyPressed(ImGuiKey_Backspace))
			Undo();
		else if (!m_bReadOnly && isShortcut && ImGui::IsKeyPressed(ImGuiKey_Y))
			Redo();
		else if (!m_bReadOnly && isShiftShortcut && ImGui::IsKeyPressed(ImGuiKey_Z))
			Redo();
		else if (!alt && !ctrl && !super && ImGui::IsKeyPressed(ImGuiKey_UpArrow))
			MoveUp(1, shift);
		else if (!alt && !ctrl && !super && ImGui::IsKeyPressed(ImGuiKey_DownArrow))
			MoveDown(1, shift);
		else if ((isOSX ? !ctrl : !alt) && !super && ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
			MoveLeft(shift, isWordmoveKey);
		else if ((isOSX ? !ctrl : !alt) && !super && ImGui::IsKeyPressed(ImGuiKey_RightArrow))
			MoveRight(shift, isWordmoveKey);
		else if (!alt && !ctrl && !super && ImGui::IsKeyPressed(ImGuiKey_PageUp))
			MoveUp(m_iVisibleLineCount - 2, shift);
		else if (!alt && !ctrl && !super && ImGui::IsKeyPressed(ImGuiKey_PageDown))
			MoveDown(m_iVisibleLineCount - 2, shift);
		else if (ctrl && !alt && !super && ImGui::IsKeyPressed(ImGuiKey_Home))
			MoveTop(shift);
		else if (ctrl && !alt && !super && ImGui::IsKeyPressed(ImGuiKey_End))
			MoveBottom(shift);
		else if (!alt && !ctrl && !super && ImGui::IsKeyPressed(ImGuiKey_Home))
			MoveHome(shift);
		else if (!alt && !ctrl && !super && ImGui::IsKeyPressed(ImGuiKey_End))
			MoveEnd(shift);
		else if (!m_bReadOnly && !alt && !shift && !super && ImGui::IsKeyPressed(ImGuiKey_Delete))
			Delete(ctrl);
		else if (!m_bReadOnly && !alt && !shift && !super && ImGui::IsKeyPressed(ImGuiKey_Backspace))
			Backspace(ctrl);
		else if (!m_bReadOnly && !alt && ctrl && shift && !super && ImGui::IsKeyPressed(ImGuiKey_K))
			RemoveCurrentLines();
		else if (!m_bReadOnly && !alt && ctrl && !shift && !super && ImGui::IsKeyPressed(ImGuiKey_LeftBracket))
			ChangeCurrentLinesIndentation(false);
		else if (!m_bReadOnly && !alt && ctrl && !shift && !super && ImGui::IsKeyPressed(ImGuiKey_RightBracket))
			ChangeCurrentLinesIndentation(true);
		else if (!alt && ctrl && shift && !super && ImGui::IsKeyPressed(ImGuiKey_UpArrow))
			MoveUpCurrentLines();
		else if (!alt && ctrl && shift && !super && ImGui::IsKeyPressed(ImGuiKey_DownArrow))
			MoveDownCurrentLines();
		else if (!m_bReadOnly && !alt && ctrl && !shift && !super && ImGui::IsKeyPressed(ImGuiKey_Slash))
			ToggleLineComment();
		else if (!alt && !ctrl && !shift && !super && ImGui::IsKeyPressed(ImGuiKey_Insert))
			m_bOverwrite ^= true;
		else if (isCtrlOnly && ImGui::IsKeyPressed(ImGuiKey_Insert))
			Copy();
		else if (isShortcut && ImGui::IsKeyPressed(ImGuiKey_C))
			Copy();
		else if (!m_bReadOnly && isShiftOnly && ImGui::IsKeyPressed(ImGuiKey_Insert))
			Paste();
		else if (!m_bReadOnly && isShortcut && ImGui::IsKeyPressed(ImGuiKey_V))
			Paste();
		else if (isShortcut && ImGui::IsKeyPressed(ImGuiKey_X))
			Cut();
		else if (isShiftOnly && ImGui::IsKeyPressed(ImGuiKey_Delete))
			Cut();
		else if (isShortcut && ImGui::IsKeyPressed(ImGuiKey_A))
			SelectAll();
		else if (isShortcut && ImGui::IsKeyPressed(ImGuiKey_D))
			AddCursorForNextOccurrence();
        else if (!m_bReadOnly && !alt && !ctrl && !shift && !super && (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)))
			EnterCharacter('\n', false);
		else if (!m_bReadOnly && !alt && !ctrl && !super && ImGui::IsKeyPressed(ImGuiKey_Tab))
			EnterCharacter('\t', shift);
		if (!m_bReadOnly && !io.InputQueueCharacters.empty() && ctrl == alt && !super)
		{
			for (int i = 0; i < io.InputQueueCharacters.Size; i++)
			{
				auto c = io.InputQueueCharacters[i];
				if (c != 0 && (c == '\n' || c >= 32))
					EnterCharacter(c, shift);
			}
			io.InputQueueCharacters.resize(0);
		}
	}
}

void CTextEditor::HandleMouseInputs()
{
	ImGuiIO& io = ImGui::GetIO();
	auto shift = io.KeyShift;
	auto ctrl = io.ConfigMacOSXBehaviors ? io.KeySuper : io.KeyCtrl;
	auto alt = io.ConfigMacOSXBehaviors ? io.KeyCtrl : io.KeyAlt;

	/*
	Pan with middle mouse button
	*/
	m_bPanning &= ImGui::IsMouseDown(2);
	if (m_bPanning && ImGui::IsMouseDragging(2))
	{
		ImVec2 scroll = { ImGui::GetScrollX(), ImGui::GetScrollY() };
		ImVec2 currentMousePos = ImGui::GetMouseDragDelta(2);
		ImVec2 mouseDelta = {
			currentMousePos.x - m_LastMousePos.x,
			currentMousePos.y - m_LastMousePos.y
		};
		ImGui::SetScrollY(scroll.y - mouseDelta.y);
		ImGui::SetScrollX(scroll.x - mouseDelta.x);
		m_LastMousePos = currentMousePos;
	}

	// Mouse left button dragging (=> update selection)
	m_bDraggingSelection &= ImGui::IsMouseDown(0);
	if (m_bDraggingSelection && ImGui::IsMouseDragging(0))
	{
		io.WantCaptureMouse = true;
		Coordinates cursorCoords = ScreenPosToCoordinates(ImGui::GetMousePos(), !m_bOverwrite);
		SetCursorPosition(cursorCoords, m_State.GetLastAddedCursorIndex(), false);
	}

	if (ImGui::IsWindowHovered())
	{
		auto click = ImGui::IsMouseClicked(0);
		if (!shift && !alt)
		{
			auto doubleClick = ImGui::IsMouseDoubleClicked(0);
			auto t = ImGui::GetTime();
			auto tripleClick = click && !doubleClick &&
				(m_fLastClickTime != -1.0f && (t - m_fLastClickTime) < io.MouseDoubleClickTime &&
					Distance(io.MousePos, m_LastClickPos) < 0.01f);

			if (click)
				m_bDraggingSelection = true;

			/*
			Pan with middle mouse button
			*/

			if (ImGui::IsMouseClicked(2))
			{
				m_bPanning = true;
				m_LastMousePos = ImGui::GetMouseDragDelta(2);
			}

			/*
			Left mouse button triple click
			*/

			if (tripleClick)
			{
				if (ctrl)
					m_State.AddCursor();
				else
					m_State.m_iCurrentCursor = 0;

				Coordinates cursorCoords = ScreenPosToCoordinates(ImGui::GetMousePos());
				Coordinates targetCursorPos = cursorCoords.m_iLine < m_Lines.size() - 1 ?
					Coordinates{ cursorCoords.m_iLine + 1, 0 } :
					Coordinates{ cursorCoords.m_iLine, GetLineMaxColumn(cursorCoords.m_iLine) };
				SetSelection({ cursorCoords.m_iLine, 0 }, targetCursorPos, m_State.m_iCurrentCursor);

				m_fLastClickTime = -1.0f;
			}

			/*
			Left mouse button double click
			*/

			else if (doubleClick)
			{
				if (ctrl)
					m_State.AddCursor();
				else
					m_State.m_iCurrentCursor = 0;

				Coordinates cursorCoords = ScreenPosToCoordinates(ImGui::GetMousePos());
				SetSelection(FindWordStart(cursorCoords), FindWordEnd(cursorCoords), m_State.m_iCurrentCursor);

				m_fLastClickTime = (float)ImGui::GetTime();
				m_LastClickPos = io.MousePos;
			}

			/*
			Left mouse button click
			*/
			else if (click)
			{
				if (ctrl)
					m_State.AddCursor();
				else
					m_State.m_iCurrentCursor = 0;

				bool isOverLineNumber;
				Coordinates cursorCoords = ScreenPosToCoordinates(ImGui::GetMousePos(), !m_bOverwrite, &isOverLineNumber);
				if (isOverLineNumber)
				{
					Coordinates targetCursorPos = cursorCoords.m_iLine < m_Lines.size() - 1 ?
						Coordinates{ cursorCoords.m_iLine + 1, 0 } :
						Coordinates{ cursorCoords.m_iLine, GetLineMaxColumn(cursorCoords.m_iLine) };
					SetSelection({ cursorCoords.m_iLine, 0 }, targetCursorPos, m_State.m_iCurrentCursor);
				}
				else
					SetCursorPosition(cursorCoords, m_State.GetLastAddedCursorIndex());

				m_fLastClickTime = (float)ImGui::GetTime();
				m_LastClickPos = io.MousePos;
			}
			else if (ImGui::IsMouseReleased(0))
			{
				m_State.SortCursorsFromTopToBottom();
				MergeCursorsIfPossible();
			}
		}
		else if (shift)
		{
			if (click)
			{
				Coordinates newSelection = ScreenPosToCoordinates(ImGui::GetMousePos(), !m_bOverwrite);
				SetCursorPosition(newSelection, m_State.m_iCurrentCursor, false);
			}
		}
	}
}

void CTextEditor::UpdateViewVariables(float aScrollX, float aScrollY)
{
	m_fContentHeight = ImGui::GetWindowHeight() - (IsHorizontalScrollbarVisible() ? IMGUI_SCROLLBAR_WIDTH : 0.0f);
	m_fContentWidth = ImGui::GetWindowWidth() - (IsVerticalScrollbarVisible() ? IMGUI_SCROLLBAR_WIDTH : 0.0f);

	m_iVisibleLineCount = Max((int)ceil(m_fContentHeight / m_CharAdvance.y), 0);
	m_iFirstVisibleLine = Max((int)(aScrollY / m_CharAdvance.y), 0);
	m_iLastVisibleLine = Max((int)((m_fContentHeight + aScrollY) / m_CharAdvance.y), 0);

	m_iVisibleColumnCount = Max((int)ceil((m_fContentWidth - Max(m_fTextStart - aScrollX, 0.0f)) / m_CharAdvance.x), 0);
	m_iFirstVisibleColumn = Max((int)(Max(aScrollX - m_fTextStart, 0.0f) / m_CharAdvance.x), 0);
	m_iLastVisibleColumn = Max((int)((m_fContentWidth + aScrollX - m_fTextStart) / m_CharAdvance.x), 0);
}

void CTextEditor::Render(bool aParentIsFocused)
{
	/* Compute m_CharAdvance regarding to scaled font size (Ctrl + mouse wheel)*/
	const float fontWidth = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, "#", nullptr, nullptr).x;
	const float fontHeight = ImGui::GetTextLineHeightWithSpacing();
	m_CharAdvance = ImVec2(fontWidth, fontHeight * m_fLineSpacing);

	// Deduce m_fTextStart by evaluating m_Lines size (global lineMax) plus two spaces as text width
	m_fTextStart = static_cast<float>(m_iLeftMargin);
	static char lineNumberBuffer[16];
	if (m_bShowLineNumbers)
	{
		snprintf(lineNumberBuffer, 16, " %llu ", m_Lines.size());
		m_fTextStart += ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, lineNumberBuffer, nullptr, nullptr).x;
	}

	ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();
	m_fScrollX = ImGui::GetScrollX();
	m_fScrollY = ImGui::GetScrollY();
	UpdateViewVariables(m_fScrollX, m_fScrollY);

	int maxColumnLimited = 0;
	if (!m_Lines.empty())
	{
		auto drawList = ImGui::GetWindowDrawList();
		float spaceSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, " ", nullptr, nullptr).x;

		for (int lineNo = m_iFirstVisibleLine; lineNo <= m_iLastVisibleLine && lineNo < m_Lines.size(); lineNo++)
		{
			ImVec2 lineStartScreenPos = ImVec2(cursorScreenPos.x, cursorScreenPos.y + lineNo * m_CharAdvance.y);
			ImVec2 textScreenPos = ImVec2(lineStartScreenPos.x + m_fTextStart, lineStartScreenPos.y);

			auto& line = m_Lines[lineNo];
			maxColumnLimited = Max(GetLineMaxColumn(lineNo, m_iLastVisibleColumn), maxColumnLimited);

			Coordinates lineStartCoord(lineNo, 0);
			Coordinates lineEndCoord(lineNo, maxColumnLimited);

			// Draw selection for the current line
			for (int c = 0; c <= m_State.m_iCurrentCursor; c++)
			{
				float rectStart = -1.0f;
				float rectEnd = -1.0f;
				Coordinates cursorSelectionStart = m_State.m_Cursors[c].GetSelectionStart();
				Coordinates cursorSelectionEnd = m_State.m_Cursors[c].GetSelectionEnd();
				assert(cursorSelectionStart <= cursorSelectionEnd);

				if (cursorSelectionStart <= lineEndCoord)
					rectStart = cursorSelectionStart > lineStartCoord ? TextDistanceToLineStart(cursorSelectionStart) : 0.0f;
				if (cursorSelectionEnd > lineStartCoord)
					rectEnd = TextDistanceToLineStart(cursorSelectionEnd < lineEndCoord ? cursorSelectionEnd : lineEndCoord);
				if (cursorSelectionEnd.m_iLine > lineNo || cursorSelectionEnd.m_iLine == lineNo && cursorSelectionEnd > lineEndCoord)
					rectEnd += m_CharAdvance.x;

				if (rectStart != -1 && rectEnd != -1 && rectStart < rectEnd)
					drawList->AddRectFilled(
						ImVec2{ lineStartScreenPos.x + m_fTextStart + rectStart, lineStartScreenPos.y },
						ImVec2{ lineStartScreenPos.x + m_fTextStart + rectEnd, lineStartScreenPos.y + m_CharAdvance.y },
						m_Palette[(int)PaletteIndex::Selection]);
			}

			// Draw line number (right aligned)
			if (m_bShowLineNumbers)
			{
				snprintf(lineNumberBuffer, 16, "%d  ", lineNo + 1);
				float lineNoWidth = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, lineNumberBuffer, nullptr, nullptr).x;
				drawList->AddText(ImVec2(lineStartScreenPos.x + m_fTextStart - lineNoWidth, lineStartScreenPos.y), m_Palette[(int)PaletteIndex::LineNumber], lineNumberBuffer);
			}

			std::vector<Coordinates> cursorCoordsInThisLine;
			for (int c = 0; c <= m_State.m_iCurrentCursor; c++)
			{
				if (m_State.m_Cursors[c].m_InteractiveEnd.m_iLine == lineNo)
					cursorCoordsInThisLine.push_back(m_State.m_Cursors[c].m_InteractiveEnd);
			}
			if (cursorCoordsInThisLine.size() > 0)
			{
				bool focused = ImGui::IsWindowFocused() || aParentIsFocused;

				// Render the cursors
				if (focused)
				{
					for (const auto& cursorCoords : cursorCoordsInThisLine)
					{
						float width = 1.0f;
						auto cindex = GetCharacterIndexR(cursorCoords);
						float cx = TextDistanceToLineStart(cursorCoords);

						if (m_bOverwrite && cindex < (int)line.size())
						{
							if (line[cindex].m_cChar == '\t')
							{
								auto x = (1.0f + std::floor((1.0f + cx) / (float(m_iTabSize) * spaceSize))) * (float(m_iTabSize) * spaceSize);
								width = x - cx;
							}
							else
								width = m_CharAdvance.x;
						}
						ImVec2 cstart(textScreenPos.x + cx, lineStartScreenPos.y);
						ImVec2 cend(textScreenPos.x + cx + width, lineStartScreenPos.y + m_CharAdvance.y);
						drawList->AddRectFilled(cstart, cend, m_Palette[(int)PaletteIndex::Cursor]);
						if (m_bCursorOnBracket)
						{
							ImVec2 topLeft = { cstart.x, lineStartScreenPos.y + fontHeight + 1.0f };
							ImVec2 bottomRight = { topLeft.x + m_CharAdvance.x, topLeft.y + 1.0f };
							drawList->AddRectFilled(topLeft, bottomRight, m_Palette[(int)PaletteIndex::Cursor]);
						}
					}
				}
			}

			// Render colorized text
			static std::string glyphBuffer;
			int charIndex = GetFirstVisibleCharacterIndex(lineNo);
			int column = m_iFirstVisibleColumn; // can be in the middle of tab character
			while (charIndex < m_Lines[lineNo].size() && column <= m_iLastVisibleColumn)
			{
				auto& glyph = line[charIndex];
				auto color = GetGlyphColor(glyph);
				ImVec2 targetGlyphPos = { lineStartScreenPos.x + m_fTextStart + TextDistanceToLineStart({lineNo, column}, false), lineStartScreenPos.y };

				if (glyph.m_cChar == '\t')
				{
					if (m_bShowWhitespaces)
					{
						ImVec2 p1, p2, p3, p4;

						const auto s = ImGui::GetFontSize();
						const auto x1 = targetGlyphPos.x + m_CharAdvance.x * 0.3f;
						const auto y = targetGlyphPos.y + fontHeight * 0.5f;

						if (m_bShortTabs)
						{
							const auto x2 = targetGlyphPos.x + m_CharAdvance.x;
							p1 = ImVec2(x1, y);
							p2 = ImVec2(x2, y);
							p3 = ImVec2(x2 - s * 0.16f, y - s * 0.16f);
							p4 = ImVec2(x2 - s * 0.16f, y + s * 0.16f);
						}
						else
						{
							const auto x2 = targetGlyphPos.x + TabSizeAtColumn(column) * m_CharAdvance.x - m_CharAdvance.x * 0.3f;
							p1 = ImVec2(x1, y);
							p2 = ImVec2(x2, y);
							p3 = ImVec2(x2 - s * 0.2f, y - s * 0.2f);
							p4 = ImVec2(x2 - s * 0.2f, y + s * 0.2f);
						}

						drawList->AddLine(p1, p2, m_Palette[(int)PaletteIndex::ControlCharacter]);
						drawList->AddLine(p2, p3, m_Palette[(int)PaletteIndex::ControlCharacter]);
						drawList->AddLine(p2, p4, m_Palette[(int)PaletteIndex::ControlCharacter]);
					}
				}
				else if (glyph.m_cChar == ' ')
				{
					if (m_bShowWhitespaces)
					{
						const auto s = ImGui::GetFontSize();
						const auto x = targetGlyphPos.x + spaceSize * 0.5f;
						const auto y = targetGlyphPos.y + s * 0.5f;
						drawList->AddCircleFilled(ImVec2(x, y), 1.5f, m_Palette[(int)PaletteIndex::ControlCharacter], 4);
					}
				}
				else
				{
					int seqLength = UTF8CharLength(glyph.m_cChar);
					if (m_bCursorOnBracket && seqLength == 1 && m_MatchingBracketCoords == Coordinates{ lineNo, column })
					{
						ImVec2 topLeft = { targetGlyphPos.x, targetGlyphPos.y + fontHeight + 1.0f };
						ImVec2 bottomRight = { topLeft.x + m_CharAdvance.x, topLeft.y + 1.0f };
						drawList->AddRectFilled(topLeft, bottomRight, m_Palette[(int)PaletteIndex::Cursor]);
					}
					glyphBuffer.clear();
					for (int i = 0; i < seqLength; i++)
						glyphBuffer.push_back(line[charIndex + i].m_cChar);
					drawList->AddText(targetGlyphPos, color, glyphBuffer.c_str());
				}

				MoveCharIndexAndColumn(lineNo, charIndex, column);
			}
		}
	}

	m_fCurrentSpaceHeight = (m_Lines.size() + Min(m_iVisibleLineCount - 1, (int)m_Lines.size())) * m_CharAdvance.y;
	m_fCurrentSpaceWidth = Max((maxColumnLimited + Min(m_iVisibleColumnCount - 1, maxColumnLimited)) * m_CharAdvance.x, m_fCurrentSpaceWidth);

	ImGui::SetCursorPos(ImVec2(0, 0));
	ImGui::Dummy(ImVec2(m_fCurrentSpaceWidth, m_fCurrentSpaceHeight));

	if (m_iEnsureCursorVisible > -1)
	{
		for (int i = 0; i < (m_bEnsureCursorVisibleStartToo ? 2 : 1); i++) // first pass for interactive end and second pass for interactive start
		{
			if (i) UpdateViewVariables(m_fScrollX, m_fScrollY); // second pass depends on changes made in first pass
			Coordinates targetCoords = GetActualCursorCoordinates(m_iEnsureCursorVisible, i); // cursor selection end or start
			if (targetCoords.m_iLine <= m_iFirstVisibleLine)
			{
				float targetScroll = std::max(0.0f, (targetCoords.m_iLine - 0.5f) * m_CharAdvance.y);
				if (targetScroll < m_fScrollY)
					ImGui::SetScrollY(targetScroll);
			}
			if (targetCoords.m_iLine >= m_iLastVisibleLine)
			{
				float targetScroll = std::max(0.0f, (targetCoords.m_iLine + 1.5f) * m_CharAdvance.y - m_fContentHeight);
				if (targetScroll > m_fScrollY)
					ImGui::SetScrollY(targetScroll);
			}
			if (targetCoords.m_iColumn <= m_iFirstVisibleColumn)
			{
				float targetScroll = std::max(0.0f, m_fTextStart + (targetCoords.m_iColumn - 0.5f) * m_CharAdvance.x);
				if (targetScroll < m_fScrollX)
					ImGui::SetScrollX(m_fScrollX = targetScroll);
			}
			if (targetCoords.m_iColumn >= m_iLastVisibleColumn)
			{
				float targetScroll = std::max(0.0f, m_fTextStart + (targetCoords.m_iColumn + 0.5f) * m_CharAdvance.x - m_fContentWidth);
				if (targetScroll > m_fScrollX)
					ImGui::SetScrollX(m_fScrollX = targetScroll);
			}
		}
		m_iEnsureCursorVisible = -1;
	}
	if (m_bScrollToTop)
	{
		ImGui::SetScrollY(0.0f);
		m_bScrollToTop = false;
	}
	if (m_iSetViewAtLine > -1)
	{
		float targetScroll;
		switch (m_SetViewAtLineMode)
		{
		default:
		case SetViewAtLineMode::FirstVisibleLine:
			targetScroll = std::max(0.0f, (float)m_iSetViewAtLine * m_CharAdvance.y);
			break;
		case SetViewAtLineMode::LastVisibleLine:
			targetScroll = std::max(0.0f, (float)(m_iSetViewAtLine - (m_iLastVisibleLine - m_iFirstVisibleLine)) * m_CharAdvance.y);
			break;
		case SetViewAtLineMode::Centered:
			targetScroll = std::max(0.0f, ((float)m_iSetViewAtLine - (float)(m_iLastVisibleLine - m_iFirstVisibleLine) * 0.5f) * m_CharAdvance.y);
			break;
		}
		ImGui::SetScrollY(targetScroll);
		m_iSetViewAtLine = -1;
	}
}

void CTextEditor::OnCursorPositionChanged()
{
	if (m_State.m_iCurrentCursor == 0 && !m_State.m_Cursors[0].HasSelection()) // only one cursor without selection
		m_bCursorOnBracket = FindMatchingBracket(m_State.m_Cursors[0].m_InteractiveEnd.m_iLine,
			GetCharacterIndexR(m_State.m_Cursors[0].m_InteractiveEnd), m_MatchingBracketCoords);
	else
		m_bCursorOnBracket = false;

	if (!m_bDraggingSelection)
	{
		m_State.SortCursorsFromTopToBottom();
		MergeCursorsIfPossible();
	}
}

void CTextEditor::OnLineChanged(bool aBeforeChange, int aLine, int aColumn, int aCharCount, bool aDeleted) // adjusts cursor position when other cursor writes/deletes in the same line
{
	static std::unordered_map<int, int> cursorCharIndices;
	if (aBeforeChange)
	{
		cursorCharIndices.clear();
		for (int c = 0; c <= m_State.m_iCurrentCursor; c++)
		{
			if (m_State.m_Cursors[c].m_InteractiveEnd.m_iLine == aLine && // cursor is at the line
				m_State.m_Cursors[c].m_InteractiveEnd.m_iColumn > aColumn && // cursor is to the right of changing part
				m_State.m_Cursors[c].GetSelectionEnd() == m_State.m_Cursors[c].GetSelectionStart()) // cursor does not have a selection
			{
				cursorCharIndices[c] = GetCharacterIndexR({ aLine, m_State.m_Cursors[c].m_InteractiveEnd.m_iColumn });
				cursorCharIndices[c] += aDeleted ? -aCharCount : aCharCount;
			}
		}
	}
	else
	{
		for (auto& item : cursorCharIndices)
			SetCursorPosition({ aLine, GetCharacterColumn(aLine, item.second) }, item.first);
	}
}

void CTextEditor::MergeCursorsIfPossible()
{
	// requires the cursors to be sorted from top to bottom
	std::unordered_set<int> cursorsToDelete;
	if (AnyCursorHasSelection())
	{
		// merge cursors if they overlap
		for (int c = m_State.m_iCurrentCursor; c > 0; c--)// iterate backwards through pairs
		{
			int pc = c - 1; // pc for previous cursor

			bool pcContainsC = m_State.m_Cursors[pc].GetSelectionEnd() >= m_State.m_Cursors[c].GetSelectionEnd();
			bool pcContainsStartOfC = m_State.m_Cursors[pc].GetSelectionEnd() > m_State.m_Cursors[c].GetSelectionStart();

			if (pcContainsC)
			{
				cursorsToDelete.insert(c);
			}
			else if (pcContainsStartOfC)
			{
				Coordinates pcStart = m_State.m_Cursors[pc].GetSelectionStart();
				Coordinates cEnd = m_State.m_Cursors[c].GetSelectionEnd();
				m_State.m_Cursors[pc].m_InteractiveEnd = cEnd;
				m_State.m_Cursors[pc].m_InteractiveStart = pcStart;
				cursorsToDelete.insert(c);
			}
		}
	}
	else
	{
		// merge cursors if they are at the same position
		for (int c = m_State.m_iCurrentCursor; c > 0; c--)// iterate backwards through pairs
		{
			int pc = c - 1;
			if (m_State.m_Cursors[pc].m_InteractiveEnd == m_State.m_Cursors[c].m_InteractiveEnd)
				cursorsToDelete.insert(c);
		}
	}
	for (int c = m_State.m_iCurrentCursor; c > -1; c--)// iterate backwards through each of them
	{
		if (cursorsToDelete.find(c) != cursorsToDelete.end())
			m_State.m_Cursors.erase(m_State.m_Cursors.begin() + c);
	}
	m_State.m_iCurrentCursor -= static_cast<int>(cursorsToDelete.size());
}

void CTextEditor::AddUndo(UndoRecord& aValue)
{
	assert(!m_bReadOnly);
	m_UndoBuffer.resize((size_t)(m_iUndoIndex + 1));
	m_UndoBuffer.back() = aValue;
	++m_iUndoIndex;
}

// TODO
// - multiline comments vs single-line: latter is blocking start of a ML
void CTextEditor::Colorize(int aFromLine, int aLines)
{
	int toLine = aLines == -1 ? (int)m_Lines.size() : std::min((int)m_Lines.size(), aFromLine + aLines);
	m_iColorRangeMin = std::min(m_iColorRangeMin, aFromLine);
	m_iColorRangeMax = std::max(m_iColorRangeMax, toLine);
	m_iColorRangeMin = std::max(0, m_iColorRangeMin);
	m_iColorRangeMax = std::max(m_iColorRangeMin, m_iColorRangeMax);
	m_bCheckComments = true;
}

void CTextEditor::ColorizeRange(int aFromLine, int aToLine)
{
	if (m_Lines.empty() || aFromLine >= aToLine || m_pLanguageDefinition == nullptr)
		return;

	std::string buffer;
	boost::cmatch results;
	std::string id;

	int endLine = std::max(0, std::min((int)m_Lines.size(), aToLine));
	for (int i = aFromLine; i < endLine; ++i)
	{
		auto& line = m_Lines[i];

		if (line.empty())
			continue;

		buffer.resize(line.size());
		for (size_t j = 0; j < line.size(); ++j)
		{
			auto& col = line[j];
			buffer[j] = col.m_cChar;
			col.m_ColorIndex = PaletteIndex::Default;
		}

		const char* bufferBegin = &buffer.front();
		const char* bufferEnd = bufferBegin + buffer.size();

		auto last = bufferEnd;

		for (auto first = bufferBegin; first != last; )
		{
			const char* token_begin = nullptr;
			const char* token_end = nullptr;
			PaletteIndex token_color = PaletteIndex::Default;

			bool hasTokenizeResult = false;

			if (m_pLanguageDefinition->m_Tokenize != nullptr)
			{
				if (m_pLanguageDefinition->m_Tokenize(first, last, token_begin, token_end, token_color))
					hasTokenizeResult = true;
			}

			if (hasTokenizeResult == false)
			{
				// todo : remove
				//printf("using regex for %.*s\n", first + 10 < last ? 10 : int(last - first), first);

				for (const auto& p : m_RegexList)
				{
					bool regexSearchResult = false;
					try { regexSearchResult = boost::regex_search(first, last, results, p.first, boost::regex_constants::match_continuous); }
					catch (...) {}
					if (regexSearchResult)
					{
						hasTokenizeResult = true;

						auto& v = *results.begin();
						token_begin = v.first;
						token_end = v.second;
						token_color = p.second;
						break;
					}
				}
			}

			if (hasTokenizeResult == false)
			{
				first++;
			}
			else
			{
				const size_t token_length = token_end - token_begin;

				if (token_color == PaletteIndex::Identifier)
				{
					id.assign(token_begin, token_end);

					// todo : allmost all language definitions use lower case to specify keywords, so shouldn't this use ::tolower ?
					if (!m_pLanguageDefinition->m_bCaseSensitive)
						std::transform(id.begin(), id.end(), id.begin(), ::toupper);

					if (!line[first - bufferBegin].m_bPreprocessor)
					{
						if (m_pLanguageDefinition->m_Keywords.count(id) != 0)
							token_color = PaletteIndex::Keyword;
						else if (m_pLanguageDefinition->m_Identifiers.count(id) != 0)
							token_color = PaletteIndex::KnownIdentifier;
						else if (m_pLanguageDefinition->m_PreprocIdentifiers	.count(id) != 0)
							token_color = PaletteIndex::PreprocIdentifier;
					}
					else
					{
						if (m_pLanguageDefinition->m_PreprocIdentifiers.count(id) != 0)
							token_color = PaletteIndex::PreprocIdentifier;
					}
				}

				for (size_t j = 0; j < token_length; ++j)
					line[(token_begin - bufferBegin) + j].m_ColorIndex = token_color;

				first = token_end;
			}
		}
	}
}

template<class InputIt1, class InputIt2, class BinaryPredicate>
bool ColorizerEquals(InputIt1 first1, InputIt1 last1,
	InputIt2 first2, InputIt2 last2, BinaryPredicate p)
{
	for (; first1 != last1 && first2 != last2; ++first1, ++first2)
	{
		if (!p(*first1, *first2))
			return false;
	}
	return first1 == last1 && first2 == last2;
}
void CTextEditor::ColorizeInternal()
{
	if (m_Lines.empty() || m_pLanguageDefinition == nullptr)
		return;

	if (m_bCheckComments)
	{
		auto endLine = m_Lines.size();
		auto endIndex = 0;
		auto commentStartLine = endLine;
		auto commentStartIndex = endIndex;
		auto withinString = false;
		auto withinSingleLineComment = false;
		auto withinPreproc = false;
		auto firstChar = true;			// there is no other non-whitespace characters in the line before
		auto concatenate = false;		// '\' on the very end of the line
		auto currentLine = 0;
		auto currentIndex = 0;
		while (currentLine < endLine || currentIndex < endIndex)
		{
			auto& line = m_Lines[currentLine];

			if (currentIndex == 0 && !concatenate)
			{
				withinSingleLineComment = false;
				withinPreproc = false;
				firstChar = true;
			}

			concatenate = false;

			if (!line.empty())
			{
				auto& g = line[currentIndex];
				auto c = g.m_cChar;

				if (c != m_pLanguageDefinition->m_cPreprocChar && !isspace(c))
					firstChar = false;

				if (currentIndex == (int)line.size() - 1 && line[line.size() - 1].m_cChar == '\\')
					concatenate = true;

				bool inComment = (commentStartLine < currentLine || (commentStartLine == currentLine && commentStartIndex <= currentIndex));

				if (withinString)
				{
					line[currentIndex].m_bMultiLineComment = inComment;

					if (c == '\"')
					{
						if (currentIndex + 1 < (int)line.size() && line[currentIndex + 1].m_cChar == '\"')
						{
							currentIndex += 1;
							if (currentIndex < (int)line.size())
								line[currentIndex].m_bMultiLineComment = inComment;
						}
						else
							withinString = false;
					}
					else if (c == '\\')
					{
						currentIndex += 1;
						if (currentIndex < (int)line.size())
							line[currentIndex].m_bMultiLineComment = inComment;
					}
				}
				else
				{
					if (firstChar && c == m_pLanguageDefinition->m_cPreprocChar)
						withinPreproc = true;

					if (c == '\"')
					{
						withinString = true;
						line[currentIndex].m_bMultiLineComment = inComment;
					}
					else
					{
						auto pred = [](const char& a, const Glyph& b) { return a == b.m_cChar; };
						auto from = line.begin() + currentIndex;
						auto& startStr = m_pLanguageDefinition->m_CommentStart;
						auto& singleStartStr = m_pLanguageDefinition->m_SingleLineComment;

						if (!withinSingleLineComment && currentIndex + startStr.size() <= line.size() &&
							ColorizerEquals(startStr.begin(), startStr.end(), from, from + startStr.size(), pred))
						{
							commentStartLine = currentLine;
							commentStartIndex = currentIndex;
						}
						else if (singleStartStr.size() > 0 &&
							currentIndex + singleStartStr.size() <= line.size() &&
							ColorizerEquals(singleStartStr.begin(), singleStartStr.end(), from, from + singleStartStr.size(), pred))
						{
							withinSingleLineComment = true;
						}

						inComment = (commentStartLine < currentLine || (commentStartLine == currentLine && commentStartIndex <= currentIndex));

						line[currentIndex].m_bMultiLineComment = inComment;
						line[currentIndex].m_bComment = withinSingleLineComment;

						auto& endStr = m_pLanguageDefinition->m_CommentEnd;
						if (currentIndex + 1 >= (int)endStr.size() &&
							ColorizerEquals(endStr.begin(), endStr.end(), from + 1 - endStr.size(), from + 1, pred))
						{
							commentStartIndex = endIndex;
							commentStartLine = endLine;
						}
					}
				}
				if (currentIndex < (int)line.size())
					line[currentIndex].m_bPreprocessor = withinPreproc;
				currentIndex += UTF8CharLength(c);
				if (currentIndex >= (int)line.size())
				{
					currentIndex = 0;
					++currentLine;
				}
			}
			else
			{
				currentIndex = 0;
				++currentLine;
			}
		}
		m_bCheckComments = false;
	}

	if (m_iColorRangeMin < m_iColorRangeMax)
	{
		const int increment = (m_pLanguageDefinition->m_Tokenize == nullptr) ? 10 : 10000;
		const int to = std::min(m_iColorRangeMin + increment, m_iColorRangeMax);
		ColorizeRange(m_iColorRangeMin, to);
		m_iColorRangeMin = to;

		if (m_iColorRangeMax == m_iColorRangeMin)
		{
			m_iColorRangeMin = std::numeric_limits<int>::max();
			m_iColorRangeMax = 0;
		}
		return;
	}
}

const CTextEditor::Palette& CTextEditor::GetDarkPalette()
{
	const static Palette p = { {
			0xdcdfe4ff,	// Default
			0xe06c75ff,	// Keyword
			0xe5c07bff,	// Number
			0x98c379ff,	// String
			0xe0a070ff, // Char literal
			0x6a7384ff, // Punctuation
			0x808040ff,	// Preprocessor
			0xdcdfe4ff, // Identifier
			0x61afefff, // Known identifier
			0xc678ddff, // Preproc identifier
			0x3696a2ff, // Comment (single line)
			0x3696a2ff, // Comment (multi line)
			0x121212ff, // Background
			0xe0e0e0ff, // Cursor
			0x2060a080, // Selection
			0xffffff15, // ControlCharacter
			0x7a8394ff, // Line number
		} };
	return p;
}

const CTextEditor::Palette& CTextEditor::GetLightPalette()
{
	const static Palette p = { {
			0x404040ff,	// None
			0x060cffff,	// Keyword	
			0x008000ff,	// Number
			0xa02020ff,	// String
			0x704030ff, // Char literal
			0x000000ff, // Punctuation
			0x606040ff,	// Preprocessor
			0x404040ff, // Identifier
			0x106060ff, // Known identifier
			0xa040c0ff, // Preproc identifier
			0x205020ff, // Comment (single line)
			0x205040ff, // Comment (multi line)
			0xffffffff, // Background
			0x000000ff, // Cursor
			0x00006040, // Selection
			0x90909090, // ControlCharacter
			0x005050ff, // Line number
		} };
	return p;
}

const std::unordered_map<char, char> CTextEditor::OPEN_TO_CLOSE_CHAR = {
	{'{', '}'},
	{'(' , ')'},
	{'[' , ']'}
};
const std::unordered_map<char, char> CTextEditor::CLOSE_TO_OPEN_CHAR = {
	{'}', '{'},
	{')' , '('},
	{']' , '['}
};

CTextEditor::PaletteId CTextEditor::defaultPalette = CTextEditor::PaletteId::Dark;