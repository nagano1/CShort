#pragma once
//
#include <stdlib.h>
#include <array>
//
#include <cstdlib>
//#include <cassert>
#include <cstdio>
#include <chrono>
#include <unordered_map>
//
#include <cstdint> // uint64_t, int_fast32_t
#include <ctime>
//
#include <string.h> // memcpy

namespace cshort {

    enum class ErrorIndex {
        first_keeper,

        //----------------------------------------------------------------------------------
        //
        //                                  Syntax Errors
        //
        //----------------------------------------------------------------------------------

        no_syntax_error,

        // common
        syntax_error,
        syntax_error2,
        should_break_line,
        indent_error,

        // operator
        invalid_operator_for_type,

        // value
        expect_end_parenthesis,
        expected_expression_after_operator,

        // string
        missing_closing_quote,
        missing_closing_quote2,
        missing_object_delimiter,
        unexpected_line_break_or_null_in_string_literal,
        invalid_operator_for_string,

        // class
        invalid_class_name,
        no_brace_for_class,
        no_brace_of_end_for_class,

        // fn
        invalid_fn_name,
        expect_bracket_for_fn_body,
        expect_parenthesis_for_fn_params,
        expect_end_parenthesis_for_fn_params,

        // return
        no_value_for_return,

        //----------------------------------------------------------------------------------
        //
        //                                  Logical/Semantic Errors
        //
        //----------------------------------------------------------------------------------
        

        // semantic errors
        main_func_not_found,
        no_logical_error,
        no_variable_defined,
        variable_name_duplicated,
        type_not_found,
        assign_null_to_unnullable,
        assign_to_immutable,
        let_without_value,
        cant_put_immutable_mark_for_non_value_assignment,
        type_is_not_assignable,
        internal_error,

        // ---------------------------------------------------------------------------------
        //
        //                                  Runtime Errors
        //
        // ---------------------------------------------------------------------------------
        division_by_zero,


        last_keeper
    };


    static constexpr int errorListSize = 1 + static_cast<int>(ErrorIndex::last_keeper);

    struct ErrorInfo {
        ErrorIndex errorIndex; // enum value for error index, used for switch case and array indexing.
        int errorCode; // user friendly error code.
        const char* msg;

        static ErrorInfo ErrorInfoList[errorListSize];
    };

    // C++-14 
    // 201402L (C++14), 201703L (C++17), 202002L (C++20)
    #if __cplusplus >= 201402L || _MSVC_LANG >= 201402L // C++14 or later
    #define  USE_STATIC_SORT
    #endif
    #ifdef USE_STATIC_SORT
    // Validate that the error list is ordered by strictly increasing errorCode and that ErrorIndex values are continuous starting at 0.
    static constexpr bool is_sorted(const ErrorInfo tempList[])
    {
        // 1) errorCode must be strictly increasing (no duplicates).
        for (std::size_t i = 0; i < errorListSize - 1; ++i) {
            if (tempList[i].errorCode >= tempList[i + 1].errorCode) {
                return false;
            }
        }
        // check if error index is continuous without gap, and starts from 0.
        int expectedErrorIndex = 0;
        for (std::size_t i = 0; i < errorListSize; ++i) {
            if (static_cast<int>(tempList[i].errorIndex) != expectedErrorIndex) {
                return false;
            }
            expectedErrorIndex++;
        }
        return true;
    }
    #endif

