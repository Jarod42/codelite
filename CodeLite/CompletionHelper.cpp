#include "CompletionHelper.hpp"

#include "Cxx/CxxScannerTokens.h"
#include "Cxx/CxxTokenizer.h"
#include "file_logger.h"

#include <vector>
#include <wx/regex.h>

namespace
{
#define PREPEND_STRING(tokn) expression.insert(expression.begin(), tokn)
#define MAX_TIP_LINE_SIZE 200

bool is_word_char(wxChar ch)
{
    return (ch >= 65 && ch <= 90)     // uppercase A-Z
           || (ch >= 97 && ch <= 122) // lowercase a-z
           || (ch >= 48 && ch <= 57)  // 0-9
           || (ch == '_');
}

wxString wrap_lines(const wxString& str)
{
    wxString wrappedString;

    int curLineBytes(0);
    for (const auto c : str) {
        if (c == '\t') {
            wrappedString << " ";

        } else if (c == '\n') {
            wrappedString << "\n";
            curLineBytes = 0;

        } else if (c == '\r') {
            // Skip it

        } else {
            wrappedString << c;
        }
        curLineBytes++;

        if(curLineBytes == MAX_TIP_LINE_SIZE) {

            // Wrap the lines
            if(wrappedString.IsEmpty() == false && wrappedString.Last() != '\n') {
                wrappedString << "\n";
            }
            curLineBytes = 0;
        }
    }
    return wrappedString;
}

} // namespace

wxString CompletionHelper::get_expression(const wxString& file_content, bool for_calltip, wxString* last_word) const
{
#define LAST_TOKEN_IS(token_type) (!types.empty() && (types[types.size() - 1] == token_type))
    // tokenize the text
    CxxTokenizer tokenizer;
    tokenizer.Reset(file_content);

    CxxLexerToken token;
    std::vector<std::pair<wxString, int>> tokens;
    tokens.reserve(10000);

    while(tokenizer.NextToken(token)) {
        tokens.push_back({ token.GetWXString(), token.GetType() });
    }

    int i = static_cast<int>(tokens.size() - 1);
    bool cont = true;
    int parentheses_depth = 0;
    if(for_calltip) {
        // read backwards until we find the first open parentheses
        for(; (i >= 0) && cont; --i) {
            switch(tokens[i].second) {
            case '(':
                if(parentheses_depth == 0) {
                    cont = false;
                } else {
                    parentheses_depth--;
                }
                break;
            case ')':
                parentheses_depth++;
                break;
            default:
                break;
            }
        }
    }

    std::vector<wxString> expression;
    std::vector<int> types;
    int depth = 0;
    cont = true;
    for(; (i >= 0) && cont; --i) {
        const wxString& text = tokens[i].first;
        int type = tokens[i].second;
        switch(type) {
        case '[':
        case '{':
        case '(':
        case '<':
            if(depth == 0) {
                cont = false;
            } else {
                PREPEND_STRING(text);
                depth--;
            }
            break;
        case '>':
        case ']':
        case ')':
            if(depth == 0 && LAST_TOKEN_IS(T_IDENTIFIER)) {
                cont = false;
            } else {
                PREPEND_STRING(text);
                depth++;
            }
            break;
        case '}':
            if(depth == 0) {
                cont = false;
            } else {
                PREPEND_STRING(text);
                depth++;
            }
            break;
        case ';':
        case T_PP_DEFINE:
        case T_PP_DEFINED:
        case T_PP_IF:
        case T_PP_IFNDEF:
        case T_PP_IFDEF:
        case T_PP_ELSE:
        case T_PP_ELIF:
        case T_PP_LINE:
        case T_PP_PRAGMA:
        case T_PP_UNDEF:
        case T_PP_ERROR:
        case T_PP_ENDIF:
        case T_PP_IDENTIFIER:
        case T_PP_DEC_NUMBER:
        case T_PP_OCTAL_NUMBER:
        case T_PP_HEX_NUMBER:
        case T_PP_FLOAT_NUMBER:
        case T_PP_STRING:
        case T_PP_AND:
        case T_PP_OR:
        case T_PP_STATE_EXIT:
        case T_PP_INCLUDE_FILENAME:
        case T_PP_INCLUDE:
        case T_PP_GT:
        case T_PP_GTEQ:
        case T_PP_LT:
        case T_PP_LTEQ:
            cont = false;
            break;
        case '=':
            if(depth == 0) {
                cont = false;
            } else {
                PREPEND_STRING(text);
            }
            break;
        case T_IDENTIFIER:
            if(LAST_TOKEN_IS(T_IDENTIFIER)) {
                // we do not allow two consecutive T_IDENTIFIER
                cont = false;
                break;
            } else {
                PREPEND_STRING(text);
            }
            break;
        case T_ALIGNAS:
        case T_ALIGNOF:
        case T_AND:
        case T_AND_EQ:
        case T_ASM:
        case T_AUTO:
        case T_BITAND:
        case T_BITOR:
        case T_BOOL:
        case T_BREAK:
        case T_CATCH:
        case T_CHAR:
        case T_CHAR16_T:
        case T_CHAR32_T:
        case T_CLASS:
        case T_COMPL:
        case T_CONST:
        case T_CONSTEXPR:
        case T_CONTINUE:
        case T_DECLTYPE:
        case T_DEFAULT:
        case T_DELETE:
        case T_DO:
        case T_DOUBLE:
        case T_ELSE:
        case T_ENUM:
        case T_EXPLICIT:
        case T_EXPORT:
        case T_EXTERN:
        case T_FALSE:
        case T_FINAL:
        case T_FLOAT:
        case T_FOR:
        case T_FRIEND:
        case T_GOTO:
        case T_IF:
        case T_INLINE:
        case T_INT:
        case T_LONG:
        case T_MUTABLE:
        case T_NAMESPACE:
        case T_NEW:
        case T_NOEXCEPT:
        case T_NOT:
        case T_NOT_EQ:
        case T_NULLPTR:
        case T_OPERATOR:
        case T_OR:
        case T_OR_EQ:
        case T_OVERRIDE:
        case T_PRIVATE:
        case T_PROTECTED:
        case T_PUBLIC:
        case T_REGISTER:
        case T_CASE:
        case T_SHORT:
        case T_SIGNED:
        case T_SIZEOF:
        case T_STATIC:
        case T_STATIC_ASSERT:
        case T_STRUCT:
        case T_SWITCH:
        case T_TEMPLATE:
        case T_THREAD_LOCAL:
        case T_THROW:
        case T_TRUE:
        case T_TRY:
        case T_TYPEDEF:
        case T_TYPEID:
        case T_TYPENAME:
        case T_UNION:
        case T_UNSIGNED:
        case T_USING:
        case T_VIRTUAL:
        case T_VOID:
        case T_VOLATILE:
        case T_WCHAR_T:
        case T_WHILE:
        case T_XOR:
        case T_XOR_EQ:
        case T_STRING:
        case T_DOT_STAR:
        case T_ARROW_STAR:
        case T_PLUS_PLUS:
        case T_MINUS_MINUS:
        case T_LS:
        case T_LE:
        case T_GE:
        case T_EQUAL:
        case T_NOT_EQUAL:
        case T_AND_AND:
        case T_OR_OR:
        case T_STAR_EQUAL:
        case T_SLASH_EQUAL:
        case T_DIV_EQUAL:
        case T_PLUS_EQUAL:
        case T_MINUS_EQUAL:
        case T_LS_ASSIGN:
        case T_RS_ASSIGN:
        case T_AND_EQUAL:
        case T_POW_EQUAL:
        case T_OR_EQUAL:
        case T_3_DOTS:
        case T_RETURN:
        case ',':
        case '*':
        case '&':
        case '!':
        case '~':
        case '+':
        case '^':
        case '|':
        case '%':
        case '?':
        case '/':
        case ':':
        case '-':
        case '@':
            if(depth <= 0) {
                cont = false;
            } else {
                PREPEND_STRING(text);
            }
            break;
        case T_CONST_CAST:
        case T_DYNAMIC_CAST:
        case T_REINTERPRET_CAST:
        case T_STATIC_CAST:
        default:
            PREPEND_STRING(text);
            break;
        }
        types.push_back(type);
    }

    // build the expression from the vector
    wxString expression_string;
    for(const auto& t : expression) {
        expression_string << t;
    }
    if(last_word && !expression.empty()) {
        *last_word = expression[expression.size() - 1];
    }
    return expression_string;
#undef LAST_TOKEN_IS
}

