#pragma once
#include "common.hpp"

struct ParseUtil {

    template<class T>
    static inline int detectOne(const T &tokenizer, const utf8byte *chars, utf8byte ch, int i) {
        if (tokenizer.first_char == ch) {
            auto idx = tokenizer.tryTokenize(chars, i);
            if (Search::IsTokenized(idx)) {
                return idx;
            }
        }

        return Search::NOTFOUND;
    };

    template<class T>
    static inline int detect(const T &tokenizer, const utf8byte *chars, utf8byte ch, int i) {
        for (int k = 0; k < tokenizer.first_chars_length; k++) {
            if (tokenizer.first_chars[k] == ch) {
                auto idx = tokenizer.tryTokenize(chars, i);
                if (Search::IsTokenized(idx)) {
                    return idx;
                }
            }
        }

        return Search::NOTFOUND;
    };

    static inline bool matchWord(const utf8byte *text,
        st_size text_length,
        const char *word, st_size word_length,
        st_uint start)
    {
        if (start + word_length <= text_length) { // determine word has enough length
            for (st_uint i = 0; i < word_length; i++) {
                if (text[start + i] != word[i]) {
                    return false;
                }
            }
            return true;
        }

        return false;
    }

    static bool IsKeyword(const utf8byte *ch, st_int length);

    /// match target at startIndex, and ensure the char after target is terminatable 
    template<int SIZE>
    static bool matchWordWithTerminatableEnd(const char *chars, int charsLength, int startIndex, const char(&target)[SIZE])
    {
        if (matchWord(chars, charsLength, target, SIZE - 1, startIndex)) {
            int terminatorIndex = startIndex + SIZE - 1; // SIZE includes null terminator
            if (terminatorIndex == charsLength) { // end-of-input is a valid terminator
                return true;
            }
            if (terminatorIndex < charsLength && ParseUtil::isTerminatableChar(chars[terminatorIndex])) {
                return true;
            }
        }

        return false;
    }

    static bool hasCharBeforeLineBreak(const char *chars, int charsLength, int startIndex) {
        for (int i = startIndex; i < charsLength; i++) {
            if (chars[i] == ' ' || chars[i] == '\r') {
                continue;
            } else  if (chars[i] == '\n' || chars[i] == '\0') {
                return false;
            } else {
                return true;
            }
        }

        return false;
    }


    static inline bool isTerminatableChar(utf8byte ch)
    {
        // A keyword can terminate before any character that cannot appear in an identifier.
        return ParseUtil::isNonIdentifierChar(ch);
    }


    struct IndexOfBreakOrEndResult {
        int index;
        bool hasLineBreak;
    };
    static inline IndexOfBreakOrEndResult indexOfBreakOrEndWithInfo(const char *chars, int charsLength, int startIndex)
    {
        if (startIndex == charsLength) { // end of chars
            return {charsLength, false};
        }

        if (startIndex > charsLength) { // invalid start index
            return {-1, false};
        }

        for (int i = startIndex; i < charsLength; i++) {
            if ('\r' == chars[i] || '\n' == chars[i]) {
                return {i, true};
            }
            else if ('\0' == chars[i]) {
                return {i, false};
            }
        }
        return {charsLength, false};
    }

    static inline int indexOfBreakOrEnd(const char *chars, int charsLength, int startIndex)
    {
        return indexOfBreakOrEndWithInfo(chars, charsLength, startIndex).index;
        
    }



    static inline int indexOf(const char *chars, int charsLength, int startIndex, char ch)
    {
        for (int i = startIndex; i < charsLength; i++) {
            if (ch == chars[i]) {
                return i;
            }
        }
        return -1;
    }

    static inline int indexOf(const char *chars, int charsLength, int startIndex, const char* word, int wordLength)
    {
        for (int i = startIndex; i < charsLength - wordLength + 1; i++) {
            if (ParseUtil::matchWord(chars, charsLength, word, wordLength, i)) {
                return i;
            }
        }
        return -1;
    }

    static inline int indexOf2(const char *chars, int charsLength, int startIndex, char ch1, char ch2)
    {
        for (int i = startIndex; i < charsLength - 1; i++) {
            if (ch1 == chars[i] && ch2 == chars[i+1]) {
                return i;
            }
        }
        return -1;
    }


    // \t is currently converted into a single space for simplicity.
    static inline bool isSpace(utf8byte ch)
    {
        return ' ' == ch || '\t' == ch;
    }

    static inline bool endsWith(const char* chars, int charsLength, const char* suffix, int suffixLength)
    {
        if (charsLength < suffixLength) {
            return false;
        }

        for (int i = 0; i < suffixLength; i++) {
            int idx = charsLength - (suffixLength - i);
            if (chars[idx] != suffix[i]) {
                return false;
            }
        }

        return true;
    }

    static inline bool equals(const char* chars, int charsLength, const char* chars2, int chars2Length) {
        if (charsLength == chars2Length) {
            return endsWith(chars, charsLength, chars2, chars2Length);
        }
        return false;
    }


    template<int SIZE>
    static inline bool endsWith(const char* chars, int charsLength, const char(&suffix)[SIZE])
    {
        const int suffixLength = SIZE - 1;
        return endsWith(chars, charsLength, suffix, suffixLength);
    }


    static inline bool isNonIdentifierChar(utf8byte ch)
    {
        return ch == ' ' || '\t' == ch || '!' == ch || '#' == ch || '\n' == ch || '\r' == ch
               || '%' == ch || ']' == ch || '"' == ch || '[' == ch || '\'' == ch
               || '=' == ch || '*' == ch || '+' == ch || '-' == ch || '?' == ch
               || '(' == ch || ')' == ch
               || '@' == ch || '{' == ch || '}' == ch || ',' == ch || ';' == ch
               || ':' == ch || '.' == ch || '`' == ch || '&' == ch || '|' == ch
               || '<' == ch || '>' == ch || '^' == ch || '\\' == ch || '/' == ch
               || '\0' == ch;
    }


    static inline bool isBreakLine(utf8byte ch)
    {
        return '\r' == ch || '\n' == ch;
    }


    static inline bool isNumberLetter(utf8byte ch)
    {
        return '0' <= ch && ch <= '9';
    }


    static inline bool isIdentifierLetter(utf8byte ch)
    {
        if ('A' <= ch && ch <= 'Z') {
            return true;
        } else if ('a' <= ch && ch <= 'z') {
            return true;
        }
        else if ('0' <= ch && ch <= '9') {
            return true;
        }
        else if ('_' == ch) {
            return true;
        }

        // non-ascii char, allow it in identifier, since it's common in modern languages to allow non-ascii char in identifier,
        // and the target language of this parser is designed for modern programming, so we allow non-ascii char in identifier.
        // we can use & with 0x80 to determine if it's ascii char or not, since ascii char has 0 in the most significant bit,
        // while non-ascii char has 1 in the most significant bit.
        return (static_cast<unsigned char>(ch) & 0x80u) != 0;
    }

    static bool isValuePreservedWord(utf8byte ch, utf8byte* bytes, int len)
    {
        if (ch == 'n') {
            if (ParseUtil::equals(bytes, len, "null", 4)) {
                return false;
            }
        }
        else if (ch == 't') {
            if (ParseUtil::equals(bytes, len, "true", 4)) {
                return false;
            }
        }
        else if (ch == 'f') {
            if (ParseUtil::equals(bytes, len, "false", 4)) {
                return false;
            }
        }

        return true;
    }
};

