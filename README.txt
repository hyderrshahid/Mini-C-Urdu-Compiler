COMPILER CONSTRUCTION — PHASE 2 C++ LL(1) PARSER

This folder extends the user's Phase 1 scanner with a table-driven LL(1) parser.
The original scanner.cpp/scanner.h have been preserved except for adding comma (,) to
getCharClass punctuation. The parser implementation is in parser.cpp.

FILES
scanner.cpp / scanner.h  Original Phase 1 lexer (+ comma punctuation only)
parser.cpp               Custom array-backed stack, token mapping, predictive loop,
                         panic-mode recovery and trace/output logic
ll1_data.h               Generated productions, LL(1) table and FOLLOW synchronizing cells
source.txt               Current program to parse (starts as Sample A)
SampleA.txt              Required valid assignment sample
SampleB.txt              Required invalid assignment sample
SampleRecovery.txt       Extra example that triggers SCAN recovery
outputs/                 Verified output snapshots for each test

BUILD (terminal / MinGW / g++)
  g++ -std=c++11 scanner.cpp -o scanner
  g++ -std=c++11 parser.cpp -o parser

RUN
  1) Copy the selected sample to source.txt.
     Windows: copy SampleA.txt source.txt
     macOS/Linux: cp SampleA.txt source.txt
  2) Run scanner first (scanner.exe on Windows or ./scanner on Unix).
  3) Run parser next (parser.exe on Windows or ./parser on Unix).
  4) Review ParseTrace.txt, SyntaxErrors.txt, ParseResult.txt.

OUTPUTS
The scanner retains its original tokens.txt and errors.txt filenames.
The parser also copies those results to required Token.txt and Error.txt aliases.
Parser results: ParseTrace.txt, SyntaxErrors.txt, ParseResult.txt.

GRAMMAR / TABLE
The code implements the LL(1) conversion discussed in this chat,
using new nonterminal names ProgramPrime, ArgListPrime, IdentListPrime,
ExprPrime, NonIdFactor, RvaluePrime, MagPrime and TermPrime.
Production entries are concrete and the predictive matrix is stored as
int parsingTable[NT_COUNT][TERM_COUNT]. A -1 cell means parsing error.
The grammar compiler verified 28 nonterminals, 64 productions,
31 terminals (including $), 155 filled entries, zero table conflicts.
This may use different NONTERMINAL LABELS and productions from the earlier
Excel workbook. Align Excel, handwritten notes and code before submission.

EXAMPLES VERIFIED
Sample A: ACCEPT, 0 syntax errors
Sample B: REJECT, 2 syntax errors, POP recovery
SampleRecovery: REJECT, 1 syntax error, SCAN recovery

NOTES / LIMITATIONS
- The Phase 1 lexer was otherwise left untouched by request. Its existing
  lexical edge cases and fixed lexeme buffer are not silently rewritten.
- The parser uses arrays and a custom resizing stack, not STL containers.
- The parser uses the scanner's legacy `<TYPE, lexeme> at line N` token rows.
- The parser reports syntax errors; check Error.txt separately for lexical errors.
- The original lexical analyzer requires normal separator/end newline for
  reliable final-token recognition; the provided samples end in a newline.
- The handwritten FIRST/FOLLOW, converted grammar and Excel matrix must be
  reconciled against ll1_data.h before final submission.
