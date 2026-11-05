
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "ll1_data.h"

struct InputToken
{
    char symbol[50];
    char lexeme[100];
    int line;
};

class SymbolStack
{
private:
    char (*values)[64];
    int length;
    int capacity;

public:
    SymbolStack()
    {
        length = 0;
        capacity = 64;
        values = new char[capacity][64];
    }

    ~SymbolStack()
    {
        delete[] values;
    }

    void push(const char* s)
    {
        if (length == capacity)
        {
            capacity *= 2;

            char (*next)[64] = new char[capacity][64];

            for (int i = 0; i < length; i++)
            {
                strcpy(next[i], values[i]);
            }

            delete[] values;
            values = next;
        }

        strncpy(values[length], s, 63);
        values[length][63] = 0;
        length++;
    }

    void pop()
    {
        if (length)
        {
            length--;
        }
    }

    const char* top() const
    {
        return length ? values[length - 1] : "";
    }

    bool empty() const
    {
        return length == 0;
    }

    void print(FILE* f) const
    {
        for (int i = length - 1; i >= 0; i--)
        {
            fprintf(f, "%s ", values[i]);
        }
    }
};

int ntIndex(const char* s)
{
    for (int i = 0; i < NT_COUNT; i++)
    {
        if (strcmp(s, nonterminals[i]) == 0)
        {
            return i;
        }
    }

    return -1;
}

int termIndex(const char* s)
{
    for (int i = 0; i < TERM_COUNT; i++)
    {
        if (strcmp(s, terminals[i]) == 0)
        {
            return i;
        }
    }

    return -1;
}

void copyTextFile(const char* from, const char* to)
{
    FILE* in = fopen(from, "rb");

    if (!in)
    {
        return;
    }

    FILE* out = fopen(to, "wb");

    if (!out)
    {
        fclose(in);
        return;
    }

    int c;

    while ((c = fgetc(in)) != EOF)
    {
        fputc(c, out);
    }

    fclose(out);
    fclose(in);
}

