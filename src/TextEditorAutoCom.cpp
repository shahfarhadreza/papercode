#include "Stdafx.h"
#include "PaperCode.h"

#include <algorithm>
#include <chrono>
#include <regex>
#include <cmath>

#include <font-awesome/IconsFontAwesome5.h>

#include <imgui_internal.h>

#include "TextEditor.h"

import smartsense;

int UTF8CharLength(TextEditor::Char c);

TextEditor::Coordinates TextEditor::FindWordStartAutoComplete(const Coordinates & aFrom)
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
			if (prevChar == '.') {
				mAutoCompleteAccess = MemberAccess::Dot;
			}
			break;
		}
		cindex -= d;
	}

	return Coordinates(at.mLine, GetCharacterColumn(at.mLine, cindex));
}

TextEditor::Coordinates TextEditor::FindWordEndAutoComplete(const Coordinates & aFrom)
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

	//printf("start %d, end %d\n", mAutoCompleteWordStart.mColumn, mAutoCompleteWordEnd.mColumn);

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

	bool visible = ImGui::BeginChild("##typeAheadSearchPopup", ImVec2(170, 180), ImGuiChildFlags_Border, flags);
    ImGui::PushAllowKeyboardFocus(false);

    if (visible) {
    	if (ImGui::IsWindowHovered()) {
    		mMouseOverAutoComplete = true;
    	} else {
    		mMouseOverAutoComplete = false;
    	}
    	//int idx = 0;
    	bool close = false;
    	ImGuiListClipper clipper;
        
    	clipper.Begin(mAutoCompleteList.size());

    	// Make sure we bring the currently 'active' item into view.
    	if (mAutoCompleteSelectionChanged) {
    		if (mAutoCompleteBestMatchIndex >= 0 && mAutoCompleteBestMatchIndex < mAutoCompleteList.size()) {
    			clipper.IncludeItemByIndex(mAutoCompleteBestMatchIndex);
    		}
    	}

    	//printf("total %d, start %d, end %d\n", mAutoCompleteList.size(), clipper.DisplayStart, clipper.DisplayEnd);
        while (clipper.Step()) {
	        for (int idx = clipper.DisplayStart; idx < clipper.DisplayEnd; ++idx) {

	        	if (idx >= mAutoCompleteList.size())
	        		break;

	        	const auto& v = mAutoCompleteList[idx];
	    	/*
	    	for (auto& v : mAutoCompleteList) {
	    	*/

	    		bool isIndexActive = idx == mAutoCompleteBestMatchIndex;

	    		if( isIndexActive ) {
		            // Draw the currently 'active' item differently
		            // ( used appropriate colors for your own style )
		            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4( 1, 0, 0, 1 ) );
		        }

		        ImGui::PushID(idx);

		    	if (ImGui::Selectable((ICON_FA_CUBE" " + v.mName).c_str(), isIndexActive)) {
		    		mAutoCompleteBestMatchIndex = idx;
		    		close = true;
		    	}
		    	if (ImGui::IsItemHovered(ImGuiHoveredFlags_None)) {
		    		if (!v.mSignature.empty()) {
		    			ImGui::BeginTooltip();

		    			ImGui::PushTextWrapPos(500);

					    ImGui::TextWrapped("%s", v.mSignature.c_str());

					    ImGui::PopTextWrapPos();

						ImGui::EndTooltip();
		    			//ImGui::SetTooltip("%s", v.mSymbol->mCalltip.c_str());
		    		}
		    	}
		    	ImGui::PopID(); 
		    	//idx++;

		    	if (isIndexActive) {
		    		if( mAutoCompleteSelectionChanged ) {
		                // Make sure we bring the currently 'active' item into view.
		                ImGui::SetScrollHereY();
		                mAutoCompleteSelectionChanged = false;
		            }
		            ImGui::PopStyleColor(1);
		    	}
		    	if (close) {
		    		break;
		    	}
		    }
		}

		clipper.End();

	    if (close) {
	    	AcceptAutoComplete();
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
	mAutoCompleteAccess = MemberAccess::None;
	mFocusBack = true;
}