wxString CompletionHelper::truncate_file_to_location(const wxString& file_content, size_t line, size_t column,
                                                     size_t flags) const
{
    size_t curline = 0;
    size_t offset = 0;

    // locate the line
    for(const auto& ch : file_content) {
        if(curline == line) {
            break;
        }
        switch(ch.GetValue()) {
        case '\n':
            ++curline;
            ++offset;
            break;
        default:
            ++offset;
            break;
        }
    }

    if(curline != line) {
        return wxEmptyString;
    }

    // columns
    offset += column;

    if(offset < file_content.size()) {
        if(flags & (TRUNCATE_COMPLETE_WORDS | TRUNCATE_COMPLETE_LINES)) {
            while(true) {
                size_t next_pos = offset;

                if(next_pos < file_content.size()) {
                    wxChar next_char = file_content[next_pos];
                    if(flags & TRUNCATE_COMPLETE_WORDS) {
                        // complete words only
                        if(is_word_char(next_char)) {
                            offset += 1;
                        } else {
                            break;
                        }

                    } else {
                        // TRUNCATE_COMPLETE_LINES
                        if(next_char == '\n') {
                            break;
                        } else {
                            offset += 1;
                        }
                    }
                } else {
                    break;
                }
            }
        }
        return file_content.Mid(0, offset);
    }
    return wxEmptyString;
}

