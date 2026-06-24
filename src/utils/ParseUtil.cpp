#include <string.h>
#include "common.hpp"
#include "ParseUtil.hpp"

// --------------------------------------------------------------------------
//                               ParseUtil
// --------------------------------------------------------------------------


bool ParseUtil::IsKeyword(const utf8byte *ch, st_int length)
{
    constexpr const char* keywords[] = {"return", "class", "fn", "false", "true", "null", "ret", "Ptr", "let", "if", "else", "while", "for", "break", "continue", "switch", "case", "default", "import", "export", "using", "as", "in", "is", "new", "delete", "try", "catch", "throw", "finally", "const", "var", "struct", "enum", "interface", "extends", "implements", "public", "private", "protected", "static", "abstract"};
    for (const char* keyword : keywords)
    {
        const st_size keywordLength = static_cast<st_size>(strlen(keyword));
        // printf("IsKeyword check: %.*s, keyword: %s\n", length, ch, keyword);
        if (static_cast<st_size>(length) == keywordLength && ParseUtil::matchWord(ch, static_cast<st_size>(length), keyword, keywordLength, 0))
        {
            return true;
        }
    }
    return false;
}
