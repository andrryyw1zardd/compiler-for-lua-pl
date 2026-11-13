#ifndef STD_LIB_HPP
#define STD_LIB_HPP

#include "sema/sema.hpp"
#include <array>

namespace STANDARD {
    using DT = Symbol::DataType;

    void initAll(SemanticAnalyzer*);
    void initGlobalFunctions(SemanticAnalyzer*);
    void initGlobalMethods(SemanticAnalyzer*);

    constexpr std::array<std::string, 13> globalFuncNames {
        "print", "type", "tostring", "tonumber",
        "error", "assert", "select", "next",
        "pairs", "ipairs", "pcall", "xpcall",
        "setmetatable"
    };

    const static std::vector<std::vector<DT>> globalFuncRetTypes {
        std::vector<DT>{DT::NIL},                   // print
        std::vector<DT>{DT::STRING},                // type
        std::vector<DT>{DT::STRING},                // tostring
        std::vector<DT>{DT::UNKNOWN},               // tonumber
        std::vector<DT>{DT::NIL},                   // error
        std::vector<DT>{DT::UNKNOWN},               // assert (it can return its args or error)
        std::vector<DT>{DT::UNKNOWN},               // select (it returns its own args, 0 or more)
        std::vector<DT>{DT::UNKNOWN, DT::UNKNOWN},  // next (key, value)
        std::vector<DT>{DT::UNKNOWN},               // pairs (can return iter, table or nil, so just UNKNOWN)
        std::vector<DT>{DT::UNKNOWN,
            DT::UNKNOWN, DT::UNKNOWN},              // ipairs (same as pairs just 0 instead of nil)
        std::vector<DT>{DT::BOOL, DT::UNKNOWN},     // pcall
        std::vector<DT>{DT::BOOL, DT::UNKNOWN},     // xpcall
        std::vector<DT>{DT::UNKNOWN}                // setmetatable
    };
     
    static inline std::vector<std::string> globalIdentNames = {
        "math", "string", "table", "os", "io", "coroutine", "utf8", "debug",
        "package", "file" };
     
    static inline std::vector<std::vector<std::string>> globalMethodNames = {
        { "abs", "ceil", "floor", "sqrt", "sin", "cos", "tan", "asin", "acos",
          "atan", "exp", "log", "fmod", "modf", "max", "min", "random",
          "randomseed", "tointeger", "type", "ult" },
        { "byte", "char", "dump", "find", "format", "gmatch", "gsub", "len",
          "lower", "match", "pack", "packsize", "rep", "reverse", "sub",
          "upper", "unpack" },
        { "concat", "insert", "move", "pack", "remove", "sort", "unpack" },
        { "clock", "date", "difftime", "execute", "exit", "getenv", "remove",
          "rename", "setlocale", "time", "tmpname" },
        { "close", "flush", "input", "lines", "open", "output", "popen",
          "read", "tmpfile", "write" },
        { "create", "isyieldable", "resume", "running", "status", "wrap",
          "yield", "close" },
        { "char", "codepoint", "codes", "len", "offset" },
        { "debug", "gethook", "getinfo", "getlocal", "getmetatable",
          "getregistry", "getupvalue", "getuservalue", "sethook", "setlocal",
          "setmetatable", "setupvalue", "setuservalue", "traceback",
          "upvalueid", "upvaluejoin" },
        { "loadlib", "searchpath" },
        { "close", "flush", "lines", "read", "seek", "setvbuf", "write" }
    };
     
    static inline std::vector<std::vector<DT>> globalMethodRetTypes = {
        { DT::FLOAT, DT::INT, DT::INT, DT::FLOAT, DT::FLOAT, DT::FLOAT,
          DT::FLOAT, DT::FLOAT, DT::FLOAT, DT::FLOAT, DT::FLOAT, DT::FLOAT,
          DT::FLOAT, DT::UNKNOWN, DT::FLOAT, DT::FLOAT, DT::FLOAT, DT::NIL,
          DT::UNKNOWN, DT::STRING, DT::BOOL
        },
        { DT::UNKNOWN, DT::STRING, DT::STRING, DT::UNKNOWN, DT::STRING,
          DT::FUNCTION, DT::UNKNOWN, DT::INT, DT::STRING, DT::UNKNOWN,
          DT::STRING, DT::INT, DT::STRING, DT::STRING, DT::STRING,
          DT::STRING, DT::UNKNOWN 
        },
        { DT::STRING, DT::NIL, DT::TABLE, DT::TABLE, DT::UNKNOWN, DT::NIL,
          DT::UNKNOWN 
        },
        { DT::FLOAT, DT::UNKNOWN, DT::FLOAT, DT::UNKNOWN, DT::NIL,
          DT::UNKNOWN, DT::UNKNOWN, DT::UNKNOWN, DT::UNKNOWN, DT::INT,
          DT::STRING 
        },
        { DT::BOOL, DT::BOOL, DT::UNKNOWN, DT::FUNCTION, DT::UNKNOWN,
          DT::UNKNOWN, DT::UNKNOWN, DT::UNKNOWN, DT::UNKNOWN, DT::UNKNOWN 
        },
        { DT::UNKNOWN, DT::BOOL, DT::UNKNOWN, DT::UNKNOWN, DT::STRING,
          DT::FUNCTION, DT::UNKNOWN, DT::BOOL 
        },
        { DT::STRING, DT::UNKNOWN, DT::FUNCTION, DT::UNKNOWN, DT::INT },
        { DT::NIL, DT::UNKNOWN, DT::TABLE, DT::UNKNOWN, DT::TABLE, DT::TABLE,
          DT::UNKNOWN, DT::UNKNOWN, DT::NIL, DT::STRING, DT::UNKNOWN,
          DT::STRING, DT::UNKNOWN, DT::STRING, DT::UNKNOWN, DT::NIL 
        },
        { DT::UNKNOWN, DT::UNKNOWN },
        { DT::BOOL, DT::UNKNOWN, DT::FUNCTION, DT::UNKNOWN, DT::UNKNOWN,
          DT::BOOL, DT::UNKNOWN }
    };
}

#endif