thread_local const std::unordered_set<wxString> cppKeywords{
    // Regular keywords
    "alignas", // C++11
    "alignof", // C++11
    "and",     // alternative for &&
    "and_eq",  // alternative for &=
    "asm",
    "auto",
    "bitand", // alternative for &
    "bitor",  // alternative for |
    "bool",
    "break",
    "case",
    "catch",
    "char",
    "char8_t",  // C++20
    "char16_t", // C++11
    "char32_t", // C++11
    "class",
    "compl",   // alternative for ~
    "concept", // C++20
    "const",
    "consteval", // C++20
    "constexpr", // C++11
    "constinit", // C++20
    "const_cast",
    "continue",
    "contract_assert", // C++26
    "co_await",        // C++20
    "co_return",       // C++20
    "co_yield",        // C++20
    "decltype",        // C++11
    "default",
    "delete",
    "do",
    "double",
    "dynamic_cast",
    "else",
    "enum",
    "explicit",
    "export",
    "extern",
    "false",
    "float",
    "for",
    "friend",
    "goto",
    "if",
    "inline",
    "int",
    "long",
    "mutable",
    "namespace",
    "new",
    "noexcept", // C++11
    "not",      // alternative for !
    "not_eq",   // alternative for !=
    "nullptr",  // C++11
    "operator",
    "or",    // alternative for ||
    "or_eq", // alternative for |=
    "private",
    "protected",
    "public",
    "register",
    "reinterpret_cast",
    "requires", // C++20
    "return",
    "short",
    "signed",
    "sizeof",
    "static",
    "static_assert", // C++11
    "static_cast",
    "struct",
    "switch",
    "template",
    "this",
    "thread_local", // C++11
    "throw",
    "true",
    "try",
    "typedef",
    "typeid",
    "typename",
    "union",
    "unsigned",
    "using",
    "virtual",
    "void",
    "volatile",
    "wchar_t",
    "while",
    "xor",    // alternative for ^
    "xor_eq", // alternative for ^=

    // Identifier with special meaning
    "final",                             // C++11
    "override",                          // C++11
    "import",                            // C++20
    "module",                            // C++20
    "pre",                               // C++26
    "post",                              // C++26
    "trivially_relocatable_if_eligible", // C++26
    "replaceable_if_eligible",           // C++26

    // recognized tokens by the preprocessor when in context of a preprocessor directive
    "if",
    "elif",
    "else",
    "endif",

    "ifdef",
    "ifndef",
    "elifdef",  // C++23
    "elifndef", // C++23
    "define",
    "undef",

    "include",
    "embed", // C++26
    "line",

    "error",
    "warning", // C++23
    "pragma",

    "defined",
    "__has_include",       // C++17
    "__has_cpp_attribute", // C++20
    "__has_embed",         // C++26

    "export", // (C++20)
    "import", // (C++20)
    "module", // (C++20)

    // recognized tokens by the preprocessor outside the context of a preprocessor directive
    "_Pragma", // C++ 11

    // Standard attributes

    "noreturn",           // C++11
    "carries_dependency", // C++11, removed in C++26
    "deprecated",         // C++14
    "fallthrough",        // C++17
    "maybe_unused",       // C++17
    "nodiscard",          // C++17
    "likely",             // C++20
    "unlikely",           // C++20
    "no_unique_address",  // C++20
    "assume",             // C++23
    "indeterminate",      // C++26

    // predefined MACRO

    "__cplusplus",
    "__STDC_HOSTED__", // C++ 11)
    "__FILE__",
    "__LINE__",
    "__DATE__",
    "__TIME__",
    "__STDCPP_DEFAULT_NEW_ALIGNMENT__", // C++ 17
    "__STDCPP_­BFLOAT16_­T__",          // C++23
    "__STDCPP_­FLOAT16_­T__",           // C++23
    "__STDCPP_FLOAT32_T__",             // C++23
    "__STDCPP_FLOAT64_T__",             // C++23
    "__STDCPP_FLOAT128_T__",            // C++23
    "__STDC_EMBED_NOT_FOUND__",         // C++26
    "__STDC_EMBED_FOUND__",             // C++26
    "__STDC_EMBED_EMPTY__",             // C++26

    // additional macro names may be predefined by the implementations:

    "__STDC__",
    "__STDC_VERSION__",                 // C++11
    "__STDC_ISO_10646__",               // C++11
    "__STDC_MB_MIGHT_NEQ_WC__",         // C++11
    "__STDCPP_THREADS__",               // C++11
    "__STDCPP_STRICT_POINTER_SAFETY__", // C++11, removed in C++23

    // The function-local predefined variable __func__ is not a predefined macro
    "__func__",

    // Feature testing C++20

    "__has_cpp_attribute",

    // Language features

"__cpp_aggregate_bases",                 // C++17
    "__cpp_aggregate_nsdmi",                 // C++14
    "__cpp_aggregate_paren_init",            // C++20
    "__cpp_alias_templates",                 // C++11
    "__cpp_aligned_new",                     // C++17
    "__cpp_attributes",                      // C++11
    "__cpp_auto_cast",                       // C++23
    "__cpp_binary_literals",                 // C++14
    "__cpp_capture_star_this",               // C++17
    "__cpp_char8_t",                         // C++20
    "__cpp_concepts",                        // C++20
    "__cpp_conditional_explicit",            // C++20
    "__cpp_consteval",                       // C++20
    "__cpp_constexpr",                       // C++11
    "__cpp_constexpr_dynamic_alloc",         // C++20
    "__cpp_constexpr_exceptions",            // C++26
    "__cpp_constexpr_in_decltype",           // C++20
    "__cpp_constinit",                       // C++20
    "__cpp_contracts",                       // C++26
    "__cpp_decltype",                        // C++11
    "__cpp_decltype_auto",                   // C++14
    "__cpp_deduction_guides",                // C++17
    "__cpp_delegating_constructors",         // C++11
    "__cpp_deleted_function",                // C++26
    "__cpp_designated_initializers",         // C++20
    "__cpp_enumerator_attributes",           // C++17
    "__cpp_explicit_this_parameter",         // C++23
    "__cpp_fold_expressions",                // C++17
    "__cpp_generic_lambdas",                 // C++14
    "__cpp_guaranteed_copy_elision",         // C++17
    "__cpp_hex_float",                       // C++17
    "__cpp_if_consteval",                    // C++23
    "__cpp_if_constexpr",                    // C++17
    "__cpp_impl_coroutine",                  // C++20
    "__cpp_impl_destroying_delete",          // C++20
    "__cpp_impl_three_way_comparison",       // C++20
    "__cpp_implicit_move",                   // C++23
    "__cpp_inheriting_constructors",         // C++11
    "__cpp_init_captures",                   // C++14
    "__cpp_initializer_lists",               // C++11
    "__cpp_inline_variables",                // C++17
    "__cpp_lambdas",                         // C++11
    "__cpp_modules",                         // C++20
    "__cpp_multidimensional_subscript",      // C++23
    "__cpp_named_character_escapes",         // C++23
    "__cpp_namespace_attributes",            // C++17
    "__cpp_noexcept_function_type",          // C++17
    "__cpp_nontype_template_args",           // C++17
    "__cpp_nontype_template_parameter_auto", // C++17
    "__cpp_nsdmi",                           // C++11
    "__cpp_pack_indexing",                   // C++26
    "__cpp_placeholder_variables",           // C++26
    "__cpp_pp_embed",                        // C++26
    "__cpp_range_based_for",                 // C++11
    "__cpp_raw_strings",                     // C++11
    "__cpp_ref_qualifiers",                  // C++11
    "__cpp_return_type_deduction",           // C++14
    "__cpp_rvalue_references",               // C++11
    "__cpp_size_t_suffix",                   // C++23
    "__cpp_sized_deallocation",              // C++14
    "__cpp_static_assert",                   // C++11
    "__cpp_static_call_operator",            // C++23
    "__cpp_structured_bindings",             // C++17
    "__cpp_template_parameters",             // C++26
    "__cpp_template_template_args",          // C++17
    "__cpp_threadsafe_static_init",          // C++11
    "__cpp_trivial_relocatability",          // C++26
    "__cpp_trivial_union",                   // C++26
    "__cpp_unicode_characters",              // C++11
    "__cpp_unicode_literals",                // C++11
    "__cpp_user_defined_literals",           // C++11
    "__cpp_using_enum",                      // C++20
    "__cpp_variable_templates",              // C++14
    "__cpp_variadic_friend",                 // C++26
    "__cpp_variadic_templates",              // C++11
    "__cpp_variadic_using",                  // C++17

    // Library features
    "__cpp_lib_adaptor_iterator_pair_constructor",   // C++23
    "__cpp_lib_addressof_constexpr",                 // C++17
    "__cpp_lib_algorithm_default_value_type",        // C++26
    "__cpp_lib_algorithm_iterator_requirements",     // C++23
    "__cpp_lib_aligned_accessor",                    // C++26
    "__cpp_lib_allocate_at_least",                   // C++23
    "__cpp_lib_allocator_traits_is_always_equal",    // C++17
    "__cpp_lib_any",                                 // C++17
    "__cpp_lib_apply",                               // C++17
    "__cpp_lib_array_constexpr",                     // C++17
    "__cpp_lib_as_const",                            // C++17
    "__cpp_lib_associative_heterogeneous_erasure",   // C++23
    "__cpp_lib_associative_heterogeneous_insertion", // C++26
    "__cpp_lib_assume_aligned",                      // C++20
    "__cpp_lib_atomic_flag_test",                    // C++20
    "__cpp_lib_atomic_float",                        // C++20
    "__cpp_lib_atomic_is_always_lock_free",          // C++17
    "__cpp_lib_atomic_lock_free_type_aliases",       // C++20
    "__cpp_lib_atomic_min_max",                      // C++26
    "__cpp_lib_atomic_ref",                          // C++20
    "__cpp_lib_atomic_shared_ptr",                   // C++20
    "__cpp_lib_atomic_value_initialization",         // C++20
    "__cpp_lib_atomic_wait",                         // C++20
    "__cpp_lib_barrier",                             // C++20
    "__cpp_lib_bind_back",                           // C++23
    "__cpp_lib_bind_front",                          // C++20
    "__cpp_lib_bit_cast",                            // C++20
    "__cpp_lib_bitops",                              // C++20
    "__cpp_lib_bitset",                              // C++26
    "__cpp_lib_bool_constant",                       // C++17
    "__cpp_lib_bounded_array_traits",                // C++20
    "__cpp_lib_boyer_moore_searcher",                // C++17
    "__cpp_lib_byte",                                // C++17
    "__cpp_lib_byteswap",                            // C++23
    "__cpp_lib_char8_t",                             // C++20
    "__cpp_lib_chrono",                              // C++17
    "__cpp_lib_chrono_udls",                         // C++14
    "__cpp_lib_clamp",                               // C++17
    "__cpp_lib_common_reference",                    // C++23
    "__cpp_lib_common_reference_wrapper",            // C++23
    "__cpp_lib_complex_udls",                        // C++14
    "__cpp_lib_concepts",                            // C++20
    "__cpp_lib_constexpr_algorithms",                // C++20
    "__cpp_lib_constexpr_atomic",                    // C++26
    "__cpp_lib_constexpr_bitset",                    // C++23
    "__cpp_lib_constexpr_charconv",                  // C++23
    "__cpp_lib_constexpr_cmath",                     // C++23
    "__cpp_lib_constexpr_complex",                   // C++20
    "__cpp_lib_constexpr_deque",                     // C++26
    "__cpp_lib_constexpr_dynamic_alloc",             // C++20
    "__cpp_lib_constexpr_exceptions",                // C++26
    "__cpp_lib_constexpr_flat_map",                  // C++26
    "__cpp_lib_constexpr_flat_set",                  // C++26
    "__cpp_lib_constexpr_forward_list",              // C++26
    "__cpp_lib_constexpr_functional",                // C++20
    "__cpp_lib_constexpr_inplace_vector",            // C++26
    "__cpp_lib_constexpr_iterator",                  // C++20
    "__cpp_lib_constexpr_list",                      // C++26
    "__cpp_lib_constexpr_map",                       // C++26
    "__cpp_lib_constexpr_memory",                    // C++20
    "__cpp_lib_constexpr_new",                       // C++26
    "__cpp_lib_constexpr_numeric",                   // C++20
    "__cpp_lib_constexpr_queue",                     // C++26
    "__cpp_lib_constexpr_set",                       // C++26
    "__cpp_lib_constexpr_stack",                     // C++26
    "__cpp_lib_constexpr_string",                    // C++17
    "__cpp_lib_constexpr_string_view",               // C++20
    "__cpp_lib_constexpr_tuple",                     // C++20
    "__cpp_lib_constexpr_typeinfo",                  // C++23
    "__cpp_lib_constexpr_unordered_map",             // C++26
    "__cpp_lib_constexpr_unordered_set",             // C++26
    "__cpp_lib_constexpr_utility",                   // C++20
    "__cpp_lib_constexpr_vector",                    // C++20
    "__cpp_lib_constrained_equality",                // C++26
    "__cpp_lib_containers_ranges",                   // C++23
    "__cpp_lib_contracts",                           // C++26
    "__cpp_lib_copyable_function",                   // C++26
    "__cpp_lib_coroutine",                           // C++20
    "__cpp_lib_debugging",                           // C++26
    "__cpp_lib_destroying_delete",                   // C++20
    "__cpp_lib_enable_shared_from_this",             // C++17
    "__cpp_lib_endian",                              // C++20
    "__cpp_lib_erase_if",                            // C++20
    "__cpp_lib_exchange_function",                   // C++14
    "__cpp_lib_execution",                           // C++17
    "__cpp_lib_expected",                            // C++23
    "__cpp_lib_filesystem",                          // C++17
    "__cpp_lib_flat_map",                            // C++23
    "__cpp_lib_flat_set",                            // C++23
    "__cpp_lib_format",                              // C++20
    "__cpp_lib_format_path",                         // C++26
    "__cpp_lib_format_ranges",                       // C++23
    "__cpp_lib_format_uchar",                        // C++26
    "__cpp_lib_formatters",                          // C++23
    "__cpp_lib_forward_like",                        // C++23
    "__cpp_lib_freestanding_algorithm",              // C++26
    "__cpp_lib_freestanding_array",                  // C++26
    "__cpp_lib_freestanding_char_traits",            // C++26
    "__cpp_lib_freestanding_charconv",               // C++26
    "__cpp_lib_freestanding_cstdlib",                // C++26
    "__cpp_lib_freestanding_cstring",                // C++26
    "__cpp_lib_freestanding_cwchar",                 // C++26
    "__cpp_lib_freestanding_errc",                   // C++26
    "__cpp_lib_freestanding_execution",              // C++26
    "__cpp_lib_freestanding_expected",               // C++26
    "__cpp_lib_freestanding_feature_test_macros",    // C++26
    "__cpp_lib_freestanding_functional",             // C++26
    "__cpp_lib_freestanding_iterator",               // C++26
    "__cpp_lib_freestanding_mdspan",                 // C++26
    "__cpp_lib_freestanding_memory",                 // C++26
    "__cpp_lib_freestanding_numeric",                // C++26
    "__cpp_lib_freestanding_operator_new",           // C++26
    "__cpp_lib_freestanding_optional",               // C++26
    "__cpp_lib_freestanding_random",                 // C++26
    "__cpp_lib_freestanding_ranges",                 // C++26
    "__cpp_lib_freestanding_ratio",                  // C++26
    "__cpp_lib_freestanding_string_view",            // C++26
    "__cpp_lib_freestanding_tuple",                  // C++26
    "__cpp_lib_freestanding_utility",                // C++26
    "__cpp_lib_freestanding_variant",                // C++26
    "__cpp_lib_fstream_native_handle",               // C++26
    "__cpp_lib_function_ref",                        // C++26
    "__cpp_lib_gcd_lcm",                             // C++17
    "__cpp_lib_generator",                           // C++23
    "__cpp_lib_generic_associative_lookup",          // C++14
    "__cpp_lib_generic_unordered_lookup",            // C++20
    "__cpp_lib_hardened_array",                      // C++26
    "__cpp_lib_hardened_basic_string",               // C++26
    "__cpp_lib_hardened_basic_string_view",          // C++26
    "__cpp_lib_hardened_bitset",                     // C++26
    "__cpp_lib_hardened_deque",                      // C++26
    "__cpp_lib_hardened_expected",                   // C++26
    "__cpp_lib_hardened_forward_list",               // C++26
    "__cpp_lib_hardened_inplace_vector",             // C++26
    "__cpp_lib_hardened_list",                       // C++26
    "__cpp_lib_hardened_mdspan",                     // C++26
    "__cpp_lib_hardened_optional",                   // C++26
    "__cpp_lib_hardened_span",                       // C++26
    "__cpp_lib_hardened_valarray",                   // C++26
    "__cpp_lib_hardened_vector",                     // C++26
    "__cpp_lib_hardware_interference_size",          // C++17
    "__cpp_lib_has_unique_object_representations",   // C++17
    "__cpp_lib_hazard_pointer",                      // C++26
    "__cpp_lib_hive",                                // C++26
    "__cpp_lib_hypot",                               // C++17
    "__cpp_lib_incomplete_container_elements",       // C++17
    "__cpp_lib_indirect",                            // C++26
    "__cpp_lib_inplace_vector",                      // C++26
    "__cpp_lib_int_pow2",                            // C++20
    "__cpp_lib_integer_comparison_functions",        // C++20
    "__cpp_lib_integer_sequence",                    // C++14
    "__cpp_lib_integral_constant_callable",          // C++14
    "__cpp_lib_interpolate",                         // C++20
    "__cpp_lib_invoke",                              // C++17
    "__cpp_lib_invoke_r",                            // C++23
    "__cpp_lib_ios_noreplace",                       // C++23
    "__cpp_lib_is_aggregate",                        // C++17
    "__cpp_lib_is_constant_evaluated",               // C++20
    "__cpp_lib_is_final",                            // C++14
    "__cpp_lib_is_implicit_lifetime",                // C++23
    "__cpp_lib_is_invocable",                        // C++17
    "__cpp_lib_is_layout_compatible",                // C++20
    "__cpp_lib_is_nothrow_convertible",              // C++20
    "__cpp_lib_is_null_pointer",                     // C++14
    "__cpp_lib_is_pointer_interconvertible",         // C++20
    "__cpp_lib_is_scoped_enum",                      // C++23
    "__cpp_lib_is_sufficiently_aligned",             // C++26
    "__cpp_lib_is_swappable",                        // C++17
    "__cpp_lib_is_virtual_base_of",                  // C++26
    "__cpp_lib_is_within_lifetime",                  // C++26
    "__cpp_lib_jthread",                             // C++20
    "__cpp_lib_latch",                               // C++20
    "__cpp_lib_launder",                             // C++17
    "__cpp_lib_linalg",                              // C++26
    "__cpp_lib_list_remove_return_type",             // C++20
    "__cpp_lib_logical_traits",                      // C++17
    "__cpp_lib_make_from_tuple",                     // C++17
    "__cpp_lib_make_reverse_iterator",               // C++14
    "__cpp_lib_make_unique",                         // C++14
    "__cpp_lib_map_try_emplace",                     // C++17
    "__cpp_lib_math_constants",                      // C++20
    "__cpp_lib_math_special_functions",              // C++17
    "__cpp_lib_mdspan",                              // C++23
    "__cpp_lib_memory_resource",                     // C++17
    "__cpp_lib_modules",                             // C++23
    "__cpp_lib_move_iterator_concept",               // C++23
    "__cpp_lib_move_only_function",                  // C++23
    "__cpp_lib_node_extract",                        // C++17
    "__cpp_lib_nonmember_container_access",          // C++17
    "__cpp_lib_not_fn",                              // C++17
    "__cpp_lib_null_iterators",                      // C++14
    "__cpp_lib_optional",                            // C++17
    "__cpp_lib_optional_range_support",              // C++26
    "__cpp_lib_out_ptr",                             // C++23
    "__cpp_lib_parallel_algorithm",                  // C++17
    "__cpp_lib_philox_engine",                       // C++26
    "__cpp_lib_polymorphic",                         // C++26
    "__cpp_lib_polymorphic_allocator",               // C++20
    "__cpp_lib_print",                               // C++23
    "__cpp_lib_quoted_string_io",                    // C++14
    "__cpp_lib_ranges",                              // C++20
    "__cpp_lib_ranges_as_const",                     // C++23
    "__cpp_lib_ranges_as_rvalue",                    // C++23
    "__cpp_lib_ranges_cache_latest",                 // C++26
    "__cpp_lib_ranges_cartesian_product",            // C++23
    "__cpp_lib_ranges_chunk",                        // C++23
    "__cpp_lib_ranges_chunk_by",                     // C++23
    "__cpp_lib_ranges_concat",                       // C++26
    "__cpp_lib_ranges_contains",                     // C++23
    "__cpp_lib_ranges_enumerate",                    // C++23
    "__cpp_lib_ranges_find_last",                    // C++23
    "__cpp_lib_ranges_fold",                         // C++23
    "__cpp_lib_ranges_generate_random",              // C++26
    "__cpp_lib_ranges_iota",                         // C++23
    "__cpp_lib_ranges_join_with",                    // C++23
    "__cpp_lib_ranges_repeat",                       // C++23
    "__cpp_lib_ranges_reserve_hint",                 // C++26
    "__cpp_lib_ranges_slide",                        // C++23
    "__cpp_lib_ranges_starts_ends_with",             // C++23
    "__cpp_lib_ranges_stride",                       // C++23
    "__cpp_lib_ranges_to_container",                 // C++23
    "__cpp_lib_ranges_to_input",                     // C++26
    "__cpp_lib_ranges_zip",                          // C++23
    "__cpp_lib_ratio",                               // C++26
    "__cpp_lib_raw_memory_algorithms",               // C++17
    "__cpp_lib_rcu",                                 // C++26
    "__cpp_lib_reference_from_temporary",            // C++23
    "__cpp_lib_reference_wrapper",                   // C++26
    "__cpp_lib_remove_cvref",                        // C++20
    "__cpp_lib_result_of_sfinae",                    // C++14
    "__cpp_lib_robust_nonmodifying_seq_ops",         // C++14
    "__cpp_lib_sample",                              // C++17
    "__cpp_lib_saturation_arithmetic",               // C++26
    "__cpp_lib_scoped_lock",                         // C++17
    "__cpp_lib_semaphore",                           // C++20
    "__cpp_lib_senders",                             // C++26
    "__cpp_lib_shared_mutex",                        // C++17
    "__cpp_lib_shared_ptr_arrays",                   // C++17
    "__cpp_lib_shared_ptr_weak_type",                // C++17
    "__cpp_lib_shared_timed_mutex",                  // C++14
    "__cpp_lib_shift",                               // C++20
    "__cpp_lib_simd",                                // C++26
    "__cpp_lib_simd_complex",                        // C++26
    "__cpp_lib_smart_ptr_for_overwrite",             // C++20
    "__cpp_lib_smart_ptr_owner_equality",            // C++26
    "__cpp_lib_source_location",                     // C++20
    "__cpp_lib_span",                                // C++20
    "__cpp_lib_span_initializer_list",               // C++26
    "__cpp_lib_spanstream",                          // C++23
    "__cpp_lib_ssize",                               // C++20
    "__cpp_lib_sstream_from_string_view",            // C++26
    "__cpp_lib_stacktrace",                          // C++23
    "__cpp_lib_start_lifetime_as",                   // C++23
    "__cpp_lib_starts_ends_with",                    // C++20
    "__cpp_lib_stdatomic_h",                         // C++23
    "__cpp_lib_string_contains",                     // C++23
    "__cpp_lib_string_resize_and_overwrite",         // C++23
    "__cpp_lib_string_udls",                         // C++14
    "__cpp_lib_string_view",                         // C++17
    "__cpp_lib_submdspan",                           // C++26
    "__cpp_lib_syncbuf",                             // C++20
    "__cpp_lib_text_encoding",                       // C++26
    "__cpp_lib_three_way_comparison",                // C++20
    "__cpp_lib_to_address",                          // C++20
    "__cpp_lib_to_array",                            // C++20
    "__cpp_lib_to_chars",                            // C++17
    "__cpp_lib_to_string",                           // C++26
    "__cpp_lib_to_underlying",                       // C++23
    "__cpp_lib_transformation_trait_aliases",        // C++14
    "__cpp_lib_transparent_operators",               // C++14
    "__cpp_lib_trivially_relocatable",               // C++26
    "__cpp_lib_tuple_element_t",                     // C++14
    "__cpp_lib_tuple_like",                          // C++23
    "__cpp_lib_tuples_by_type",                      // C++14
    "__cpp_lib_type_identity",                       // C++20
    "__cpp_lib_type_trait_variable_templates",       // C++17
    "__cpp_lib_uncaught_exceptions",                 // C++17
    "__cpp_lib_unordered_map_try_emplace",           // C++17
    "__cpp_lib_unreachable",                         // C++23
    "__cpp_lib_unwrap_ref",                          // C++20
    "__cpp_lib_variant",                             // C++17
    "__cpp_lib_void_t",                              // C++17
};