void TextEditor::AcceptAutoComplete() {
	if (mAutoCompleteBestMatchIndex >= 0 && mAutoCompleteList.size() > 0) {
		const std::string& bestWord = mAutoCompleteList[mAutoCompleteBestMatchIndex].mName;
		// Maybe we don't to replace just insert would be good
		if (mAutoCompleteWordStart == mAutoCompleteWordEnd) {
			InsertText(bestWord);
		} else {
			ReplaceRange(bestWord, mAutoCompleteWordStart, mAutoCompleteWordEnd);
		}
	}
	ResetAutoComplete();
}

void TextEditor::StartAutoComplete(const Coordinates& pos, bool force) {

	// TODO: Don't show up if we are inside a string

	mAutoCompleteWord = GetWordForAutoComplete(pos);
	mAutoCompleteSelectionChanged = true;
	mAutoCompleteBestMatchIndex = -1;

	//printf("LOG: find :%s:\n", mAutoCompleteWord.c_str());
	if (force == false && mAutoCompleteWord.empty()) {
		return;
	}

	// TODO: Instead of waiting for the whole list to popup
	// maybe this could be dynamic?? like show while loading??
	
	if (!mShowAutoComplete) {	
		mAutoCompleteList.clear();

		// First, get all the possible completion words

		UIEditorPtr editor = UISystem::get().getEditorManager().getActiveEditor();

		SmartSourceFilePtr smartFile = nullptr;
		std::shared_ptr<SmartSense> smartSense = PaperCode::get().getSmartSense();

		if (editor && smartSense) {
			std::string filePath = editor->getFilePath().string();
			smartFile = smartSense->getDatabase().getFile(filePath);

			CompletionRequestArgs args;

			args.mFilePath = filePath;
			args.mUnsavedText = editor->mImEditor->GetText();
			args.mLine = pos.mLine + 1;
			args.mColumn = pos.mColumn + 1;

			smartSense->requestCodeCompleteAt(args, mAutoCompleteList);

			// TODO: TOO EXPENSIVE 
			std::sort(mAutoCompleteList.begin(), mAutoCompleteList.end(), 
				[](const SmartCompletioResult& s1, const SmartCompletioResult& s2){
					return s1.mName < s2.mName;
				});
		}
	}

	// Now we have a list to match with the word we typed in
	if (!mAutoCompleteList.empty()) {

		if (!mShowAutoComplete) {
			mShowAutoComplete = true;
		}
		if (!mAutoCompleteWord.empty()) {
			// TODO: TOO EXPENSIVE 
			int idx = 0;
			for (auto& v : mAutoCompleteList) {
				//printf(":%s: == :%s:\n", v.c_str(), mAutoCompleteWord.c_str());
		  		if (v.mName.starts_with(mAutoCompleteWord)) {
					mAutoCompleteBestMatchIndex = idx;
					break;
				}
				idx++;
			}
		}
	} else {
		ResetAutoComplete();
	}
}

std::shared_ptr<SmartSense> newSmartSense() {
	auto sense = std::make_shared<SmartSense>();
    sense->init();
    return sense;
}

void notifySmartSense(const std::string& filepath, const std::string& buf) {
	auto smartSense = PaperCode::get().getSmartSense();
    if (smartSense) {
        smartSense->notifyFileModified(filepath, buf);
    }
}

void drawSmartSenseState() {
	const ImGuiStyle& style = ImGui::GetStyle();

	auto smartSense = PaperCode::get().getSmartSense();
    if (smartSense) {
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (style.WindowPadding.x*2));

        SmartSenseState state = smartSense->getState();

        std::string filename_str = std::filesystem::path(smartSense->getCurrentFileName()).filename().string();
        const char* filename = filename_str.c_str();

        if (state == SmartSenseState::FullScanning) { 
            ImGui::Text("| SmartSense: %s: %s...", "Full scanning", filename);
        } else if (state == SmartSenseState::QuickScanning) {
            ImGui::Text("| SmartSense: %s: %s...", "Quick scanning", filename);
        }
    }
}

