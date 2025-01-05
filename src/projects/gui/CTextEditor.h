#pragma once

#include <cmath>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include "boost/regex.hpp"
#include "imgui.h"

class CTextEditor
{
public:
	CTextEditor();
	~CTextEditor();

	enum class PaletteId
	{
		Dark, Light
	};

	enum class LanguageDefinitionId
	{
		None, 
		Cpp, 
		C, 
		Cs, 
		Python, 
		Lua, 
		Json, 
		Sql, 
		AngelScript,
		Glsl, 
		Hlsl
	};

	enum class SetViewAtLineMode
	{
		FirstVisibleLine, Centered, LastVisibleLine
	};

	inline void SetReadOnlyEnabled(bool bValue) { m_bReadOnly = bValue; }
	inline bool IsReadOnlyEnabled() const { return m_bReadOnly; }
	inline void SetAutoIndentEnabled(bool bValue) { m_bAutoIndent = bValue; }
	inline bool IsAutoIndentEnabled() const { return m_bAutoIndent; }
	inline void SetShowWhitespacesEnabled(bool bValue) { m_bShowWhitespaces = bValue; }
	inline bool IsShowWhitespacesEnabled() const { return m_bShowWhitespaces; }
	inline void SetShowLineNumbersEnabled(bool bValue) { m_bShowLineNumbers = bValue; }
	inline bool IsShowLineNumbersEnabled() const { return m_bShowLineNumbers; }
	inline void SetShortTabsEnabled(bool bValue) { m_bShortTabs = bValue; }
	inline bool IsShortTabsEnabled() const { return m_bShortTabs; }
	inline size_t GetLineCount() const { return m_Lines.size(); }
	inline bool IsOverwriteEnabled() const { return m_bOverwrite; }
	void SetPalette(PaletteId Value);
	PaletteId GetPalette() const { return m_PaletteId; }
	void SetLanguageDefinition(LanguageDefinitionId Value);
	LanguageDefinitionId GetLanguageDefinition() const { return m_LanguageDefinitionId; };
	const char* GetLanguageDefinitionName() const;
	void SetTabSize(int iValue);
	inline int GetTabSize() const { return m_iTabSize; }
	void SetLineSpacing(float fValue);
	inline float GetLineSpacing() const { return m_fLineSpacing;  }

	inline static void SetDefaultPalette(PaletteId Value) { defaultPalette = Value; }
	inline static PaletteId GetDefaultPalette() { return defaultPalette; }

	void SelectAll();
	void SelectLine(int iLine);
	void SelectRegion(int iStartLine, int iStartChar, int iEndLine, int iEndChar);
	void SelectNextOccurrenceOf(const char* Text, int iTextSize, bool bCaseSensitive = true);
	void SelectAllOccurrencesOf(const char* Text, int iTextSize, bool bCaseSensitive = true);
	bool AnyCursorHasSelection() const;
	bool AllCursorsHaveSelection() const;
	void ClearExtraCursors();
	void ClearSelections();
	void SetCursorPosition(int iLine, int iCharIndex);
	inline void GetCursorPosition(int& iOutLine, int& iOutColumn) const
	{
		auto coords = GetActualCursorCoordinates();
		iOutLine = coords.m_iLine;
		iOutColumn = coords.m_iColumn;
	}
	int GetFirstVisibleLine();
	int GetLastVisibleLine();
	void SetViewAtLine(int iLine, SetViewAtLineMode Mode);

	void Copy();
	void Cut();
	void Paste();
	void Undo(int aSteps = 1);
	void Redo(int aSteps = 1);
	inline bool CanUndo() const { return !m_bReadOnly && m_iUndoIndex > 0; };
	inline bool CanRedo() const { return !m_bReadOnly && m_iUndoIndex < (int)m_UndoBuffer.size(); };
	inline int GetUndoIndex() const { return m_iUndoIndex; };

	void SetText(const std::string& Text);
	std::string GetText() const;

	void SetTextLines(const std::vector<std::string>& Lines);
	std::vector<std::string> GetTextLines() const;

	bool Render(const char* aTitle, bool aParentIsFocused = false, const ImVec2& aSize = ImVec2(), bool aBorder = false);

private:
	// ------------- Generic utils ------------- //

	static inline ImVec4 U32ColorToVec4(ImU32 in)
	{
		float s = 1.0f / 255.0f;
		return ImVec4(
			((in >> IM_COL32_A_SHIFT) & 0xFF) * s,
			((in >> IM_COL32_B_SHIFT) & 0xFF) * s,
			((in >> IM_COL32_G_SHIFT) & 0xFF) * s,
			((in >> IM_COL32_R_SHIFT) & 0xFF) * s);
	}

