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
        auto ch = chars[i];

        // use & and != 0x80 to determine if it's a ascii char, since the target should be ascii char.
        // ascii char has 0 in the most significant bit, while non-ascii char has 1 in the most significant bit,
        // so we can use & with 0x80 to determine if it's ascii char or not
        if ((ch & 0x80) == 0x80) {
            return -1;
        }

        if (ch == '\0') { // end of chars, match failed
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
};


bool ParseUtil::IsKeyword(utf8byte *ch, st_int length)
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
