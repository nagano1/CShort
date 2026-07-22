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


// Helper: assert that 'output' contains the substring 'fragment'.
static void assertContains(const char *output, const char *fragment) {
    if (strstr(output, fragment) == nullptr) {
        fprintf(stderr, "FAILED: expected output to contain: %s\n", fragment);
        fprintf(stderr, "Actual output:\n%s\n", output);
        assert(false && "output missing expected fragment");
    }
}


// ---------------------------------------------------------------------------
//  LLVM tests
// ---------------------------------------------------------------------------

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

    char *outputText = CompilerForLLVM::compile(document, context->memBuffer);
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
    char *outputText = CompilerForLLVM::compile(document, context->memBuffer);
    printf("testCompileLLVM2 outputText =\n%s\n", outputText);

    assertContains(outputText, "define i64 @main()");
    assertContains(outputText, "ret i64 100");

    Alloc::deleteDocument(document);
}


// ---------------------------------------------------------------------------
//  Wasm (WAT) tests
// ---------------------------------------------------------------------------

// Test: i64 local assigned from integer literal, then returned.
//   fn Main() { i64 a = 100; return a }
void testCompileWasm1() {
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

    char *outputText = CompilerForWasm::compile(document, context->memBuffer);
    printf("testCompileWasm1 outputText =\n%s\n", outputText);

    assertContains(outputText, "(module");
    assertContains(outputText, "(func $main (result i64)");
    assertContains(outputText, "(local $a i64)");
    assertContains(outputText, "i32.const 100");
    assertContains(outputText, "local.set $a");
    assertContains(outputText, "local.get $a");
    assertContains(outputText, "return");
    assertContains(outputText, "(export \"main\" (func $main))");

    Alloc::deleteDocument(document);
}

// Test: direct integer literal return.
//   fn Main() { return 100 }
void testCompileWasm2() {
    constexpr char source[] = R"(
fn Main()
{
    return 100
})";

    auto *document = Alloc::newDocument(DocumentType::CodeDocument);
    auto *context = document->context;

    DocumentUtils::parseText(document, source, (int)strlen(source));
    Validator::validateScript(document);

    char *outputText = CompilerForWasm::compile(document, context->memBuffer);
    printf("testCompileWasm2 outputText =\n%s\n", outputText);

    assertContains(outputText, "(module");
    assertContains(outputText, "(func $main (result i64)");
    assertContains(outputText, "i32.const 100");
    assertContains(outputText, "return");
    assertContains(outputText, "(export \"main\" (func $main))");

    Alloc::deleteDocument(document);
}

// Test: binary expression with parentheses.
//   fn Main() { return (10 + 20) * 3 }
void testCompileWasm3() {
    constexpr char source[] = R"(
fn Main()
{
    return (10 + 20) * 3
})";

    auto *document = Alloc::newDocument(DocumentType::CodeDocument);
    auto *context = document->context;

    DocumentUtils::parseText(document, source, (int)strlen(source));
    Validator::validateScript(document);

    char *outputText = CompilerForWasm::compile(document, context->memBuffer);
    printf("testCompileWasm3 outputText =\n%s\n", outputText);

    assertContains(outputText, "(module");
    assertContains(outputText, "(func $main (result i64)");
    // The add and multiply must appear somewhere in the body.
    assertContains(outputText, ".add");
    assertContains(outputText, ".mul");
    assertContains(outputText, "return");
    assertContains(outputText, "(export \"main\" (func $main))");

    Alloc::deleteDocument(document);
}


void checkSemanticError(const char* str) {

}

#define CheckTextEq(x) checkTextEquality(#x, x)
void callTests()
{
    testCompileLLVM1();
    testCompileLLVM2();

    testCompileWasm1();
    testCompileWasm2();
    testCompileWasm3();

    /*
    checkSemanticError(R"(fn Main() { int a = 5
        int a = 6})");
    */
}