	static inline bool IsUTFSequence(char c)
	{
		return (c & 0xC0) == 0x80;
	}

	static inline float Distance(const ImVec2& a, const ImVec2& b)
	{
		float x = a.x - b.x;
		float y = a.y - b.y;
		return sqrt(x * x + y * y);
	}
	template<typename T>
	static inline T Max(T a, T b) { return a > b ? a : b; }
	template<typename T>
	static inline T Min(T a, T b) { return a < b ? a : b; }

	// ------------- Internal ------------- //

	enum class PaletteIndex
	{
		Default,
		Keyword,
		Number,
		String,
		CharLiteral,
		Punctuation,
		Preprocessor,
		Identifier,
		KnownIdentifier,
		PreprocIdentifier,
		Comment,
		MultiLineComment,
		Background,
		Cursor,
		Selection,
		ControlCharacter,
		LineNumber,
		Max
	};

	// Represents a character coordinate from the user's point of view,
	// i. e. consider an uniform grid (assuming fixed-width font) on the
	// screen as it is rendered, and each cell has its own coordinate, starting from 0.
	// Tabs are counted as [1..mTabSize] count empty spaces, depending on
	// how many space is necessary to reach the next tab stop.
	// For example, coordinate (1, 5) represents the character 'B' in a line "\tABC", when mTabSize = 4,
	// because it is rendered as "    ABC" on the screen.
	struct Coordinates
	{
		int m_iLine, m_iColumn;
		Coordinates() : m_iLine(0), m_iColumn(0) {}
		Coordinates(int iLine, int iColumn) : m_iLine(iLine), m_iColumn(iColumn)
		{
			assert(iLine >= 0);
			assert(iColumn >= 0);
		}
		static Coordinates Invalid() { static Coordinates invalid(-1, -1); return invalid; }

		bool operator ==(const Coordinates& o) const
		{
			return
				m_iLine == o.m_iLine &&
				m_iColumn == o.m_iColumn;
		}

		bool operator !=(const Coordinates& o) const
		{
			return
				m_iLine != o.m_iLine ||
				m_iColumn != o.m_iColumn;
		}

		bool operator <(const Coordinates& o) const
		{
			if (m_iLine != o.m_iLine)
				return m_iLine < o.m_iLine;
			return m_iColumn < o.m_iColumn;
		}

		bool operator >(const Coordinates& o) const
		{
			if (m_iLine != o.m_iLine)
				return m_iLine > o.m_iLine;
			return m_iColumn > o.m_iColumn;
		}

		bool operator <=(const Coordinates& o) const
		{
			if (m_iLine != o.m_iLine)
				return m_iLine < o.m_iLine;
			return m_iColumn <= o.m_iColumn;
		}

		bool operator >=(const Coordinates& o) const
		{
			if (m_iLine != o.m_iLine)
				return m_iLine > o.m_iLine;
			return m_iColumn >= o.m_iColumn;
		}

		Coordinates operator -(const Coordinates& o)
		{
			return Coordinates(m_iLine - o.m_iLine, m_iColumn - o.m_iColumn);
		}

		Coordinates operator +(const Coordinates& o)
		{
			return Coordinates(m_iLine + o.m_iLine, m_iColumn + o.m_iColumn);
		}
	};

	struct Cursor
	{
		Coordinates m_InteractiveStart = { 0, 0 };
		Coordinates m_InteractiveEnd = { 0, 0 };
		inline Coordinates GetSelectionStart() const { return m_InteractiveStart < m_InteractiveEnd ? m_InteractiveStart : m_InteractiveEnd; }
		inline Coordinates GetSelectionEnd() const { return m_InteractiveStart > m_InteractiveEnd ? m_InteractiveStart : m_InteractiveEnd; }
		inline bool HasSelection() const { return m_InteractiveStart != m_InteractiveEnd; }
	};

	struct EditorState // state to be restored with undo/redo
	{
		int m_iCurrentCursor = 0;
		int m_iLastAddedCursor = 0;
		std::vector<Cursor> m_Cursors = { {{0,0}} };
		void AddCursor();
		int GetLastAddedCursorIndex();
		void SortCursorsFromTopToBottom();
	};

	struct Identifier
	{
		Coordinates m_Location;
		std::string m_Declaration;
	};

	typedef std::unordered_map<std::string, Identifier> Identifiers;
	typedef std::array<ImU32, (unsigned)PaletteIndex::Max> Palette;

	struct Glyph
	{
		char m_cChar;
		PaletteIndex m_ColorIndex = PaletteIndex::Default;
		bool m_bComment : 1;
		bool m_bMultiLineComment : 1;
		bool m_bPreprocessor : 1;

