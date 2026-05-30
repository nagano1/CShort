#include <iostream>
#include <string>
#include <array>
#include <algorithm>


#include <cstdlib>
#include <cassert>
#include <cstdio>
#include <chrono>
#include <unordered_map>
#include <vector>

#include <cstdint>
#include <ctime>

#include "code_nodes.hpp"

namespace cshort {
    /**
     *  EndOfFile Struct
     * EndOfFile node is a special node that represents the end of the file. It is used to indicate the end of the code and
     * to attach some format tokens like comments/line-breaks that are after the last code node in the file.
     */
    static CodeLine *appendToLine(EndOfFileNodeStruct *self, CodeLine *currentCodeLine) {
        return  TokenVTableCall::callAppendTokenToLine(&self->eofToken, currentCodeLine);
    }

    static void copySelfText(EndOfFileNodeStruct *self, utf8byte *buf) {
        return;
    }

    static int selfTextLength(EndOfFileNodeStruct *) {
        return 0;
    }

    static const char endOfFileTypeText[] = "<EndOfFile>";


    static int applyFuncToDescendants(EndOfFileNodeStruct *Node, ApplyFunc_params3) {
        return 0;
    }



    static node_vtable _endOfDocVTable = CREATE_VTABLE(EndOfFileNodeStruct,
                                                       selfTextLength,
                                                       copySelfText,
                                                       appendToLine,
                                                       applyFuncToDescendants,
                                                       endOfFileTypeText,
                                                       NodeTypeId::EndOfDoc);

    const node_vtable *VTables::EndOfFileVTable = &_endOfDocVTable;
}