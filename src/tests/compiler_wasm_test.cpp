#include <cstdio>
#include <cstring>
#include <cassert>
#include <cstdint>

#include "parser.hpp"
#include "ParseUtil.hpp"
#include "compiler.hpp"

using namespace cshort;

void callTests();

int main()
{
    printf("-------------------- Compiler Wasm tests ---------------------\n");
    fflush(stdout);

    callTests();

    printf("-------------------- passed successfully ---------------------\n");
    return 0;
}


// Helper: assert that the first four bytes equal the Wasm magic number.
static void assertWasmMagic(const uint8_t *data, size_t size)
{
    assert(size > 8 && "Wasm module must be larger than 8 bytes");
    assert(data != nullptr && "outData must not be null");
    assert(data[0] == 0x00 && "magic byte 0 mismatch");
    assert(data[1] == 0x61 && "magic byte 1 mismatch");
    assert(data[2] == 0x73 && "magic byte 2 mismatch");
    assert(data[3] == 0x6D && "magic byte 3 mismatch");
}


// Test 1: local variable assigned from a literal, then returned.
void testCompileWasm1()
{
    constexpr char source[] = R"(
fn Main()
{
    i64 a = 100
    return a
})";

    auto *document = Alloc::newDocument(DocumentType::CodeDocument);
    auto *context  = document->context;

    DocumentUtils::parseText(document, source, (int)strlen(source));
    Validator::validateScript(document);

    uint8_t *data = nullptr;
    size_t   size = CompilerForWasm::compileToBytes(document, context->memBuffer, &data);

    assertWasmMagic(data, size);
    printf("testCompileWasm1: size=%zu OK\n", size);

    Alloc::deleteDocument(document);
}


// Test 2: direct integer literal in return.
void testCompileWasm2()
{
    constexpr char source[] = R"(
fn Main()
{
    return 42
})";

    auto *document = Alloc::newDocument(DocumentType::CodeDocument);
    auto *context  = document->context;

    DocumentUtils::parseText(document, source, (int)strlen(source));
    Validator::validateScript(document);

    uint8_t *data = nullptr;
    size_t   size = CompilerForWasm::compileToBytes(document, context->memBuffer, &data);

    assertWasmMagic(data, size);
    printf("testCompileWasm2: size=%zu OK\n", size);

    Alloc::deleteDocument(document);
}


// Test 3: fallback module when there is no main function.
void testCompileWasmFallback()
{
    // Empty document has no mainFunc → should produce the fallback module.
    constexpr char source[] = "";

    auto *document = Alloc::newDocument(DocumentType::CodeDocument);
    auto *context  = document->context;

    DocumentUtils::parseText(document, source, (int)strlen(source));
    Validator::validateScript(document);

    uint8_t *data = nullptr;
    size_t   size = CompilerForWasm::compileToBytes(document, context->memBuffer, &data);

    assertWasmMagic(data, size);
    printf("testCompileWasmFallback: size=%zu OK\n", size);

    Alloc::deleteDocument(document);
}


void callTests()
{
    testCompileWasm1();
    testCompileWasm2();
    testCompileWasmFallback();
}
