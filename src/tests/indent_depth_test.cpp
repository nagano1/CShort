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

const int indentDepthRuleList1[] = {0,0,0,1,2,1,2,3,1,0,0};
constexpr const char *classOnlyText = u8R"(// 0
class TestCl     // 0
{ // 0
    fn func(int a    // 1
            int b)   // 2: line break in parameter list, indent depth should be increased
    { // 1
        let aw = 242 + 23421
            - 142
    } // 1
}    // 0
// 0)";

const int indentDepthRuleList2[] = {0,0,0,0,1,1,2,2,1,2,2,1,1,2,2,2,1,0};
constexpr const char *classOnlyText2 = u8R"(// 0
class
TestCl
{
    fn est() {}
    fn func(int a, int b) {

        let a = 314}
    fn a() {
        
    
    /*fjwoi*/}
    class B { fn c() {

        }
        fn d() {}
    }
})";

const int indentDepthRuleList3[] = {0,0,0,1,2,3,3,4,3,4,2,1,0};
constexpr const char *classOnlyText3 = u8R"(
class TestCl
{
    class B { class C {
        class D {
            fn C
            (int a,
                int b)
            {

        }}
    }}
})";

const int indentDepthRuleList4[] = {0,0,0,1,1,2,2,1,2,1,0};
constexpr const char *classOnlyText4 = u8R"(
class TestCl
{
    fn Cook
    (
        int a = 3,
        int b
    ) {

    }
})";

// Only assignment, call, increment, decrement, and new object expressions can be used as a statement
const int indentDepthRuleList5[] = {0,0,1,2,3,4,2,1,0};
constexpr const char *classOnlyText5 = u8R"(
fn Cook() {
    int g = (343
        - (
            3241 + (232 * 3)
                - 34234
        )
    )
})";


void checkIndentDepth(const char *chars, const int indentDepthRuleList[], int ruleListLength)
{
    printf("----------------- CShort test ----------------\n");
    auto *document = Alloc::newDocument(DocumentType::CodeDocument);
    DocumentUtils::parseText(document, chars, strlen(chars));
    assert(document->context->syntaxErrorInfo.hasError == false);

    auto *line = document->firstCodeLine;
    int i = 0;
    while (line)
    {
        printf("%d, depth: %d\n", indentDepthRuleList[i], line->depth);
        assert(line->depth == indentDepthRuleList[i]);
        line = line->nextLine;
        i++;
    }

    assert(i == ruleListLength);

    Alloc::deleteDocument(document);
}
void testParsing()
{
    checkIndentDepth(classOnlyText, indentDepthRuleList1, sizeof(indentDepthRuleList1) / sizeof(indentDepthRuleList1[0]));
    checkIndentDepth(classOnlyText2, indentDepthRuleList2, sizeof(indentDepthRuleList2) / sizeof(indentDepthRuleList2[0]));
    checkIndentDepth(classOnlyText3, indentDepthRuleList3, sizeof(indentDepthRuleList3) / sizeof(indentDepthRuleList3[0]));
    checkIndentDepth(classOnlyText4, indentDepthRuleList4, sizeof(indentDepthRuleList4) / sizeof(indentDepthRuleList4[0]));
    checkIndentDepth(classOnlyText5, indentDepthRuleList5, sizeof(indentDepthRuleList5) / sizeof(indentDepthRuleList5[0]));

}

#if defined(_MSC_VER)
#pragma warning(pop)
#endif