#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4267) // conversion from 'size_t' to 'int', possible loss of data
#endif

#include <cstdio>

#include "code_nodes.hpp"
#include "ParseUtil.hpp"

using namespace cshort;

void testParsing();

int main()
{
    printf("----------------- CShort test ----------------\n");
    fflush(stdout);

    testParsing();

    return 0;
}

const int indentDepthRuleList[] = {0,0,0,1,2,1,2,1,0,0};
constexpr const char *classOnlyText = u8R"(// 0
class TestCl     // 0
{ // 0
    fn func(int a    // 1
            int b)   // 2: line break in parameter list, indent depth should be increased
    { // 1
        let aw = (242 + 23421
            - 142)
    } // 1
}    // 0
// 0)";

void testParsing()
{

    const char* chars = classOnlyText;
    auto* document = Alloc::newDocument(DocumentType::CodeDocument);
    DocumentUtils::parseText(document, chars, strlen(classOnlyText));
    assert(document->context->syntaxErrorInfo.hasError == false);

    auto* line = document->firstCodeLine;
    int i = 0;
    while (line) {
        printf("%d, depth: %d\n", indentDepthRuleList[i], line->depth);
        assert(line->depth == indentDepthRuleList[i]);
        line = line->nextLine;
        i++;
    }

    assert( i == sizeof(indentDepthRuleList) / sizeof(indentDepthRuleList[0]));

    Alloc::deleteDocument(document);

}

#if defined(_MSC_VER)
#pragma warning(pop)
#endif