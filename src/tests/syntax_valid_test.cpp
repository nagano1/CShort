#pragma warning(push)
#pragma warning(disable : 4267) // conversion from 'size_t' to 'int', possible loss of data

#include <cstdio>

#include "code_nodes.hpp"
#include "ParseUtil.hpp"

using namespace cshort;

void testParsing();

int main()
{
    printf("cshort");
    fflush(stdout);

    testParsing();

    return 0;
}

constexpr auto *classOnlyText = const_cast<char *>(u8R"(
class TestClass { /*comment*/
    // comment
}

    )");
constexpr auto *multipleRowsComment = const_cast<char *>(u8R"(
class TestClass { /*comment

    test
*/
    // comment
}
/*comment

*/)");




const char classCommentText[] = u8"class A \r\n // comment \r\n {}";

void checkTextEquality(const char *name, const char* code)
{
    fprintf(stderr, "checking: %s\n", name);

    auto *document = Alloc::newDocument(DocumentType::CodeDocument);
    DocumentUtils::parseText(document, code, strlen(code));
    char *treeText = DocumentUtils::getTextFromTree(document);

    assert(document->context->syntaxErrorInfo.hasError == false);

    if (strcmp(code, treeText) == 0) {

    }
    else {
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
    CheckTextEq(classOnlyText);
    CheckTextEq(classCommentText);
    //CheckTextEq(multipleRowsComment);
    CheckTextEq(""); // empty text
    CheckTextEq(" \r\n \n\n  ");

}

#pragma warning(pop)