#include <stdio.h>
#include <ctype.h>
#include <string.h>

typedef enum { S0, S1, S2, S3, DEAD } State;

int isKeyword(char *str) {
    char *keywords[] = {"int","float","char","double","if","else","while",
                         "for","return","void","break","continue"};
    int n = sizeof(keywords)/sizeof(keywords[0]);
    for (int i = 0; i < n; i++)
        if (strcmp(str, keywords[i]) == 0) return 1;
    return 0;
}

// DFA for identifier/keyword: letter (letter|digit)*
void scanIdentifier(FILE *fp, char firstChar) {
    char buffer[100];
    int i = 0;
    State state = S0;
    char ch = firstChar;

    buffer[i++] = ch;   // consumed while entering S0->S1
    state = S1;         // S1 = accepting state

    while (1) {
        ch = fgetc(fp);
        if (isalnum(ch) || ch == '_') {
            buffer[i++] = ch;   // stay in S1 (loop transition)
            state = S1;
        } else {
            ungetc(ch, fp);     // no valid transition -> stop
            break;
        }
    }
    buffer[i] = '\0';

    if (state == S1) {
        if (isKeyword(buffer))
            printf("Keyword: %s\n", buffer);
        else
            printf("Identifier: %s\n", buffer);
    }
}

// DFA for number: digit+ ('.' digit+)?
// States: S0 (start) -> S1 (integer part, final) -> S2 (seen '.') -> S3 (fraction part, final)
void scanNumber(FILE *fp, char firstChar) {
    char buffer[100];
    int i = 0;
    State state = S1;      // after first digit, we're already in S1
    char ch = firstChar;
    buffer[i++] = ch;

    while (1) {
        ch = fgetc(fp);
        if (state == S1 && isdigit(ch)) {
            buffer[i++] = ch;               // S1 -> S1
        } else if (state == S1 && ch == '.') {
            buffer[i++] = ch;
            state = S2;                     // S1 -> S2
        } else if (state == S2 && isdigit(ch)) {
            buffer[i++] = ch;
            state = S3;                     // S2 -> S3 (final)
        } else if (state == S3 && isdigit(ch)) {
            buffer[i++] = ch;               // S3 -> S3
        } else {
            ungetc(ch, fp);
            break;
        }
    }
    buffer[i] = '\0';

    if (state == S1)
        printf("Integer Constant: %s\n", buffer);
    else if (state == S3)
        printf("Float Constant: %s\n", buffer);
    else
        printf("Error: Invalid number '%s' (dangling '.')\n", buffer);
}

// DFA for '/' : could be operator '/', or start of comment
void scanSlash(FILE *fp) {
    char next = fgetc(fp);
    if (next == '/') {                      // S0 -'/' -> S1 -'/' -> S2 (comment)
        char ch;
        while ((ch = fgetc(fp)) != '\n' && ch != EOF);  // stay in S2 until '\n'
        // reached final state S3 -> discard
    } else if (next == '*') {
        char prev = 0, ch;
        while ((ch = fgetc(fp)) != EOF) {   // S2 loop until '*' then '/'
            if (prev == '*' && ch == '/') break;   // reached final state S4
            prev = ch;
        }
    } else {
        printf("Operator: /\n");
        ungetc(next, fp);
    }
}

int main() {
    FILE *fp = fopen("sample.c", "r");
    if (!fp) { printf("Could not open file\n"); return 1; }

    char ch;
    while ((ch = fgetc(fp)) != EOF) {
        if (ch == ' ' || ch == '\t' || ch == '\n') continue;   // whitespace DFA (discard)

        if (isalpha(ch) || ch == '_') { scanIdentifier(fp, ch); continue; }
        if (isdigit(ch)) { scanNumber(fp, ch); continue; }
        if (ch == '/') { scanSlash(fp); continue; }

        if (strchr("+-*%=<>!&|", ch)) {
            char next = fgetc(fp);
            char op2[3] = {ch, next, '\0'};
            if ((ch=='='&&next=='=')||(ch=='!'&&next=='=')||(ch=='<'&&next=='=')||
                (ch=='>'&&next=='=')||(ch=='&'&&next=='&')||(ch=='|'&&next=='|')||
                (ch=='+'&&next=='+')||(ch=='-'&&next=='-'))
                printf("Operator: %s\n", op2);
            else {
                printf("Operator: %c\n", ch);
                ungetc(next, fp);
            }
            continue;
        }

        if (strchr("(){}[];,#", ch)) {
            printf("Punctuation: %c\n", ch);
            continue;
        }

        if (ch == '"') {
            char buf[100]; int i = 0;
            buf[i++] = ch;
            while ((ch = fgetc(fp)) != '"' && ch != EOF) buf[i++] = ch;
            buf[i++] = '"'; buf[i] = '\0';
            printf("String Literal: %s\n", buf);
            continue;
        }
    }

    fclose(fp);
    return 0;
}