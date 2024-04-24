module;

#include "Stdafx.h"
#include <map>
#include <unordered_map>
#include <chrono>
#include <atomic>
#include <mutex>

#include <clang-c/Index.h>  // This is libclang.

export module smartsense;
 
import project;
import manager;

using namespace::std::literals;

export enum class SmartSymbolType {
	Invalid,
	Function,
	Aggregate,
};

export struct SmartSymbol {
	std::string mName = "";
	SmartSymbolType mType = SmartSymbolType::Invalid;
	std::string mCalltip = "Call me!";

	bool isFunction() const { 
		return mType == SmartSymbolType::Function; 
	}
};

export using SmartSymbolPtr = std::shared_ptr<SmartSymbol>;
export using SymbolMap = std::unordered_map<std::string, SmartSymbolPtr>;

export struct SmartSourceFile {
	CXIndex mIndex = nullptr;
	CXTranslationUnit mTransUnit = nullptr;

	std::string mFilePath;
	SymbolMap mSymbols;

	SmartSourceFile() {

	}

	~SmartSourceFile() {
		if (mTransUnit) {
			clang_disposeTranslationUnit(mTransUnit);
			mTransUnit = nullptr;
		}
		if (mIndex)
			clang_disposeIndex(mIndex);
		mIndex = nullptr;
	}

	SymbolMap& getSymbolMap() {
		return mSymbols;
	}

	const std::string& getFilePath() { return mFilePath; }

	void clearEverything() {
		mSymbols.clear();
	}

	SmartSymbolPtr insert(const std::string& name, SmartSymbolType type) {
		auto sym = std::make_shared<SmartSymbol>();
		sym->mName = name;
		sym->mType = type;
		return insert(sym);
	}

	SmartSymbolPtr insert(SmartSymbolPtr sym) {
		auto it = mSymbols.find(sym->mName);
		if (it != mSymbols.end()) {
			return nullptr;
		}
		mSymbols[sym->mName] = sym;
		return sym;
	}
};

export using SmartSourceFilePtr = std::shared_ptr<SmartSourceFile>;
export using SourceFileMap = std::unordered_map<std::string, SmartSourceFilePtr>;

export struct SmartDatabase {
	SourceFileMap mFiles;

	SmartSourceFilePtr getFile(const std::string& filepath) {
		auto it = mFiles.find(filepath);
		if (it == mFiles.end()) {
			auto file = std::make_shared<SmartSourceFile>();
			mFiles[filepath] = file;
			return file;
		} else {
			return it->second;
		}
	}
};

export enum class SmartSenseState {
	Idle,
	QuickScanning,
	FullScanning,
	Error,
};

static std::string Convert(const CXString& s) {
    std::string result = clang_getCString(s);
    clang_disposeString(s);
    return result;
}

export struct SmartCompletioResult {
	std::string mName = "";
	std::string mSignature = "";
};

export using SmartCompletionCB = std::function<void(std::vector<SmartCompletioResult>)>;

export struct CompletionRequestArgs {
	std::string mFilePath;
	std::string mUnsavedText;
	int mLine;
	int mColumn;
	std::vector<SmartCompletioResult> mResult;
};

export struct SmartSense {

	std::jthread mThread;
	std::atomic_bool mTerminate = false;
	std::map<std::string, std::string> mFilesModified;
	int mQuckScanInterval = 0;
	std::mutex string_mutex;
	std::string mCurrentParsingFileName = "";

	SmartSenseState mState = SmartSenseState::Idle;

	// This object needs to be 'thread-safe' since this will be 
	// accessed by both smart sense thread and our main thread
	SmartDatabase mDatabase;

	SmartSense() {

	}

	~SmartSense() {
		terminate();
	}

	bool init() {

		mTerminate = false;

		mThread = std::jthread([this] () {

			mCurrentParsingFileName = "";

			fullScan();
			while(!mTerminate) {
				scan();
			}
		});

		return true;
	}

	void cleanUp() {
		
	}