		Glyph(char cChar, PaletteIndex ColorIndex) : m_cChar(cChar), m_ColorIndex(ColorIndex),
			m_bComment(false), m_bMultiLineComment(false), m_bPreprocessor(false) {}
	};

	typedef std::vector<Glyph> Line;

	struct LanguageDefinition
	{
		typedef std::pair<std::string, PaletteIndex> TokenRegexString;
		typedef bool(*TokenizeCallback)(const char* in_begin, const char* in_end, const char*& out_begin, const char*& out_end, PaletteIndex& paletteIndex);

		std::string m_Name;
		std::unordered_set<std::string> m_Keywords;
		Identifiers m_Identifiers;
		Identifiers m_PreprocIdentifiers;
		std::string m_CommentStart, m_CommentEnd, m_SingleLineComment;
		char m_cPreprocChar = '#';
		TokenizeCallback m_Tokenize = nullptr;
		std::vector<TokenRegexString> m_TokenRegexStrings;
		bool m_bCaseSensitive = true;

		static const LanguageDefinition& Cpp();
		static const LanguageDefinition& Hlsl();
		static const LanguageDefinition& Glsl();
		static const LanguageDefinition& Python();
		static const LanguageDefinition& C();
		static const LanguageDefinition& Sql();
		static const LanguageDefinition& AngelScript();
		static const LanguageDefinition& Lua();
		static const LanguageDefinition& Cs();
		static const LanguageDefinition& Json();
	};

	enum class UndoOperationType { Add, Delete };
	struct UndoOperation
	{
		std::string m_Text;
		CTextEditor::Coordinates m_Start;
		CTextEditor::Coordinates m_End;
		UndoOperationType m_Type;
	};

	typedef std::vector<std::pair<boost::regex, PaletteIndex>> RegexList;

	class UndoRecord
	{
	public:
		UndoRecord() {}
		~UndoRecord() {}

		UndoRecord(
			const std::vector<UndoOperation>& Operations,
			CTextEditor::EditorState& Before,
			CTextEditor::EditorState& After);

		void Undo(CTextEditor* pTextEditor);
		void Redo(CTextEditor* pTextEditor);

		std::vector<UndoOperation> m_Operations;

		EditorState m_Before;
		EditorState m_After;
	};

	std::string GetText(const Coordinates& Start, const Coordinates& End) const;
	std::string GetClipboardText() const;
	std::string GetSelectedText(int iCursor = -1) const;

	void SetCursorPosition(const Coordinates& Position, int iCursor = -1, bool bClearSelection = true);

	int InsertTextAt(Coordinates& Where, const char* pValue);
	void InsertTextAtCursor(const char* pValue, int iCursor = -1);

	enum class MoveDirection { Right = 0, Left = 1, Up = 2, Down = 3 };
	bool Move(int& iLine, int& iCharIndex, bool bLeft = false, bool bLockLine = false) const;
	void MoveCharIndexAndColumn(int iLine, int& iCharIndex, int& iColumn) const;
	void MoveCoords(Coordinates& Coords, MoveDirection Direction, bool bWordMode = false, int iLineCount = 1) const;

	void MoveUp(int iAmount = 1, bool bSelect = false);
	void MoveDown(int iAmount = 1, bool bSelect = false);
	void MoveLeft(bool bSelect = false, bool bWordMode = false);
	void MoveRight(bool bSelect = false, bool bWordMode = false);
	void MoveTop(bool bSelect = false);
	void MoveBottom(bool bSelect = false);
	void MoveHome(bool bSelect = false);
	void MoveEnd(bool bSelect = false);
	void EnterCharacter(ImWchar wChar, bool bShift);
	void Backspace(bool bWordMode = false);
	void Delete(bool bWordMode = false, const EditorState* pEditorState = nullptr);

	void SetSelection(Coordinates Start, Coordinates End, int iCursor = -1);
	void SetSelection(int iStartLine, int iStartChar, int iEndLine, int iEndChar, int iCursor = -1);

	void SelectNextOccurrenceOf(const char* pText, int iTextSize, int iCursor = -1, bool bCaseSensitive = true);
	void AddCursorForNextOccurrence(bool bCaseSensitive = true);
	bool FindNextOccurrence(const char* pText, int iTextSize, const Coordinates& From, Coordinates& outStart, Coordinates& outEnd, bool bCaseSensitive = true);
	bool FindMatchingBracket(int iLine, int iCharIndex, Coordinates& out);
	void ChangeCurrentLinesIndentation(bool bIncrease);
	void MoveUpCurrentLines();
	void MoveDownCurrentLines();
	void ToggleLineComment();
	void RemoveCurrentLines();

	float TextDistanceToLineStart(const Coordinates& From, bool bSanitizeCoords = true) const;
	void EnsureCursorVisible(int iCursor = -1, bool bStartToo = false);