int main()
{
    // Keep original Phase 1 filenames and also provide assignment-required aliases.
    copyTextFile("tokens.txt", "Token.txt");
    copyTextFile("errors.txt", "Error.txt");

    FILE* input = fopen("tokens.txt", "r");

    if (!input)
    {
        puts("Missing tokens.txt: run scanner first.");
        return 1;
    }

    InputToken* toks = new InputToken[4096];

    int count = 0;
    char kind[32];
    char lex[100];
    int lineno;

    char buffer[256];
    int lexicalMismatch = 0;

    while (fgets(buffer, sizeof(buffer), input))
    {
        // Legacy format: <TYPE, lexeme> at line N
        // Parse from the final delimiter so operators containing '>'
        // and the comma token work correctly.

        char* sep = strstr(buffer, ", ");
        char* ending = strstr(buffer, "> at line ");

        if (buffer[0] != '<' || !sep || !ending || sep >= ending)
        {
            printf("Unrecognized token row: %s", buffer);
            lexicalMismatch++;
            continue;
        }

        int kindLen = (int)(sep - (buffer + 1));
        int lexLen = (int)(ending - (sep + 2));

        if (kindLen <= 0 || kindLen >= 32 || lexLen <= 0 || lexLen >= 100)
        {
            lexicalMismatch++;
            continue;
        }

        memcpy(kind, buffer + 1, kindLen);
        kind[kindLen] = 0;

        memcpy(lex, sep + 2, lexLen);
        lex[lexLen] = 0;

        if (sscanf(ending + 10, "%d", &lineno) != 1)
        {
            lexicalMismatch++;
            continue;
        }

        if (count >= 4094)
        {
            puts("Too many tokens");
            fclose(input);
            delete[] toks;
            return 1;
        }

        InputToken& tok = toks[count++];

        strcpy(tok.lexeme, lex);
        tok.line = lineno;

        if (strcmp(kind, "IDENTIFIER") == 0)
        {
            strcpy(tok.symbol, "identifier");
        }
        else if (strcmp(kind, "NUMBER") == 0)
        {
            strcpy(tok.symbol, "number");
        }
        else
        {
            strcpy(tok.symbol, lex);
        }
    }

    fclose(input);

    int lastLine = count ? toks[count - 1].line : 1;

    strcpy(toks[count].symbol, "$");
    strcpy(toks[count].lexeme, "$");
    toks[count].line = lastLine;

    FILE* trace = fopen("ParseTrace.txt", "w");
    FILE* errors = fopen("SyntaxErrors.txt", "w");
    FILE* res = fopen("ParseResult.txt", "w");

    if (!trace || !errors || !res)
    {
        puts("Unable to write parser results");
        delete[] toks;
        return 1;
    }

    fprintf(trace, "Step | Stack (top left) | Remaining input | Production / Action | Error\n");

    SymbolStack stack;

    stack.push("$");
    stack.push("Program");

    int pos = 0;
    int step = 0;
    int syntaxErrors = lexicalMismatch;

    for (; step < 20000 && !stack.empty(); step++)
    {
        const char* top = stack.top();
        const char* look = toks[pos].symbol;

        int nonterminal = ntIndex(top);
        int col = termIndex(look);

        fprintf(trace, "%d | ", step + 1);
        stack.print(trace);
        fprintf(trace, "| ");

        for (int k = pos; k <= count && k < pos + 22; k++)
        {
            fprintf(trace, "%s ", toks[k].symbol);
        }

        fprintf(trace, "| ");

        if (strcmp(top, "$") == 0 && strcmp(look, "$") == 0)
        {
            fprintf(trace, "End of input | %s\n",
                    syntaxErrors ? "Errors recorded" : "ACCEPT");

            stack.pop();
            break;
        }

        if (nonterminal < 0)
        {
            if (strcmp(top, look) == 0)
            {
                fprintf(trace, "MATCH %s | -\n", look);
                stack.pop();
                pos++;
            }
            else
            {
                syntaxErrors++;

                fprintf(trace, "POP terminal %s | Expected '%s', found '%s'\n",
                        top, top, look);

                fprintf(errors,
                        "Line %d: Expected '%s', got '%s'. POP expected terminal.\n",
                        toks[pos].line, top, look);

                stack.pop();
            }
        }
        else if (col >= 0 && parsingTable[nonterminal][col] >= 0)
        {
            int production = parsingTable[nonterminal][col];

            fprintf(trace, "%s -> %s | -\n",
                    prodLhs[production], prodRhs[production]);

            stack.pop();

            if (strcmp(prodRhs[production], "ε") != 0)
            {
                char rhs[256];
                strcpy(rhs, prodRhs[production]);

                char parts[24][64];
                int total = 0;

                char* part = strtok(rhs, " ");

                while (part && total < 24)
                {
                    strcpy(parts[total++], part);
                    part = strtok(NULL, " ");
                }

                for (int i = total - 1; i >= 0; i--)
                {
                    stack.push(parts[i]);
                }
            }
        }
        else
        {
            syntaxErrors++;

            if (col >= 0 && isFollow[nonterminal][col])
            {
                fprintf(trace,
                        "POP %s | Unexpected '%s', synchronizing on FOLLOW\n",
                        top, look);

                fprintf(errors,
                        "Line %d: Unexpected '%s' while parsing %s. POP on FOLLOW.\n",
                        toks[pos].line, look, top);

                stack.pop();
            }
            else if (strcmp(look, "$") == 0)
            {
                fprintf(trace, "POP %s | End of input\n", top);

                fprintf(errors,
                        "Line %d: Premature end while parsing %s. POP.\n",
                        toks[pos].line, top);

                stack.pop();
            }
            else
            {
                fprintf(trace,
                        "SCAN %s | Unexpected '%s' while parsing %s\n",
                        look, look, top);

                fprintf(errors,
                        "Line %d: Unexpected '%s' while parsing %s. SCAN.\n",
                        toks[pos].line, look, top);

                pos++;
            }
        }
    }

    // Valid strings require no errors and both input/stack consumed.
    const bool success = stack.empty() &&
                         strcmp(toks[pos].symbol, "$") == 0 &&
                         syntaxErrors == 0;

    if (step >= 20000)
    {
        syntaxErrors++;

        fprintf(errors,
                "Safety limit reached while recovering.\n");
    }

    fprintf(res, "%s\nSyntax errors: %d\n",
            success ? "ACCEPT" : "REJECT", syntaxErrors);

    if (syntaxErrors == 0)
    {
        fprintf(errors, "No syntax errors.\n");
    }

    fclose(trace);
    fclose(errors);
    fclose(res);

    delete[] toks;

    printf("Parsing finished: %s (%d error(s)).\n",
           success ? "ACCEPT" : "REJECT", syntaxErrors);

    return success ? 0 : 2;
}