	void terminate() {
		mTerminate = true;
		cleanUp();
	}

	std::string getCurrentFileName() {
		return mCurrentParsingFileName;
	}

	SmartDatabase& getDatabase() {
		return mDatabase;
	}

	SmartSenseState getState() const { return mState; }

	void fullScan() {
		mState = SmartSenseState::FullScanning;
		std::cout << "SmartSense: Full scanning...." << std::endl;
		ProjectPtr project = Manager::get().getActiveProject();
		if (project) {
			for (ProjectFilePtr file : project->mFileList) {

				if (mTerminate) {
					break;
				}

				auto filePath = file->getAbsolutePath(project);

				std::ifstream t(filePath);
			    if (t.good()) {
			        std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
			        t.close();

					std::cout << "SmartSense: Parse: " << file->getFileName() << std::endl;
					parseFile(filePath.string(), str);
				}
			}
			std::cout << "SmartSense: Done scanning" << std::endl;
		}
		mState = SmartSenseState::Idle;
	}

	void requestCodeCompleteAt(const CompletionRequestArgs& args, std::vector<SmartCompletioResult>& results) {
		std::cout << std::format("SmartSense: Request! ({} : {})",  args.mLine,  args.mColumn) << std::endl;
		//mCurrentRequest = std::make_shared<CompletionRequest>();
		//mCurrentRequest->mCallback = cb;
		const char* filename = args.mFilePath.c_str();

		CXUnsavedFile unsaved_files;
		unsaved_files.Filename = filename;
		unsaved_files.Contents = args.mUnsavedText.c_str();
		unsaved_files.Length   = args.mUnsavedText.length();

		SmartSourceFilePtr smartFile = mDatabase.getFile(args.mFilePath);

		std::cout << "SmartSense: CodeComplete: Query Start..." << std::endl;

		CXCodeCompleteResults* ccResults = clang_codeCompleteAt(
			smartFile->mTransUnit, filename, 
			args.mLine, args.mColumn, 
			&unsaved_files, 1, 0);

		std::cout << "SmartSense: CodeComplete: Query End" << std::endl;

		if (ccResults) {

			if (!results.empty())
				results.clear();

			std::cout << "SmartSense: CodeComplete: Collect Results..." << std::endl;

			auto context = clang_codeCompleteGetContexts(ccResults);

			results.reserve(ccResults->NumResults);

			for(int i = 0;i < ccResults->NumResults;i++) {
				CXCompletionResult* result = &ccResults->Results[i];

				const CXCursorKind kind = result->CursorKind;
	            const CXCompletionString& string = result->CompletionString;

	            const CXAvailabilityKind availabilityKind = clang_getCompletionAvailability(string);
                if (availabilityKind == CXAvailability_NotAccessible || availabilityKind == CXAvailability_NotAvailable) {
                	continue;
                }

                std::string strkind = Convert(clang_getCursorKindSpelling(kind));

                SmartCompletioResult reqResult;

				const int chunkCount = clang_getNumCompletionChunks(string);
	            for (int j=0; j<chunkCount; ++j) {
	                const CXCompletionChunkKind chunkKind = clang_getCompletionChunkKind(string, j);
	                std::string text = Convert(clang_getCompletionChunkText(string, j));

	                if (chunkKind == CXCompletionChunk_TypedText) {
	                	reqResult.mName = text;
	                	reqResult.mSignature += text;
	                    //std::cout << "result: " << text << std::endl;
	                } else {
	                    reqResult.mSignature += text;
                    	if (chunkKind == CXCompletionChunk_ResultType) {
                        	reqResult.mSignature += ' ';
                    	}
	                }
	            }
	            results.push_back(reqResult);
	            //std::string strComment = Convert(clang_getCompletionBriefComment(string));
				//std::cout << "result: " << strkind << ", comment: " << strComment << std::endl;
			}

			clang_disposeCodeCompleteResults(ccResults);

			std::cout << "SmartSense: CodeComplete: Results Collected" << std::endl;
		}
	}

