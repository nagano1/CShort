#include <assert.h>
#include <stdio.h>

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

void testCompileWasm1() {
    constexpr char source[] = R"(
fn Main()
{
    i64 a = 100 // implicit conversion path accepted
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
    assertContains(outputText, "i64.const 100");
    assertContains(outputText, "local.set $a");
    assertContains(outputText, "local.get $a");
    assertContains(outputText, "return");
    assertContains(outputText, "(export \"main\" (func $main))");

    Alloc::deleteDocument(document);
}

void testCompileWasm2() {
    constexpr char source[] = R"(
fn Main()
{
    i32 a = 7
    return a
})";

    auto *document = Alloc::newDocument(DocumentType::CodeDocument);
    auto *context = document->context;

    DocumentUtils::parseText(document, source, (int)strlen(source));
    Validator::validateScript(document);

    char *outputText = CompilerForWasm::compile(document, context->memBuffer);
    printf("testCompileWasm2 outputText =\n%s\n", outputText);

    assertContains(outputText, "(local $a i32)");
    assertContains(outputText, "local.get $a");
    assertContains(outputText, "i64.extend_i32_s"); // widening for i64 function result
    assertContains(outputText, "return");

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

    /*
    checkSemanticError(R"(fn Main() { int a = 5
        int a = 6})");
    */
}