bool CompletionHelper::is_cxx_keyword(const wxString& word) { return cppKeywords.count(word) != 0; }

std::vector<wxString> CompletionHelper::split_function_signature(const wxString& signature, wxString* return_value,
                                                                 size_t flags) const
{
    // ---------------------------------------------------------------------------------------------
    // ----------------macros start-------------------------------------------------------------------
    // ---------------------------------------------------------------------------------------------
#define ADD_CURRENT_PARAM(current_param) \
    current_param->Trim().Trim(false);   \
    if(!current_param->empty()) {        \
        args.push_back(*current_param);  \
    }                                    \
    current_param->clear();
#define LAST_TOKEN_IS(token_type) (!types.empty() && (types[types.size() - 1] == token_type))
#define LAST_TOKEN_IS_ONE_OF_2(t1, t2) (LAST_TOKEN_IS(t1) || LAST_TOKEN_IS(t2))
#define LAST_TOKEN_IS_ONE_OF_3(t1, t2, t3) (LAST_TOKEN_IS_ONE_OF_2(t1, t2) || LAST_TOKEN_IS(t3))
#define LAST_TOKEN_IS_ONE_OF_4(t1, t2, t3, t4) (LAST_TOKEN_IS_ONE_OF_3(t1, t2, t3) || LAST_TOKEN_IS(t4))
#define LAST_TOKEN_IS_ONE_OF_5(t1, t2, t3, t4, t5) (LAST_TOKEN_IS_ONE_OF_4(t1, t2, t3, t4) || LAST_TOKEN_IS(t5))
#define LAST_TOKEN_IS_CLOSING_PARENTHESES() LAST_TOKEN_IS_ONE_OF_4('}', ']', '>', ')')
#define LAST_TOKEN_IS_OPEN_PARENTHESES() LAST_TOKEN_IS_ONE_OF_4('{', '[', '<', '(')
#define REMOVE_TRAILING_SPACE()                                                         \
    if(!current_param->empty() && (*current_param)[current_param->size() - 1] == ' ') { \
        current_param->RemoveLast();                                                    \
    }
#define APPEND_SPACE_IF_MISSING()                                                         \
    if(!current_param->empty() && ((*current_param)[current_param->size() - 1] != ' ')) { \
        current_param->Append(" ");                                                       \
    }

    // ---------------------------------------------------------------------------------------------
    // ----------------macros end-------------------------------------------------------------------
    // ---------------------------------------------------------------------------------------------
    CxxTokenizer tokenizer;
    tokenizer.Reset(signature);

    CxxLexerToken token;

    wxString cur_func_param;
    wxString* current_param = &cur_func_param;
    std::vector<wxString> args;
    std::vector<wxString> func_args;
    int depth = 0;

    std::vector<int> types;
    // search for the first opening brace
    while(tokenizer.NextToken(token)) {
        if(token.GetType() == '(') {
            depth = 1;
            break;
        }
    }

    bool done_collecting_args = false;
    constexpr int STATE_NORMAL = 0;
    constexpr int STATE_DEFAULT_VALUE = 1;
    int state = STATE_NORMAL;
    while(tokenizer.NextToken(token)) {
        switch(state) {
        case STATE_DEFAULT_VALUE:
            // consume everything until we reach signature end
            // or until we hit a "," (where depth==1)
            switch(token.GetType()) {
            case '<':
            case '{':
            case '(':
            case '[':
                depth++;
                break;
            case '>':
            case '}':
            case ']':
                depth--;
                break;
            case ')':
                depth--;
                if(depth == 0) {
                    // end of argument reading, switch back to the normal state
                    tokenizer.UngetToken();
                    // restore the depth
                    depth++;
                    state = STATE_NORMAL;
                }
                break;
            case ',':
                if(depth == 1) {
                    tokenizer.UngetToken();
                    state = STATE_NORMAL;
                }
                break;
            default:
                break;
            }
            break;
        case STATE_NORMAL:
            switch(token.GetType()) {
            case T_ALIGNAS:
            case T_ALIGNOF:
            case T_AND:
            case T_AND_EQ:
            case T_ASM:
            case T_AUTO:
            case T_BITAND:
            case T_BITOR:
            case T_BOOL:
            case T_BREAK:
            case T_CATCH:
            case T_CHAR:
            case T_CHAR16_T:
            case T_CHAR32_T:
            case T_CLASS:
            case T_COMPL:
            case T_CONST:
            case T_CONSTEXPR:
            case T_CONST_CAST:
            case T_CONTINUE:
            case T_DECLTYPE:
            case T_DEFAULT:
            case T_DELETE:
            case T_DO:
            case T_DOUBLE:
            case T_DYNAMIC_CAST:
            case T_ELSE:
            case T_ENUM:
            case T_EXPLICIT:
            case T_EXPORT:
            case T_EXTERN:
            case T_FALSE:
            case T_FINAL:
            case T_FLOAT:
            case T_FOR:
            case T_FRIEND:
            case T_GOTO:
            case T_IF:
            case T_INLINE:
            case T_INT:
            case T_LONG:
            case T_MUTABLE:
            case T_NAMESPACE:
            case T_NEW:
            case T_NOEXCEPT:
            case T_NOT:
            case T_NOT_EQ:
            case T_NULLPTR:
            case T_OPERATOR:
            case T_OR:
            case T_OR_EQ:
            case T_OVERRIDE:
            case T_PRIVATE:
            case T_PROTECTED:
            case T_PUBLIC:
            case T_REGISTER:
            case T_REINTERPRET_CAST:
            case T_CASE:
            case T_SHORT:
            case T_SIGNED:
            case T_SIZEOF:
            case T_STATIC:
            case T_STATIC_ASSERT:
            case T_STATIC_CAST:
            case T_STRUCT:
            case T_SWITCH:
            case T_TEMPLATE:
            case T_THREAD_LOCAL:
            case T_THROW:
            case T_TRUE:
            case T_TRY:
            case T_TYPEDEF:
            case T_TYPEID:
            case T_TYPENAME:
            case T_UNION:
            case T_UNSIGNED:
            case T_USING:
            case T_VIRTUAL:
            case T_VOID:
            case T_VOLATILE:
            case T_WCHAR_T:
            case T_WHILE:
            case T_XOR:
            case T_XOR_EQ:
            case T_STRING:
            case T_DOT_STAR:
            case T_ARROW_STAR:
            case T_PLUS_PLUS:
            case T_MINUS_MINUS:
            case T_LS:
            case T_LE:
            case T_GE:
            case T_EQUAL:
            case T_NOT_EQUAL:
            case T_AND_AND:
            case T_OR_OR:
            case T_STAR_EQUAL:
            case T_SLASH_EQUAL:
            case T_DIV_EQUAL:
            case T_PLUS_EQUAL:
            case T_MINUS_EQUAL:
            case T_LS_ASSIGN:
            case T_RS_ASSIGN:
            case T_AND_EQUAL:
            case T_POW_EQUAL:
            case T_OR_EQUAL:
            case T_3_DOTS:
            case '&':
            case ':':
                current_param->Append(token.GetWXString());
                APPEND_SPACE_IF_MISSING();
                break;
            case '*':
                if(LAST_TOKEN_IS('*')) {
                    REMOVE_TRAILING_SPACE();
                }
                current_param->Append(token.GetWXString());
                APPEND_SPACE_IF_MISSING();
                break;
            case T_IDENTIFIER: {
                wxString peeked_token_text;
                bool add_identifier = true;
                int next_token_type = tokenizer.PeekToken(peeked_token_text);
                // Check if we want to ignore the argument name
                if((depth == 1) && (next_token_type == ',' || next_token_type == '=' || next_token_type == ')') &&
                   (flags & STRIP_NO_NAME)) {
                    // two consecutive T_IDENTIFIER, don't add it
                    add_identifier = false;
                } else if(LAST_TOKEN_IS_CLOSING_PARENTHESES() || LAST_TOKEN_IS_ONE_OF_2(T_IDENTIFIER, '*')) {
                    APPEND_SPACE_IF_MISSING();
                }

                if(add_identifier) {
                    current_param->Append(token.GetWXString());
                }
            } break;
            case T_ARROW:
                if(done_collecting_args) {
                    // we are collecting function return value now, disregard it
                } else {
                    current_param->Append(token.GetWXString());
                }
                break;
            case ',':
                if(depth == 1) {
                    ADD_CURRENT_PARAM(current_param);
                } else {
                    current_param->Append(",");
                    APPEND_SPACE_IF_MISSING();
                }
                break;
            case '>':
            case ']':
            case '}':
                depth--;
                REMOVE_TRAILING_SPACE();
                current_param->Append(token.GetWXString());
                break;
            case ')':
                depth--;
                if(!done_collecting_args && depth == 0) {
                    // reached signature end
                    ADD_CURRENT_PARAM(current_param);
                    func_args.swap(args);
                    done_collecting_args = true;
                } else {
                    if(LAST_TOKEN_IS_CLOSING_PARENTHESES() || LAST_TOKEN_IS(T_IDENTIFIER)) {
                        REMOVE_TRAILING_SPACE();
                    }
                    current_param->Append(token.GetWXString());
                }
                break;
            case '[':
            case '<':
            case '(':
            case '{':
                depth++;
                REMOVE_TRAILING_SPACE();
                current_param->Append(token.GetWXString());
                break;
            case '=':
                if((depth == 1) && (flags & STRIP_NO_DEFAULT_VALUES)) {
                    state = STATE_DEFAULT_VALUE;
                } else {
                    APPEND_SPACE_IF_MISSING();
                    current_param->Append("= ");
                }
                break;
            default:
                current_param->Append(token.GetWXString());
                break;
            }

            break;
        }
        types.push_back(token.GetType());
    }

    if(!done_collecting_args) {
        // we did not complete
        func_args.swap(args);
    } else {
        // check if we have a return value
        ADD_CURRENT_PARAM(current_param);
        if(!args.empty() && return_value) {
            *return_value = args[0].Trim().Trim(false);
        }
    }
    return func_args;
#undef ADD_CURRENT_PARAM
#undef LAST_TOKEN_IS
#undef LAST_TOKEN_IS_ONE_OF_2
#undef LAST_TOKEN_IS_ONE_OF_3
#undef LAST_TOKEN_IS_ONE_OF_4
#undef LAST_TOKEN_IS_ONE_OF_5
#undef LAST_TOKEN_IS_CLOSING_PARENTHESES
#undef LAST_TOKEN_IS_OPEN_PARENTHESES
#undef REMOVE_TRAILING_SPACE
}

