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
    printf("cshort test\n");
    fflush(stdout);

    testParsing();

    return 0;
}

constexpr const char *classOnlyText = u8R"(
class TestClass { /*comment*/
    // comment
}

    )";
constexpr const char *multipleRowsComment = u8R"(
class TestClass { /*comment

    test
*/
    // comment
}
/*<[comment]

/*<[B]
    test
[B]>*/
[comment]>*/)";

constexpr const char *namedTagCommentText = u8R"(
/*
/*<[tag1]
class TestClass {
    // comment
}
[tag1]>*/

*/)";

constexpr const char *fnTestText = u8R"(

fn functionName(bool a) {
    bool b = true
    TypeName *value // comment
    b = false/*comment*/
}

class A { // comment
    fn method1() {
        #bool immutableBool = true
        ?bool *nullableBool = false
        int a = 3214
        return true
    }
}
)";



const char classCommentText[] = u8"class A \r\n // comment \r\n {}";

void checkTextEquality(const char *name, const char* code)
{
    fprintf(stderr, "checking: %s\n", name);

    auto *document = Alloc::newDocument(DocumentType::CodeDocument);
    DocumentUtils::parseText(document, code, strlen(code));
    char *treeText = DocumentUtils::getTextFromTree(document);

    if (document->context->syntaxErrorInfo.hasError) {
        fprintf(stderr, "unexpected syntax error: %s at position %d\n", getErrorMessage(document->context->syntaxErrorInfo.errorItem.errorIndex), document->context->syntaxErrorInfo.errorItem.charPosition);
        assert(false && "unexpected syntax error");
    }
    assert(document->context->syntaxErrorInfo.hasError == false);

    if (strcmp(code, treeText) != 0) {
        fprintf(stderr, "expected:\n[%s]\n", code);
        fprintf(stderr, "actual:\n[%s]\n", treeText);
        assert(false && "text not equal");
    }

    free(treeText);
    Alloc::deleteDocument(document);
}


#define CheckTextEq(x) checkTextEquality(#x, x)
void testParsing()
{
    CheckTextEq(fnTestText);
    CheckTextEq(classOnlyText);
    CheckTextEq(classCommentText);
    CheckTextEq(multipleRowsComment);
    CheckTextEq(namedTagCommentText);
    CheckTextEq(""); // empty text
    CheckTextEq(" \r\n \n\n  ");

}

#if defined(_MSC_VER)
#pragma warning(pop)
#endif