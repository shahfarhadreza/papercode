module;

#include <vector>
#include <string>

export module logsystem;

export enum class LogType {
    General,
    Info,
    Warning,
    Error,
    Success
};

export struct LogEntry {
    std::string Text;
    LogType Type;
};

export struct LogSystem {
    std::vector<LogEntry> mEntries;

    void log(const std::string& log) {
    	LogEntry entry;
    	entry.Text = log;
    	entry.Type = LogType::General;
    	mEntries.push_back(entry);
    }

    void log(const std::string& log, LogType type) {
    	LogEntry entry;
    	entry.Text = log;
    	entry.Type = type;
    	mEntries.push_back(entry);
    }

    void info(const std::string& log) {
    	LogEntry entry;
    	entry.Text = log;
    	entry.Type = LogType::Info;
    	mEntries.push_back(entry);
    }

    void warning(const std::string& log) {
    	LogEntry entry;
    	entry.Text = log;
    	entry.Type = LogType::Warning;
    	mEntries.push_back(entry);
    }

    void error(const std::string& log) {
    	LogEntry entry;
    	entry.Text = log;
    	entry.Type = LogType::Error;
    	mEntries.push_back(entry);
    }

    void success(const std::string& log) {
    	LogEntry entry;
    	entry.Text = log;
    	entry.Type = LogType::Success;
    	mEntries.push_back(entry);
    }

    void newline() {
    	LogEntry entry;
    	entry.Text = "\r\n";
    	entry.Type = LogType::General;
    	mEntries.push_back(entry);
    }

    void clear() {
    	mEntries.clear();
    }
};

