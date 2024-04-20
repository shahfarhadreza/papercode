#include <algorithm>
#include <chrono>
#include <string>
#include <regex>
#include <cmath>

#include <imgui_internal.h>

#include "TextEditor.h"

int UTF8CharLength(TextEditor::Char c);

TextEditor::Coordinates TextEditor::FindWordStartAutoComplete(const Coordinates & aFrom) const
{
	Coordinates at = aFrom;
	if (at.mLine >= (int)mLines.size())
		return at;

	auto& line = mLines[at.mLine];
	auto cindex = GetCharacterIndex(at);

	bool endLine = false;

	if ( cindex > 0 && cindex == (int)line.size()) {
		// probably cursor is at the end of the line
		// lets goto one index prev
		cindex--;
		endLine = true;
	}

	if (cindex >= (int)line.size() || cindex < 0) {
		return at;
	}

	auto rightChar = line[cindex].mChar;

	if (!(isalpha(rightChar) || rightChar == '_')) {
		if (endLine) {
			return at;
		}
		if (cindex > 0) {
			cindex -= UTF8CharLength(rightChar);
		}
	}

	while (cindex > 0) {
		auto c = line[cindex].mChar;
		auto d = UTF8CharLength(c);

		auto prevChar = line[cindex-1].mChar;

		if (!(isalpha(prevChar) || prevChar == '_')) {
			break;
		}
		cindex -= d;
	}

	return Coordinates(at.mLine, GetCharacterColumn(at.mLine, cindex));
}

TextEditor::Coordinates TextEditor::FindWordEndAutoComplete(const Coordinates & aFrom) const
{
	Coordinates at = aFrom;
	if (at.mLine >= (int)mLines.size())
		return at;

	auto& line = mLines[at.mLine];
	auto cindex = GetCharacterIndex(at);

	if (cindex >= (int)line.size())
		return at;

	while (cindex < (int)line.size())
	{
		auto c = line[cindex].mChar;
		auto d = UTF8CharLength(c);

		if (!(isalpha(c) || c == '_')) {
			break;
		}
		cindex += d;
	}
	return Coordinates(aFrom.mLine, GetCharacterColumn(aFrom.mLine, cindex));
}

std::string TextEditor::GetWordForAutoComplete(const Coordinates& aCoords) {
	mAutoCompleteWordStart = FindWordStartAutoComplete(aCoords);
	mAutoCompleteWordEnd = FindWordEndAutoComplete(aCoords);

	printf("start %d, end %d\n", mAutoCompleteWordStart.mColumn, mAutoCompleteWordEnd.mColumn);

	std::string r;

	auto istart = GetCharacterIndex(mAutoCompleteWordStart);
	auto iend = GetCharacterIndex(mAutoCompleteWordEnd);

	for (auto it = istart; it < iend; ++it) {
		auto c = mLines[aCoords.mLine][it].mChar;
		r.push_back(c);
	}

	return r;
}

void TextEditor::RenderAutoComplete(const ImVec2& aPosition) {

    ImGuiWindowFlags flags = 
        ImGuiWindowFlags_NoTitleBar          | 
        ImGuiWindowFlags_NoResize            |
        ImGuiWindowFlags_NoMove              |
        ImGuiWindowFlags_HorizontalScrollbar |
        ImGuiWindowFlags_NoSavedSettings  ;

	ImGui::SetNextWindowPos(aPosition);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(7, 7));

	bool visible = ImGui::BeginChild("##typeAheadSearchPopup", ImVec2(130, 160), ImGuiChildFlags_Border, flags);
    ImGui::PushAllowKeyboardFocus(false);

    if (visible) {
    	if (ImGui::IsWindowHovered()) {
    		mMouseOverAutoComplete = true;
    	} else {
    		mMouseOverAutoComplete = false;
    	}
    	int idx = 0;
    	for (auto& v : mAutoCompleteList) {


    		bool isIndexActive = idx == mAutoCompleteBestMatchIndex;

    		if( isIndexActive ) {
	            // Draw the currently 'active' item differently
	            // ( used appropriate colors for your own style )
	            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4( 1, 0, 0, 1 ) );
	        }

	        ImGui::PushID(idx);
	    	if (ImGui::Selectable(v.c_str(), isIndexActive)) {

	    		mAutoCompleteBestMatchIndex = idx;

	    		AcceptAutoComplete();
	    		ImGui::PopID();

	    		if (isIndexActive) {
	    			ImGui::PopStyleColor(1);
	    		}
	    		break;
	    	}
	    	ImGui::PopID();
	    	idx++;

	    	if (isIndexActive) {
	    		if( mAutoCompleteSelectionChanged ) {
	                // Make sure we bring the currently 'active' item into view.
	                ImGui::SetScrollHereY();
	                mAutoCompleteSelectionChanged = false;
	            }
	            ImGui::PopStyleColor(1);
	    	}
	    }
    }

    ImGui::PopAllowKeyboardFocus();
    ImGui::EndChild();

    ImGui::PopStyleVar();
}

void TextEditor::ResetAutoComplete() {
	mShowAutoComplete = false;
	mAutoCompleteList.clear();
	mAutoCompleteWord = "";
	mAutoCompleteBestMatchIndex = -1;
	mAutoCompleteSelectionChanged = false;
	mFocusBack = true;
}

void TextEditor::AcceptAutoComplete() {
	if (mAutoCompleteBestMatchIndex >= 0 && mAutoCompleteList.size() > 0) {
		const std::string& bestWord = mAutoCompleteList[mAutoCompleteBestMatchIndex];
		ReplaceRange(bestWord, mAutoCompleteWordStart, mAutoCompleteWordEnd);
	}
	ResetAutoComplete();
}

void TextEditor::StartAutoComplete(const Coordinates& pos) {

	// TODO: Don't show up if we are inside a string

	mAutoCompleteWord = GetWordForAutoComplete(pos);
	mAutoCompleteSelectionChanged = true;

	//printf("LOG: Auto %s\n", mAutoCompleteWord.c_str());

	if (mAutoCompleteWord.empty() == false && 
		(mAutoCompleteWord.find_first_not_of(" \t\n\v\f\r") != std::string::npos)) 
	{
		if (!mShowAutoComplete) {
			mShowAutoComplete = true;
		}
		mAutoCompleteList.clear();

		// First, narrow down the list to the words contains our

		// Keywords
		for (auto& key : mLanguageDefinition.mKeywords) {
			size_t found = key.find(mAutoCompleteWord);
	  		if (found != std::string::npos) {
	  			mAutoCompleteList.push_back(key);
	  		}
		}

		// Now we have short list
		if (!mAutoCompleteList.empty()) {
			int idx = 0;
			mAutoCompleteBestMatchIndex = 0;
			for (auto& v : mAutoCompleteList) {
				//printf("%s == %s: %d\n", v.c_str(), mAutoCompleteWord.c_str(), match);
		  		if (v.starts_with(mAutoCompleteWord)) {
					mAutoCompleteBestMatchIndex = idx;
					break;
				}
				idx++;
			}
		} else {
			ResetAutoComplete();
		}
	} else {
		if (mShowAutoComplete) {
			ResetAutoComplete();
		}
	}
}