	bool parseFile(const std::string& filePath, const std::string& fullText) {

		const char* filename = filePath.c_str();

		if (!filePath.empty()) {
			std::unique_lock<std::mutex> lock(string_mutex);
			mCurrentParsingFileName = filePath;
		}

		SmartSourceFilePtr smartFile = mDatabase.getFile(filePath);
		smartFile->clearEverything();

		if (!smartFile->mIndex)
			smartFile->mIndex = clang_createIndex(1, 0);

		//printf("path %s\n", smartFile->mUnsavedFile.Filename);

		if (smartFile->mTransUnit)
			clang_disposeTranslationUnit(smartFile->mTransUnit);

		smartFile->mTransUnit  = clang_parseTranslationUnit(
								smartFile->mIndex,
								filename, nullptr, 0,
								nullptr, 0,
								/*CXTranslationUnit_DetailedPreprocessingRecord |*/
					            CXTranslationUnit_Incomplete |
					            CXTranslationUnit_PrecompiledPreamble |
					            CXTranslationUnit_CreatePreambleOnFirstParse |
					            CXTranslationUnit_KeepGoing |
					            CXTranslationUnit_CacheCompletionResults |
					            CXTranslationUnit_SkipFunctionBodies |
					            CXTranslationUnit_LimitSkipFunctionBodiesToPreamble |
					            clang_defaultEditingTranslationUnitOptions());
		if (smartFile->mTransUnit == nullptr) {
			std::cout << "SmartSense: Unable to parse translation unit. Quitting." << std::endl;
		} else {

			bool analyze = false;

			if (analyze) {
				analyseFile(smartFile);
			}
		}
		return true;
	}

	void analyseFile(SmartSourceFilePtr smartFile) {

		SmartSourceFile* smartFileRawPtr = smartFile.get();

		CXCursor cursor = clang_getTranslationUnitCursor(smartFile->mTransUnit); //Obtain a cursor at the root of the translation unit

		clang_visitChildren(cursor, [](CXCursor current_cursor, CXCursor parent, CXClientData client_data) {

			SmartSourceFile* smartFileRawPtr = (SmartSourceFile*)client_data;

			CXSourceLocation location = clang_getCursorLocation( current_cursor );

			// Let's only care about this file symbols (no include files staffs i guess)
		  	//if( clang_Location_isFromMainFile( location ) == 0 )
		    	//return CXChildVisit_Continue;

		    CXCursorKind cursor_kind = clang_getCursorKind(current_cursor);

		    if (cursor_kind == CXCursor_FunctionDecl || 
		    	cursor_kind == CXCursor_StructDecl || 
		    	cursor_kind == CXCursor_UnionDecl || 
		    	cursor_kind == CXCursor_EnumDecl || 
		    	cursor_kind == CXCursor_ClassDecl ||
		    	cursor_kind == CXCursor_TypedefDecl) {
			    //CXString kind_spelling = clang_getKindSpelling(cursor_kind);
			    //std::cout << "Kind: " << clang_getCString(kind_spelling);
			    //clang_disposeString(kind_spelling);

			    CXType cursor_type = clang_getCursorType(current_cursor);

			    CXString cursor_spelling = clang_getCursorSpelling(current_cursor);
		    	std::string cursor_name = Convert(cursor_spelling);

			    std::string calltip = "No Tip :(";

			    if (cursor_kind == CXCursor_FunctionDecl) {
			    	auto return_type = Convert(clang_getTypeSpelling(clang_getResultType(cursor_type)));

			    	std::string args = "";

			    	int num_args = clang_Cursor_getNumArguments(current_cursor);
				    for (int i = 0; i < num_args; ++i) {
				        auto arg_cursor = clang_Cursor_getArgument(current_cursor, i);
				        auto arg_name = Convert(clang_getCursorSpelling(arg_cursor));
				        if (arg_name.empty()) {
				            arg_name = "no name!";
				        }
				        auto arg_data_type = Convert(clang_getTypeSpelling(clang_getArgType(cursor_type, i)));

				        if (i != 0) {
				        	args += ", ";
				        }

				        args += std::format("{} {}", arg_data_type, arg_name);
				    }

			    	calltip = std::format("{} {}({})", return_type, cursor_name, args);
			    }
/*

			    CXString type_kind_spelling = clang_getTypeKindSpelling(cursor_type.kind);
			    std::cout << ", TypeKind: " << clang_getCString(type_kind_spelling);
			    clang_disposeString(type_kind_spelling);

			    CXString cursor_spelling = clang_getCursorSpelling(current_cursor);
		      	std::cout <<  ", Name: " << clang_getCString(cursor_spelling);
		      	clang_disposeString(cursor_spelling);
*/
		      	//std::cout <<  "Function: " << ;

		      	//std::cout <<  "Name: " << cursor_name << std::endl;

		      	SmartSymbolPtr sym = smartFileRawPtr->insert(cursor_name, 
		      		cursor_kind == CXCursor_FunctionDecl ? SmartSymbolType::Function : SmartSymbolType::Aggregate);

		      	if (sym) {
		      		sym->mCalltip = calltip;
		      	}

		      	//std::cout << std::endl;
		    } else  {
		    	CXString cursor_spelling = clang_getCursorSpelling(current_cursor);
		    	std::string cursor_name = clang_getCString(cursor_spelling);

		    	if( clang_Location_isFromMainFile( location ) ) {
			    	//std::cout <<  "Name: " << cursor_name << ", Kind: " << cursor_kind << std::endl;
			    }
/*
		    	if(cursor_name == "printf") {
		      		std::cout <<  "Name: " << cursor_name << " Kind: " << cursor_kind << std::endl;

		      		smartFileRawPtr->insertGlobalFunction(cursor_name);
		    	}
*/
		      	clang_disposeString(cursor_spelling);
		    }
		    return CXChildVisit_Recurse;
		    //return CXChildVisit_Continue;
		}, smartFileRawPtr);
	}

