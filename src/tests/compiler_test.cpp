#include <cstdio>
#include <cstring>

#include "parser.hpp"
#include "ParseUtil.hpp"
#include "compiler.hpp"

using namespace cshort;

void callTests();

int main()
{
    printf("-------------------- Compiler tests ---------------------\n");
    fflush(stdout);

    callTests();

    printf("-------------------- passed successfully ---------------------\n");
    return 0;
}


// Helper: assert that 'ir' contains the substring 'fragment'.
static void assertContains(const char *ir, const char *fragment) {
    if (strstr(ir, fragment) == nullptr) {
        fprintf(stderr, "FAILED: expected IR to contain: %s\n", fragment);
        fprintf(stderr, "Actual IR:\n%s\n", ir);
        assert(false && "IR missing expected fragment");
    }
}


void testCompileLLVM1() {
            constexpr char source[] = R"(
fn Main()
{
    i64 a = 100 // implicit conversion to i64
    return a
})";

    auto *document = Alloc::newDocument(DocumentType::CodeDocument);
    auto *context = document->context;

    DocumentUtils::parseText(document, source, (int)strlen(source));

    Validator::validateScript(document);

    char *outputText = CompilerForLLVM::compile(document, context);
    printf("testCompileLLVM1 outputText =\n%s\n", outputText);

    assertContains(outputText, "define i64 @main()");
    assertContains(outputText, "alloca i64");
    assertContains(outputText, "store i64 100");
    assertContains(outputText, "ret i64");

    Alloc::deleteDocument(document);
}


void testCompileLLVM2() {
    constexpr char source[] = R"(
fn Main()
{
    return 100
})";

    auto *document = Alloc::newDocument(DocumentType::CodeDocument);
    auto *context = document->context;

    DocumentUtils::parseText(document, source, (int)strlen(source));

    Validator::validateScript(document);

    char *outputText = CompilerForLLVM::compile(document, context);
    printf("testCompileLLVM2 outputText =\n%s\n", outputText);

    assertContains(outputText, "define i64 @main()");
    assertContains(outputText, "ret i64 100");

    Alloc::deleteDocument(document);
}


void checkSemanticError(const char* str) {

}

#define CheckTextEq(x) checkTextEquality(#x, x)
void callTests()
{
    testCompileLLVM1();
    testCompileLLVM2();

    /*
    checkSemanticError(R"(fn Main() { int a = 5
        int a = 6})");
    */
}