namespace
{
thread_local wxRegEx reDoxyParam("([@\\\\]{1}param)[ \t]+([_a-z][a-z0-9_]*)?", wxRE_DEFAULT | wxRE_ICASE);
thread_local wxRegEx reDoxyBrief("([@\\\\]{1}(brief|details))[ \t]*", wxRE_DEFAULT | wxRE_ICASE);
thread_local wxRegEx reDoxyThrow("([@\\\\]{1}(throw|throws))[ \t]*", wxRE_DEFAULT | wxRE_ICASE);
thread_local wxRegEx reDoxyReturn("([@\\\\]{1}(return|retval|returns))[ \t]*", wxRE_DEFAULT | wxRE_ICASE);
thread_local wxRegEx reDoxyToDo("([@\\\\]{1}todo)[ \t]*", wxRE_DEFAULT | wxRE_ICASE);
thread_local wxRegEx reDoxyRemark("([@\\\\]{1}(remarks|remark))[ \t]*", wxRE_DEFAULT | wxRE_ICASE);
thread_local wxRegEx reDate("([@\\\\]{1}date)[ \t]*", wxRE_DEFAULT | wxRE_ICASE);
thread_local wxRegEx reFN("([@\\\\]{1}fn)[ \t]*", wxRE_DEFAULT | wxRE_ICASE);
} // namespace

