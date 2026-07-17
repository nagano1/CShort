#include <cstdio>

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



void compileLLVMExport1() {
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
    // currently, the outputText is not used, but in the future, validation and execution will be performed
    printf("outputText = %s\n", outputText);

    Alloc::deleteDocument(document);
}

void checkSemanticError(const char* str) {

}

#define CheckTextEq(x) checkTextEquality(#x, x)
void callTests()
{
    compileLLVMExport1();

    checkSemanticError(R"(fn Main() { int a = 5
        int a = 6})");

}
