#pragma warning(push)
#pragma warning(disable : 4267) // conversion from 'size_t' to 'int', possible loss of data

#include <cstdio>

#include "code_nodes.hpp"
#include "ParseUtil.hpp"

using namespace cshort;

void callAllTests();
int main()
{
    callAllTests();
    return 0;
}

int testParseUtil() {
    assert(true == ParseUtil::isIdentifierLetter('a'));

    {
        static constexpr char chars[] = "class\n A{}";
        assert(5 == ParseUtil::indexOfBreakOrEnd(chars, sizeof(chars)-1, 0));

        static constexpr char chars2[] = "class\0 A{}";
        assert(5 == ParseUtil::indexOfBreakOrEnd(chars2, sizeof(chars2) - 1, 3));

        static constexpr char chars3[] = "class\0 A{}";
        assert(10 == ParseUtil::indexOfBreakOrEnd(chars3, sizeof(chars3) - 1, 7));
    }

    {
        static constexpr char chars[] = "class\n A{}";
        assert(7 == ParseUtil::indexOf(chars, sizeof(chars)-1, 0, 'A'));

        static constexpr char chars2[] = "class\n A{}";
        assert(-1 == ParseUtil::indexOf(chars2, sizeof(chars2)-1, 0, 'G'));

    }


    {
        static constexpr char chars[] = "return    \n";
        assert(false == ParseUtil::hasCharBeforeLineBreak(chars, sizeof(chars)-1, 6));

        static constexpr char chars2[] = "return    a\r\n";
        assert(true == ParseUtil::hasCharBeforeLineBreak(chars2, sizeof(chars2)-1, 6));

    }

    assert(true == ParseUtil::isIdentifierLetter('a'));
    assert(true == ParseUtil::isIdentifierLetter(std::string{ u8"😂" }.c_str()[0]));
    assert(false == ParseUtil::isIdentifierLetter('\n'));


    static constexpr char chars[] = "class A{}";
    assert(true == ParseUtil::matchWordWithTerminatableEnd(chars, sizeof(chars) - 1, 0, "class"));

    assert(false == ParseUtil::matchWordWithTerminatableEnd("", 0, 0, "class"));
    assert(true == ParseUtil::matchWordWithTerminatableEnd("", 0, 0, ""));

    constexpr char txt[] = "aefvariable aowef \n";
    assert(false == ParseUtil::matchWordWithTerminatableEnd(txt, sizeof(txt)-1, 2, "false"));

    {
        std::string class_text(u8"class auto * 😂日本語=10234;");
        assert(true == ParseUtil::matchWordWithTerminatableEnd(class_text.c_str(), class_text.length(), 0, "class"));
    }


    {
        std::string class_text(u8"😂classauto;");
        assert(false == ParseUtil::matchWordWithTerminatableEnd(class_text.c_str(), class_text.length(), 0, "class"));
    }


    // matchWord
    {
        std::string class_text(u8"class");
        assert(class_text.length() == 5);
        auto result = ParseUtil::matchWord(class_text.c_str(), class_text.length(), "class", 5, 0);
        assert(result == true);
    }

    {
        std::string class_text(u8" class"); //space
        auto result = ParseUtil::matchWord(class_text.c_str(), class_text.length(), "class", 5, 0);
        assert(result == false);
    }

    {
        std::string class_text(u8"abcclass");
        auto result = ParseUtil::matchWord(class_text.c_str(), class_text.length(), "class", 5, 3);
        assert(result == true);
    }

    {
        std::string class_text(u8"classauto;");
        auto result = ParseUtil::matchWord(class_text.c_str(), class_text.length(), "class", 5, 0);
        assert(result == true);
    }

    {
        std::string text(u8"ab");
        auto result = ParseUtil::matchWord(text.c_str(), text.length(), "abcdefg", 5, 0);
        assert(result == false);
    }

    {
        // endsWith
        {
            // 
            std::string text(u8"ab");
            auto result = ParseUtil::endsWith(text.c_str(), text.length(), "ab");
            assert(result == true);
        }

        {
            std::string text(u8"abcdefg");
            auto result = ParseUtil::endsWith(text.c_str(), text.length(), "efg");
            assert(result == true);
        }
        {
            std::string text(u8"abcd");
            auto result = ParseUtil::endsWith(text.c_str(), text.length(), "aabcd");
            assert(result == false);
        }
    }
    return 0;
}


void callAllTests() {
    testParseUtil();
}

#pragma warning(pop)