	void notifyFileAdded(const std::string& filePath, const std::string& fullText) {
		
	}

	void notifyFileModified(const std::string& filePath, const std::string& fullText) {
		mFilesModified[filePath] = fullText;
		mQuckScanInterval = 0;
	}

	// Quick Scan modified files (Reparse)
	void scan() {
		if (mFilesModified.empty()) {
			return;
		} else {
			// OKay so we have file(s) that has been modified and
			// We need to parse them again
			// Let's not hurry too much (to ensure lag-free typing)
			std::this_thread::sleep_for(500ms);
			mQuckScanInterval++;
			if (mQuckScanInterval < 5) {
				return;
			}
		}

		mState = SmartSenseState::QuickScanning;

		for (auto [filePath, smartFile] : mDatabase.mFiles) {
			if (mTerminate) {
				break;
			}

			auto it = mFilesModified.find(filePath);

			const char* filename = filePath.c_str();

			if (it != mFilesModified.end()) {

				std::cout << "SmartSense: ReParse: " << filePath << std::endl;

				if (!filePath.empty()) {
					std::unique_lock<std::mutex> lock(string_mutex);
					mCurrentParsingFileName = filePath;
				}

				CXUnsavedFile unsaved_files;
				unsaved_files.Filename = filename;
				unsaved_files.Contents = it->second.c_str();
				unsaved_files.Length   = it->second.length();

				auto ret = clang_reparseTranslationUnit(smartFile->mTransUnit, 1, &unsaved_files,
                             clang_defaultReparseOptions(smartFile->mTransUnit));

				std::cout << "SmartSense: Done ReParsing: " << ret << std::endl;

				mFilesModified.erase(it);
			}
		}
		for (const auto& [file, code] : mFilesModified) {
			std::cout << "SmartSense: Modified file: '" << file << "' does not exist in project." << std::endl;
		}
		mState = SmartSenseState::Idle;
	}
};