	Coordinates SanitizeCoordinates(const Coordinates& Value) const;
	Coordinates GetActualCursorCoordinates(int iCursor = -1, bool bStart = false) const;
	Coordinates ScreenPosToCoordinates(const ImVec2& Position, bool bInsertionMode = false, bool* bIsOverLineNumber = nullptr) const;
	Coordinates FindWordStart(const Coordinates& From) const;
	Coordinates FindWordEnd(const Coordinates& From) const;
	int GetCharacterIndexL(const Coordinates& Coordinates) const;
	int GetCharacterIndexR(const Coordinates& Coordinates) const;
	int GetCharacterColumn(int iLine, int iIndex) const;
	int GetFirstVisibleCharacterIndex(int iLine) const;
	int GetLineMaxColumn(int iLine, int iLimit = -1) const;

	Line& InsertLine(int iIndex);
	void RemoveLine(int iIndex, const std::unordered_set<int>* pHandledCursors = nullptr);
	void RemoveLines(int iStart, int iEnd);
	void DeleteRange(const Coordinates& Start, const Coordinates& End);
	void DeleteSelection(int iCursor = -1);

	void RemoveGlyphsFromLine(int iLine, int iStartChar, int iEndChar = -1);
	void AddGlyphsToLine(int iLine, int iTargetIndex, Line::iterator SourceStart, Line::iterator SourceEnd);
	void AddGlyphToLine(int iLine, int iTargetIndex, Glyph Glyph);
	ImU32 GetGlyphColor(const Glyph& aGlyph) const;

	void HandleKeyboardInputs(bool bParentIsFocused = false);
	void HandleMouseInputs();
	void UpdateViewVariables(float fScrollX, float fScrollY);
	void Render(bool bParentIsFocused = false);

	void OnCursorPositionChanged();
	void OnLineChanged(bool bBeforeChange, int iLine, int iColumn, int iCharCount, bool bDeleted);
	void MergeCursorsIfPossible();

	void AddUndo(UndoRecord& Value);

	void Colorize(int iFromLine = 0, int iCount = -1);
	void ColorizeRange(int iFromLine = 0, int iToLine = 0);
	void ColorizeInternal();

	std::vector<Line> m_Lines;
	EditorState m_State;
	std::vector<UndoRecord> m_UndoBuffer;
	int m_iUndoIndex;

	int m_iTabSize;
	float m_fLineSpacing;
	bool m_bOverwrite;
	bool m_bReadOnly;
	bool m_bAutoIndent;
	bool m_bShowWhitespaces;
	bool m_bShowLineNumbers;
	bool m_bShortTabs;

	int m_iSetViewAtLine;
	SetViewAtLineMode m_SetViewAtLineMode;
	int m_iEnsureCursorVisible;
	bool m_bEnsureCursorVisibleStartToo;
	bool m_bScrollToTop;

	float m_fTextStart;
	int m_iLeftMargin;
	ImVec2 m_CharAdvance;
	float m_fCurrentSpaceHeight;
	float m_fCurrentSpaceWidth;
	float m_fLastClickTime;
	ImVec2 m_LastClickPos;
	int m_iFirstVisibleLine;
	int m_iLastVisibleLine;
	int m_iVisibleLineCount;
	int m_iFirstVisibleColumn;
	int m_iLastVisibleColumn;
	int m_iVisibleColumnCount;
	float m_fContentWidth;
	float m_fContentHeight;
	float m_fScrollX;
	float m_fScrollY;
	bool m_bPanning;
	bool m_bDraggingSelection;
	ImVec2 m_LastMousePos;
	bool m_bCursorPositionChanged;
	bool m_bCursorOnBracket;
	Coordinates m_MatchingBracketCoords;

	int m_iColorRangeMin;
	int m_iColorRangeMax;
	bool m_bCheckComments;
	PaletteId m_PaletteId;
	Palette m_Palette;
	LanguageDefinitionId m_LanguageDefinitionId;
	const LanguageDefinition* m_pLanguageDefinition;
	RegexList m_RegexList;

	inline bool IsHorizontalScrollbarVisible() const { return m_fCurrentSpaceWidth > m_fContentWidth; }
	inline bool IsVerticalScrollbarVisible() const { return m_fCurrentSpaceHeight > m_fContentHeight; }
	inline int TabSizeAtColumn(int iColumn) const { return m_iTabSize - (iColumn % m_iTabSize); }

	static const Palette& GetDarkPalette();
	static const Palette& GetLightPalette();
	static const std::unordered_map<char, char> OPEN_TO_CLOSE_CHAR;
	static const std::unordered_map<char, char> CLOSE_TO_OPEN_CHAR;
	static PaletteId defaultPalette;
};
