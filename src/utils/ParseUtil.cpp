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
