#include <cstdio>
#include <cstring>
#include <cassert>

#include "parser.hpp"
#include "ParseUtil.hpp"
#include "compiler.hpp"

using namespace cshort;

void callTests();

int main()
{
    printf("-------------------- Compiler MSIL tests ---------------------\n");
    fflush(stdout);

    callTests();

    printf("-------------------- passed successfully ---------------------\n");
    return 0;
}


// Helper: assert that 'il' contains the substring 'fragment'.
static void assertContains(const char *il, const char *fragment) {
    if (strstr(il, fragment) == nullptr) {
        fprintf(stderr, "FAILED: expected IL to contain: %s\n", fragment);
        fprintf(stderr, "Actual IL:\n%s\n", il);
        assert(false && "IL missing expected fragment");
    }
}


// Test 1: local variable assigned from a literal, then returned.
void testCompileMSIL1() {
    constexpr char source[] = R"(
fn Main()
{
    i64 a = 100
    return a
})";

    auto *document = Alloc::newDocument(DocumentType::CodeDocument);
    auto *context = document->context;

    DocumentUtils::parseText(document, source, (int)strlen(source));
    Validator::validateScript(document);

    char *outputText = CompilerForMSIL::compile(document, context->memBuffer);
    printf("testCompileMSIL1 outputText =\n%s\n", outputText);

    assertContains(outputText, ".entrypoint");
    assertContains(outputText, "int64");
    assertContains(outputText, "ldc.i8 100");
    assertContains(outputText, "stloc");
    assertContains(outputText, "ldloc");
    assertContains(outputText, "ret");

    Alloc::deleteDocument(document);
}


// Test 2: direct integer literal in return.
void testCompileMSIL2() {
    constexpr char source[] = R"(
fn Main()
{
    return 100
})";

    auto *document = Alloc::newDocument(DocumentType::CodeDocument);
    auto *context = document->context;

    DocumentUtils::parseText(document, source, (int)strlen(source));
    Validator::validateScript(document);

    char *outputText = CompilerForMSIL::compile(document, context->memBuffer);
    printf("testCompileMSIL2 outputText =\n%s\n", outputText);

    assertContains(outputText, ".entrypoint");
    assertContains(outputText, "ldc.i8 100");
    assertContains(outputText, "ret");

    Alloc::deleteDocument(document);
}


// Test 3: fallback module when there is no main function.
void testCompileMSILFallback() {
    constexpr char source[] = "";

    auto *document = Alloc::newDocument(DocumentType::CodeDocument);
    auto *context = document->context;

    DocumentUtils::parseText(document, source, (int)strlen(source));
    Validator::validateScript(document);

    char *outputText = CompilerForMSIL::compile(document, context->memBuffer);
    printf("testCompileMSILFallback outputText =\n%s\n", outputText);

    assertContains(outputText, ".entrypoint");
    assertContains(outputText, "ret");

    Alloc::deleteDocument(document);
}


// Test 4: i32 local variable with widening to i64.
void testCompileMSIL_i32() {
    constexpr char source[] = R"(
fn Main()
{
    i32 b = 42
    return b
})";

    auto *document = Alloc::newDocument(DocumentType::CodeDocument);
    auto *context = document->context;

    DocumentUtils::parseText(document, source, (int)strlen(source));
    Validator::validateScript(document);

    char *outputText = CompilerForMSIL::compile(document, context->memBuffer);
    printf("testCompileMSIL_i32 outputText =\n%s\n", outputText);

    assertContains(outputText, "int32");
    assertContains(outputText, "ldc.i4 42");
    assertContains(outputText, "conv.i8");
    assertContains(outputText, "ret");

    Alloc::deleteDocument(document);
}


void callTests()
{
    testCompileMSIL1();
    testCompileMSIL2();
    testCompileMSILFallback();
    testCompileMSIL_i32();
}
