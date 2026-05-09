#include "LexerError.h"
#include <sstream>
using namespace std;

// Stores all four fields; offendingText may be empty for errors with no recoverable text.
LexerError::LexerError(string message, int line, int column,
                       string offendingText)
    :message_(move(message)), line_(line), column_(column), offendingText_(move(offendingText)) {}

// Builds "LexerError [line:col] message" and appends ("offendingText") when present.
string LexerError::toString() const {
    ostringstream oss;
    oss << "LexerError [" << line_ << ":" << column_ << "] " << message_;
    if (!offendingText_.empty())
        oss << " (\"" << offendingText_ << "\")";
    return oss.str();
}

// Returns the plain message without location information.
const string& LexerError::getMessage() const { return message_; }

// Returns the 1-based line where the error was detected.
int LexerError::getLine()    const { return line_; }

// Returns the 1-based column where the error was detected.
int LexerError::getColumn()  const { return column_; }
