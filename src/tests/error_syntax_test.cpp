#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4267) // conversion from 'size_t' to 'int', possible loss of data
#endif

#include <cstdio>

#include "code_nodes.hpp"
#include "ParseUtil.hpp"

using namespace cshort;

void callTests();

int main()
{
    printf("cshort test\n");
    fflush(stdout);

    callTests();

    return 0;
}

constexpr const char *classOnlyText = u8R"(
class {


}
)";

constexpr const char *no_brace_for_classText = u8R"(
class A 


}
)";


constexpr const char *no_brace_of_end_for_classText = u8R"(
class A {



)";

constexpr const char *no_brace_of_end_for_classText2 = u8R"(
class A {
    class B {
    }
)";

static void testSyntaxError(const char* codeText, ErrorIndex errorIndex, const char* errorCodeText)
{
    auto* document = Alloc::newDocument(DocumentType::CodeDocument);
    DocumentUtils::parseText(document, codeText,  strlen(codeText));


    auto* context = document->context;
    assert(context->syntaxErrorInfo.hasError == true);
    assert(context->syntaxErrorInfo.errorItem.errorIndex == errorIndex);

    Alloc::deleteDocument(document);
}

#define TEST_SYNTAX_ERROR(codeText, errorCode) \
    testSyntaxError(codeText, errorCode, #errorCode)
void callTests()
{
    TEST_SYNTAX_ERROR(classOnlyText, ErrorIndex::invalid_class_name);
    TEST_SYNTAX_ERROR(no_brace_for_classText, ErrorIndex::no_brace_for_class);
    TEST_SYNTAX_ERROR(no_brace_of_end_for_classText, ErrorIndex::no_brace_of_end_for_class);
    TEST_SYNTAX_ERROR(no_brace_of_end_for_classText2, ErrorIndex::no_brace_of_end_for_class);

}

#if defined(_MSC_VER)
#pragma warning(pop)
#endif