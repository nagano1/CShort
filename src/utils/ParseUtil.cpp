#include <string.h>
#include "common.hpp"
#include "ParseUtil.hpp"

// --------------------------------------------------------------------------
// 
//                               ParseUtils
// 
// --------------------------------------------------------------------------



int ParseUtil::_matchFirstWithTrim(const char *chars, int charsLength, const char *target, int start)
{
    int currentTargetIndex = 0;
    int matchStartIndex = -1;

    for (int i = start; true; i++) {
        if (target[currentTargetIndex] == '\0') { // allow match at end-of-input
            break;
        }
        if (i < 0 || i >= charsLength) { // end of chars, match failed (chars may not be null-terminated)
            return -1;
        }
        auto ch = chars[i];

        // use & 0x80 to determine if it's an ascii char; target is expected to be ascii.
        if ((static_cast<unsigned char>(ch) & 0x80u) != 0) {
            return -1;
        }

        if (matchStartIndex == -1) {
            if (ch == ' ') { //allow trim
                continue;
            }
            else if (ch == '\t' || ch == '\n' || ch == '\r') { // allow trim
                continue;
            }
            else { // trim finished, start to match target
                matchStartIndex = i;
                currentTargetIndex = 0;
            }
        }

        if (target[currentTargetIndex] == '\0') {
            // match success and target has been fully matched, return the start index of match
            break;
        }

        assert(matchStartIndex != -1);
        if (target[currentTargetIndex] == chars[i]) { // continue to match next char in target
            currentTargetIndex++;
        }
        else {
            return -1;
        }
    }

    if (currentTargetIndex == 0) { // no char in target has been matched, match failed
        return -1;
    }
    else {
        return matchStartIndex;
    }
}


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