    static int initErrorInfoList()
    {
        static constexpr ErrorInfo errorInfoListData[] = {
            
            ErrorInfo{ ErrorIndex::first_keeper, 9912, "start"},

            //----------------------------------------------------------------------------------
            //
            //                                     Syntax Errors
            //
            //----------------------------------------------------------------------------------
            ErrorInfo{ ErrorIndex::no_syntax_error, 10000, "no_syntax_error"},

            // common
            ErrorInfo{ ErrorIndex::syntax_error, 418030, "syntax error" },
            ErrorInfo{ ErrorIndex::syntax_error2, 418031, "syntax error2" },
            ErrorInfo{ ErrorIndex::should_break_line, 418032, "should have a line break" },
            ErrorInfo{ ErrorIndex::indent_error, 418033, "indent error" },

            // operator
            ErrorInfo{ ErrorIndex::invalid_operator_for_type, 418035, "invalid operator for type" },

            // value
            ErrorInfo{ ErrorIndex::expect_end_parenthesis, 418133, "expect_end_parenthesis" },
            ErrorInfo{ ErrorIndex::expected_expression_after_operator, 418134, "expected expression after operator" },

            // string
            ErrorInfo{ ErrorIndex::missing_closing_quote, 989800, "missing closing quote" },
            ErrorInfo{ ErrorIndex::missing_closing_quote2, 989900, "missing closing quote" },

            ErrorInfo{ ErrorIndex::missing_object_delimiter, 7677812, "missing object delimiter"},
            ErrorInfo{ ErrorIndex::unexpected_line_break_or_null_in_string_literal, 7677813, "unexpected line break or end of document  in string literal"},
            ErrorInfo{ ErrorIndex::invalid_operator_for_string, 7677814, "invalid operator for string"},

            // class
            ErrorInfo{ ErrorIndex::invalid_class_name, 7777413, "Invalid class name"},
            ErrorInfo{ ErrorIndex::no_brace_for_class, 7777414, "no brace for class"},
            ErrorInfo{ ErrorIndex::no_brace_of_end_for_class, 7777415, "no brace of end for class"},

            // fn
            ErrorInfo{ ErrorIndex::invalid_fn_name, 7777815, "invalid fn name"},
            ErrorInfo{ ErrorIndex::expect_bracket_for_fn_body, 7777816, "expect '{' for fn body"},
            ErrorInfo{ ErrorIndex::expect_parenthesis_for_fn_params, 7777817, "expect '(' for fn parameters"},
            ErrorInfo{ ErrorIndex::expect_end_parenthesis_for_fn_params, 7777818, "expect ')' for fn parameters"},

            // return
            ErrorInfo{ ErrorIndex::no_value_for_return, 7778818, "no value for return statement"},

            //----------------------------------------------------------------------------------
            //
            //                                  Logical/Semantic Errors
            //
            //----------------------------------------------------------------------------------
            ErrorInfo{ErrorIndex::main_func_not_found, 57760000, "main function not found"},
            ErrorInfo{ErrorIndex::no_logical_error,     57770000, "no_logical_error"},
            ErrorInfo{ErrorIndex::no_variable_defined,  57770001, "no variable defined"},
            ErrorInfo{ErrorIndex::variable_name_duplicated, 57770002, "variable name duplicated"},

            ErrorInfo{ErrorIndex::type_not_found,       57770003, "type not found"},
            ErrorInfo{ErrorIndex::assign_null_to_unnullable, 57770004, "assign null to unnullable type"},
            ErrorInfo{ErrorIndex::assign_to_immutable,  57770005, "assign to immutable"},
            ErrorInfo{ErrorIndex::let_without_value, 57770006, "let without value"},

            ErrorInfo{ErrorIndex::cant_put_immutable_mark_for_non_value_assignment, 57770007, "cant put immutable mark for non value assignment"},
            ErrorInfo{ErrorIndex::type_is_not_assignable, 57770008, "type is not assignable"},
            ErrorInfo{ErrorIndex::internal_error, 57770009, "internal error"},

            // ---------------------------------------------------------------------------------
            //
            //                                  Runtime Errors
            //
            // ---------------------------------------------------------------------------------
            ErrorInfo{ErrorIndex::division_by_zero, 70000000, "division by zero"},

            ErrorInfo{ ErrorIndex::last_keeper, 99999999, "end" },
        };


        static_assert(errorListSize == (sizeof errorInfoListData) / sizeof(ErrorInfo), "error list should have the same length"); 
        static_assert(0 == (int)ErrorIndex::first_keeper, "first keeper id = 0");
        static_assert(errorListSize-1 == (int)ErrorIndex::last_keeper, "last keeper id = ");

        #ifdef USE_STATIC_SORT
        static_assert(is_sorted(errorInfoListData), "error List should be sorted with error code"); // C++14
        #endif

        // initialize error info list
        for (int i = 0; i < errorListSize; i++) {
            auto &&errorInfo = errorInfoListData[i];
            // assign error info to the list to get error info by error index in O(1) time.
            ErrorInfo::ErrorInfoList[static_cast<int>(errorInfoListData[i].errorIndex)] = errorInfo;
        }

        return 0;
    }


    enum class Language {
        en = 8591000,
        jp = 8591001,
    };


    static const char *translateErrorMessage(ErrorIndex errorIndex, Language lang) {
        (void)errorIndex;
        (void)lang;
        return nullptr;
    }

    static void ensureErrorInfoInitialized() {
        static const int initResult = initErrorInfoList();
        (void)initResult;
    }

    // for error code, we use a large number to avoid conflict with other error codes.
    static int getErrorCode(ErrorIndex errorIndex) {
        ensureErrorInfoInitialized();

        auto&& errorInfo = ErrorInfo::ErrorInfoList[static_cast<int>(errorIndex)];
        return errorInfo.errorCode;
    }

    // for error message, we use the message in error info list, and we can also translate the message to other language if needed.
    static const char *getErrorMessage(ErrorIndex errorIndex) {
        ensureErrorInfoInitialized();

        const char *mes = nullptr;
        auto&& errorInfo = ErrorInfo::ErrorInfoList[static_cast<int>(errorIndex)];
        mes = errorInfo.msg;

        const char *transMess = translateErrorMessage(errorIndex, Language::en);
        if (transMess != nullptr) {
            mes = transMess;
        }

        return mes;
    }

    #define MAX_REASON_LENGTH 1024


    using CodeErrorItem = struct _CodeErrorItem {
        ErrorIndex errorIndex;
        char reason[MAX_REASON_LENGTH + 1];
        int reasonLength = 0;

        st_int charPosition;
        st_int charPosition2;
        st_int linePos1;
        st_int charPos1;
        st_int linePos2;
        st_int charPos2;

        int errorId;
        int charEndPosition;

        // 0: "between start and  end"
        // 1: "from start to end of line,"
        int errorDisplayType = 0;
    };


    /**
     * Syntax error is allowed only once
     */
    using SyntaxErrorInfo = struct _errorInfo {
        bool hasError{false};
        CodeErrorItem  errorItem;
        static const int SYNTAX_ERROR_RETURN = -1;
    };


    struct _NodeBase;
    // semantic errors with NodeBase
    using SemanticErrorItem = struct _ErrorNodeItem {
        _NodeBase *node;
        _ErrorNodeItem *next;

        CodeErrorItem codeErrorItem;
    };

    using SemanticErrorInfo = struct _semanticErrorInfo {
        bool hasError{false};
        int count;

        SemanticErrorItem *firstErrorItem;
        SemanticErrorItem *lastErrorItem;
        static const int SYNTAX_ERROR_RETURN = -1;
    };
}
