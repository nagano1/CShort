#include <string.h>
#include "common.hpp"
#include "ParseUtil.hpp"

// --------------------------------------------------------------------------
// 
//                               ParseUtils
// 
// --------------------------------------------------------------------------


bool ParseUtil::IsKeyword(const utf8byte *ch, st_int length)
{
    constexpr const char* keywords[] = {"return", "class", "fn", "false", "true", "null", "ret"};
    for (auto &&keyword : keywords)
    {
        int keywordLength = strlen(keyword);
        // printf("IsKeyword check: %.*s, keyword: %s\n", length, ch, keyword);
        if (length == keywordLength && ParseUtil::matchWord(ch, length, keyword, keywordLength, 0))
        {
            return true;
        }
    }
    return false;
}
