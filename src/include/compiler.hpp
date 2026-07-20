#pragma once

#include <stdlib.h>
#include "ParseUtil.hpp"
#include "common.hpp"
#include "parser.hpp"
#include "types.hpp"

namespace cshort
{
    /*
    struct ScriptRunner {
        static int64_t runScriptWithLength(const char* script, int byteLength);

        template<std::size_t SIZE>
        static int64_t runScript(const char(&text)[SIZE])
        {
            return runScriptWithLength(text, SIZE - 1);
        }
    };
    */
    struct CompilerForLLVM {
        static char *compile(DocumentStruct *document, MemBuffer &memBuffer);
    };
}