wxString CompletionHelper::format_comment(TagEntryPtr tag, const wxString& input_comment) const
{
    return format_comment(tag.get(), input_comment);
}

wxString CompletionHelper::format_comment(TagEntry* tag, const wxString& input_comment) const
{
    wxString beautified_comment;
    if(tag) {
        if(tag->IsMethod()) {
            auto args = split_function_signature(tag->GetSignature(), nullptr);
            beautified_comment << "```\n";
            if(args.empty()) {
                beautified_comment << tag->GetName() << "()\n";
            } else {
                beautified_comment << tag->GetName() << "(\n";
                for(const wxString& arg : args) {
                    beautified_comment << "  " << arg << ",\n";
                }

                if(beautified_comment.EndsWith(",\n")) {
                    beautified_comment.RemoveLast(2);
                }
                beautified_comment << ")\n";
            }
            beautified_comment << "```\n";
        } else if(tag->GetKind() == "variable" || tag->GetKind() == "member" || tag->IsLocalVariable()) {
            wxString clean_pattern = tag->GetPatternClean();
            clean_pattern.Trim().Trim(false);

            if(!clean_pattern.empty()) {
                beautified_comment << "```\n";
                beautified_comment << clean_pattern << "\n";
                beautified_comment << "```\n";
            } else {
                beautified_comment << tag->GetKind() << "\n";
            }
        } else {
            // other
            wxString clean_pattern = tag->GetPatternClean();
            clean_pattern.Trim().Trim(false);
            if(!clean_pattern.empty()) {
                beautified_comment << "```\n";
                beautified_comment << clean_pattern << "\n";
                beautified_comment << "```\n";
            }
        }
    }
    wxString formatted_comment;
    if(!input_comment.empty()) {
        formatted_comment = wrap_lines(input_comment);

        if(reDoxyParam.IsValid() && reDoxyParam.Matches(formatted_comment)) {
            reDoxyParam.ReplaceAll(&formatted_comment, "\nParameter\n`\\2`");
        }

        if(reDoxyBrief.IsValid() && reDoxyBrief.Matches(formatted_comment)) {
            reDoxyBrief.ReplaceAll(&formatted_comment, "");
        }

        if(reDoxyThrow.IsValid() && reDoxyThrow.Matches(formatted_comment)) {
            reDoxyThrow.ReplaceAll(&formatted_comment, "\n`Throws:`\n");
        }

        if(reDoxyReturn.IsValid() && reDoxyReturn.Matches(formatted_comment)) {
            reDoxyReturn.ReplaceAll(&formatted_comment, "\n`Returns:`\n");
        }

        if(reDoxyToDo.IsValid() && reDoxyToDo.Matches(formatted_comment)) {
            reDoxyToDo.ReplaceAll(&formatted_comment, "\nTODO\n");
        }

        if(reDoxyRemark.IsValid() && reDoxyRemark.Matches(formatted_comment)) {
            reDoxyRemark.ReplaceAll(&formatted_comment, "\n  ");
        }

        if(reDate.IsValid() && reDate.Matches(formatted_comment)) {
            reDate.ReplaceAll(&formatted_comment, "Date ");
        }

        if(reFN.IsValid() && reFN.Matches(formatted_comment)) {
            size_t fnStart, fnLen, fnEnd;
            if(reFN.GetMatch(&fnStart, &fnLen)) {
                fnEnd = formatted_comment.find('\n', fnStart);
                if(fnEnd != wxString::npos) {
                    // remove the string from fnStart -> fnEnd (including ther terminating \n)
                    formatted_comment.Remove(fnStart, (fnEnd - fnStart) + 1);
                }
            }
        }

        // horizontal line
        if(!beautified_comment.empty()) {
            beautified_comment << "---\n";
        }
        beautified_comment << formatted_comment;
    }
    return beautified_comment;
}

