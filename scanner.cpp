#include "scanner.h"
#include <iostream>

class Lexer {
private:
    FILE* sourceFile;
    int currentLine;
    CustomList<Token> tokens;
    CustomList<Error> errors;
 
    int transitionTable[30][18];
    bool advanceTable[30][18];
    bool isAcceptState[30];

    enum State {
        Start = 0,
        
        // Identifiers
        IdPending = 1, IdValid = 2,
        
        // Numbers
        NumInt = 3, NumDot = 4, NumFloat = 5, 
        NumE = 6, NumESign = 7, NumEDigit = 8,
        
        // Strings
        StrBody = 9,
        
        // Operators
        OpColon = 10, OpEq = 11, OpLt = 12, OpGt = 13,
        OpAnd = 14, OpOr = 15, OpNot = 16, OpPlus = 17, OpMinus = 18,
        
        // Final Accepting States
        AcceptId = 19, AcceptNum = 20, AcceptStr = 21, 
        AcceptOpRetract = 22, AcceptOpExact = 23, AcceptPunct = 24, AcceptWs = 25,
        
        // Error Trap States
        ErrNoUnderscore = 26, ErrInvalid = 27
    };

   void setupTables() {
        // 1. Set Defaults
        for (int i = 0; i < 30; i++) {
            isAcceptState[i] = false;
            for (int j = 0; j < 18; j++) {
                transitionTable[i][j] = ErrInvalid; 
                advanceTable[i][j] = true;          
            }
        }

        // 2. Define Accept States
        isAcceptState[AcceptId] = true; 
        isAcceptState[AcceptNum] = true; 
        isAcceptState[AcceptStr] = true; 
        isAcceptState[AcceptOpRetract] = true; 
        isAcceptState[AcceptOpExact] = true; 
        isAcceptState[AcceptPunct] = true; 
        isAcceptState[AcceptWs] = true; 
        isAcceptState[ErrNoUnderscore] = true; 
        isAcceptState[ErrInvalid] = true;

        // 3. Start State Branches
        transitionTable[Start][0] = AcceptWs;          
        transitionTable[Start][1] = IdPending;    
        transitionTable[Start][2] = NumInt;       
        transitionTable[Start][3] = IdValid;
        transitionTable[Start][5] = IdPending;    
        transitionTable[Start][6] = OpPlus;       
        transitionTable[Start][7] = OpMinus;      
        transitionTable[Start][8] = OpColon;      
        transitionTable[Start][9] = OpEq;         
        transitionTable[Start][10] = OpLt;        
        transitionTable[Start][11] = OpGt;        
        transitionTable[Start][12] = OpAnd;       
        transitionTable[Start][13] = OpOr;        
        transitionTable[Start][14] = OpNot;       
        transitionTable[Start][15] = StrBody;     
        transitionTable[Start][16] = AcceptPunct;      
        transitionTable[Start][17] = ErrInvalid;    
        
        
        // 4. Identifier Paths 
        for (int col = 0; col < 18; col++) {
            // Letter start (1 = Letter, 2 = Digit, 5 = E/e)
            if (col == 1 || col == 2 || col == 5) 
                transitionTable[IdPending][col] = IdPending; 
            else if (col == 3) 
                transitionTable[IdPending][col] = IdValid;          
            else { 
                transitionTable[IdPending][col] = ErrNoUnderscore; 
                advanceTable[IdPending][col] = false; 
            }

            // Valid identifier 
            if (col == 1 || col == 2 || col == 3 || col == 5) 
                transitionTable[IdValid][col] = IdValid;
            else {
                transitionTable[IdValid][col] = AcceptId;       
                advanceTable[IdValid][col] = false;   
            }
        }

        // 5. Number Paths
        for (int col = 0; col < 18; col++) {
            if (col == 2) transitionTable[NumInt][col] = NumInt;
            else if (col == 4) transitionTable[NumInt][col] = NumDot; 
            else if (col == 5) transitionTable[NumInt][col] = NumE;   
            else { transitionTable[NumInt][col] = AcceptNum; advanceTable[NumInt][col] = false; }
            
            if (col == 2) transitionTable[NumDot][col] = NumFloat;
            if (col == 2) transitionTable[NumFloat][col] = NumFloat;
            else if (col == 5) transitionTable[NumFloat][col] = NumE;
            else { transitionTable[NumFloat][col] = AcceptNum; advanceTable[NumFloat][col] = false; }
            
            if (col == 6 || col == 7) transitionTable[NumE][col] = NumESign; 
            else if (col == 2) transitionTable[NumE][col] = NumEDigit;
            
            if (col == 2) transitionTable[NumESign][col] = NumEDigit;
            
            if (col == 2) transitionTable[NumEDigit][col] = NumEDigit;
            else { transitionTable[NumEDigit][col] = AcceptNum; advanceTable[NumEDigit][col] = false; }
        }

        // 6. Operator Paths
        for(int col = 0; col < 18; col++) {
            transitionTable[OpColon][col] = AcceptOpRetract; advanceTable[OpColon][col] = false;
            transitionTable[OpMinus][col] = AcceptOpRetract; advanceTable[OpMinus][col] = false;
            transitionTable[OpLt][col] = AcceptOpRetract;    advanceTable[OpLt][col] = false;
            transitionTable[OpGt][col] = AcceptOpRetract;    advanceTable[OpGt][col] = false;
            transitionTable[OpPlus][col] = AcceptOpRetract;  advanceTable[OpPlus][col] = false;

            transitionTable[OpEq][col] = ErrInvalid;         advanceTable[OpEq][col] = false;
            transitionTable[OpAnd][col] = ErrInvalid;        advanceTable[OpAnd][col] = false;
            transitionTable[OpOr][col] = ErrInvalid;         advanceTable[OpOr][col] = false;
            transitionTable[OpNot][col] = ErrInvalid;        advanceTable[OpNot][col] = false;
        }
        
        transitionTable[OpColon][8] = AcceptOpExact; advanceTable[OpColon][8] = true; 
        transitionTable[OpColon][9] = AcceptOpExact; advanceTable[OpColon][9] = true; 
        transitionTable[OpEq][9] = AcceptOpExact;    advanceTable[OpEq][9] = true;    
        
        transitionTable[OpLt][9] = AcceptOpExact;    advanceTable[OpLt][9] = true;    
        transitionTable[OpLt][10] = AcceptOpExact;   advanceTable[OpLt][10] = true;   
        transitionTable[OpLt][11] = AcceptOpExact;   advanceTable[OpLt][11] = true;   
        
        transitionTable[OpGt][9] = AcceptOpExact;    advanceTable[OpGt][9] = true;    
        transitionTable[OpGt][11] = AcceptOpExact;   advanceTable[OpGt][11] = true;   
        
        transitionTable[OpAnd][12] = AcceptOpExact;  advanceTable[OpAnd][12] = true;  
        transitionTable[OpOr][13] = AcceptOpExact;   advanceTable[OpOr][13] = true;   
        transitionTable[OpNot][9] = AcceptOpExact;   advanceTable[OpNot][9] = true;   
        
        transitionTable[OpPlus][6] = AcceptOpExact;  advanceTable[OpPlus][6] = true;  
        transitionTable[OpPlus][9] = AcceptOpExact;  advanceTable[OpPlus][9] = true;  

        transitionTable[OpMinus][2] = NumInt; advanceTable[OpMinus][2] = true; 
        transitionTable[OpPlus][2] = NumInt;  advanceTable[OpPlus][2] = true; 

        // 7. String Literal Path
        for (int col = 0; col < 18; col++) {
            if (col == 15) 
            transitionTable[StrBody][col] = AcceptStr; 
            else 
            transitionTable[StrBody][col] = StrBody;             
        }
    }
    void categorizeToken(int state, const char* lexeme) {
        if (state == AcceptId) {
            saveToken("IDENTIFIER", lexeme);
        } 
        else if (state == ErrNoUnderscore) {
            // Check if it's a keyword before declaring an error
            bool isKeyword = false;
            for (int i = 0; i < n; i++) {
                if (strcmp(lexeme, keywords[i]) == 0) {
                    isKeyword = true;
                    break;
                }
            }

            if (isKeyword) {
                saveToken("KEYWORD", lexeme);
            } else {
                saveError("Lexical Error: Identifier missing underscore", lexeme);
            }
        }
        else if (state == AcceptNum) { saveToken("NUMBER", lexeme); } 
        else if (state == AcceptStr) { saveToken("STRING", lexeme); } 
        else if (state == AcceptOpExact || state == AcceptOpRetract || state == AcceptPunct) {
            saveToken("OPERATOR", lexeme);
        } 
        else if (state == ErrInvalid) {
            saveError("Lexical Error: Invalid operator or character sequence", lexeme);
        }
    }

