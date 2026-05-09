#include "ParseError.h"
#include <sstream>
using namespace std;

// Stores all four fields; offendingText may be empty for errors with no recoverable text.
ParseError::ParseError(string message, int line, int column,
                       string offendingText)
    : message_(move(message)), line_(line), column_(column),
      offendingText_(move(offendingText)) {}

// Builds "ParseError [line:col] message" and appends ("offendingText") when present.
string ParseError::toString() const {
    ostringstream oss;
    oss << "ParseError [" << line_ << ":" << column_ << "] " << message_;
    if (!offendingText_.empty())
        oss << " (\"" << offendingText_ << "\")";
    return oss.str();
}

// Returns the plain message without location information.
const string& ParseError::getMessage() const { return message_; }

// Returns the 1-based line where the error was detected.
int ParseError::getLine() const { return line_; }

// Returns the 1-based column where the error was detected.
int ParseError::getColumn() const { return column_; }
