#include <cstdio>
#include <iostream>
#include <string>
#include <array>
#include <algorithm>
#include <cinttypes>

#include <cstdlib>
#include <cassert>
#include <cstdio>
#include <chrono>
#include <unordered_map>
#include <vector>

#include <cstdint>
#include <ctime>
#include <cstdint>

#include "compiler.hpp"

namespace cshort {

    // ---------------------------------------------------------------------------
    //
    //                            Compiler using LLVM
    //
    // ---------------------------------------------------------------------------
    
    char *CompilerForLLVM::compile(DocumentStruct *document, ParseContext *context) {
        int outputTextLength = 500;
        utf8byte *text = context->newText(outputTextLength);
        text[0] = 'a';
        return text;
    }
}