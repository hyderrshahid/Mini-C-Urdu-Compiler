#include <iostream>
#include <cstdio>
#include <cstring>


struct Token {
    char type[30];
    char lexeme[100];
    int line;
};

struct Error {
    char message[100];
    char lexeme[100];
    int line;
};

template <typename T>
class CustomList {
private:
    T* data;
    int capacity;
    int current_size;

public:
    CustomList() {
        capacity = 16;
        current_size = 0;
        data = new T[capacity];
    }

    ~CustomList() { 
        delete[] data; 
    }
    
    void push(T item) {
        if (current_size >= capacity) {
            capacity *= 2; 
            T* temp = new T[capacity];
            for (int i = 0; i < current_size; i++) {
                temp[i] = data[i]; 
            }
            delete[] data;
            data = temp;
        }
        data[current_size++] = item;
    }
    
    T get(int index) const { 
        return data[index]; 
    }
    
    int size() const { 
        return current_size; 
    }
};

const int n = 62;
const char* keywords[n] = {
    "asm", "Wagarna", "new", "this", "auto", "enum", "operator", "throw", "Mantiqi", 
    "explicit", "private", "True", "break", "export", "protected", "try", "case", 
    "extern", "public", "typedef", "catch", "False", "register", "typeid", "Harf", 
    "Ashriya", "typename", "Adadi", "class", "for", "Wapas", "union", "const", "dost", 
    "short", "unsigned", "goto", "signed", "using", "continue", "Agar", "sizeof", 
    "virtual", "default", "inline", "static", "Khali", "delete", "volatile", "do", 
    "long", "struct", "double", "mutable", "switch", "while", "namespace", "template", 
    "Marqazi", "Matn", "input->", "output-<"};