    void saveToken(const char* type, const char* lexeme) {
        Token t;
        strcpy(t.type, type);
        strcpy(t.lexeme, lexeme);
        t.line = currentLine;
        tokens.push(t);
    }

    void saveError(const char* msg, const char* lexeme) {
        Error e;
        strcpy(e.message, msg);
        strcpy(e.lexeme, lexeme);
        e.line = currentLine;
        errors.push(e);
    }

public:
    Lexer(const char* filename) {
        sourceFile = fopen(filename, "r");
        currentLine = 1;
        setupTables(); 
    }

    ~Lexer() {
        if (sourceFile) fclose(sourceFile);
    }

    int getCharClass(char c) {
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') return 0;

        if (c == 'E' || c == 'e') return 5;
        
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) return 1; 
        if (c >= '0' && c <= '9') return 2;                             
        if (c == '_') return 3;                                         
        if (c == '.') return 4;
        if (c == '+') return 6;
        if (c == '-') return 7;
        if (c == ':') return 8;
        if (c == '=') return 9;
        if (c == '<') return 10;
        if (c == '>') return 11;
        if (c == '&') return 12;
        if (c == '|') return 13;
        if (c == '!') return 14;
        if (c == '"') return 15;
        if (c == ',' || c == '%' || c == '/' || c == '*' || c == '(' || c == ')' || 
            c == '{' || c == '}' || c == '[' || c == ']') {
            return 16;
        }
        return 17; 
    }

    void analyze() {
        if (!sourceFile) {
            std::cout << "Error: Could not open source file.\n";
            return;
        }

        char lexemeBuffer[100];
        int lexIndex = 0;
        int currentState = Start;
        
        char c = fgetc(sourceFile);

        while (c != EOF) {
            if (c == '\n') currentLine++;

            if (currentState == Start && (c == ' ' || c == '\t' || c == '\n' || c == '\r')) {
                c = fgetc(sourceFile);
                continue;
            }

            int charClass = getCharClass(c);
            int nextState = transitionTable[currentState][charClass];
            bool shouldAdvance = advanceTable[currentState][charClass];

            if (shouldAdvance) {
                lexemeBuffer[lexIndex++] = c;
                c = fgetc(sourceFile); 
            }

            currentState = nextState;

            if (isAcceptState[currentState]) {
                lexemeBuffer[lexIndex] = '\0'; 

                if (strcmp(lexemeBuffer, "input") == 0 && c == '-') {
                    char nextChar = fgetc(sourceFile);
                    if (nextChar == '>') {
                        strcat(lexemeBuffer, "->");
                        c = fgetc(sourceFile); 
                    } else {
                        ungetc(nextChar, sourceFile); 
                    }
                } 

                categorizeToken(currentState, lexemeBuffer);

                currentState = Start;
                lexIndex = 0;
                lexemeBuffer[0] = '\0';
            }
        }
        
        writeOutputFiles();
    }
    void writeOutputFiles() {
        FILE* tokFile = fopen("tokens.txt", "w");
        for (int i = 0; i < tokens.size(); i++) {
            Token t = tokens.get(i);
            fprintf(tokFile, "<%s, %s> at line %d\n", t.type, t.lexeme, t.line);
        }
        fclose(tokFile);

        FILE* errFile = fopen("errors.txt", "w");
        for (int i = 0; i < errors.size(); i++) {
            Error e = errors.get(i);
            fprintf(errFile, "Line %d: %s -> '%s'\n", e.line, e.message, e.lexeme);
        }
        fclose(errFile);
        
        std::cout << "Lexical analysis complete. Check tokens.txt and errors.txt.\n";
    }
};

int main() {
    Lexer lexer("source.txt");
    lexer.analyze();
    return 0;
}