thread_local wxRegEx reIncludeFile("include *[\\\"\\<]{1}([a-zA-Z0-9_/\\.\\+\\-]*)");

bool CompletionHelper::is_line_include_statement(const wxString& line, wxString* file_name, wxString* suffix) const
{
    wxString tmp_line = line;

    tmp_line.Trim().Trim(false);
    tmp_line.Replace("\t", " ");

    // search for the "#"
    wxString remainder;
    if(!tmp_line.StartsWith("#", &remainder)) {
        return false;
    }

    if(!reIncludeFile.Matches(remainder)) {
        return false;
    }

    if(file_name) {
        *file_name = reIncludeFile.GetMatch(remainder, 1);
    }

    if(suffix) {
        if(tmp_line.Contains("<")) {
            *suffix = ">";
        } else {
            *suffix = "\"";
        }
    }
    return true;
}

bool CompletionHelper::is_include_statement(const wxString& f_content, wxString* file_name, wxString* suffix) const
{
    // read backward until we find LF
    if(f_content.empty()) {
        return false;
    }

    int i = f_content.size() - 1;
    for(; i >= 0; --i) {
        if(f_content[i] == '\n') {
            break;
        }
    }

    wxString line = f_content.Mid(i);
    return is_line_include_statement(line, file_name, suffix);
}

wxString CompletionHelper::normalize_function(const TagEntry* tag, size_t flags)
{
    wxString return_value;
    wxString fullname;

    wxString name = tag->GetName();
    wxString signature = tag->GetSignature();
    fullname << name << "(";
    std::vector<wxString> args = split_function_signature(signature, &return_value, flags);
    wxString funcsig;
    for(const wxString& arg : args) {
        funcsig << arg << ", ";
    }

    if(funcsig.EndsWith(", ")) {
        funcsig.RemoveLast(2);
    }

    fullname << funcsig << ")";
    if(tag->is_const()) {
        fullname << " const";
    }
    return fullname;
}

wxString CompletionHelper::normalize_function(TagEntryPtr tag, size_t flags)
{
    return normalize_function(tag.get(), flags);
}

std::vector<wxString> CompletionHelper::get_cxx_keywords()
{
    return {cppKeywords.begin(), cppKeywords.end()};
}
