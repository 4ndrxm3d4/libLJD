#ifndef LJD_MAIN_COMPAT_H
#define LJD_MAIN_COMPAT_H

/* Compatibility replacement for the original main.h when building as a
 * shared library. This removes the Windows-only assumptions while keeping
 * the same helper API used by bytecode/ast/lua modules.
 */

#include "ljd_compat.h"

/* Ensure char is unsigned on platforms where it matters, matching the
 * original desktop requirement. The original header rejected unsigned-less
 * builds; we simply make it the default here for library builds. */
#ifndef _CHAR_UNSIGNED
/* Note: if your toolchain still treats char as signed, define
 * _CHAR_UNSIGNED=1 in build flags. For Android NDK this is usually not
 * necessary. */
#endif

#define DEBUG_INFO __FUNCTION__, __FILE__, __LINE__

constexpr char PROGRAM_NAME[] = "LuaJIT Decompiler v2";
constexpr uint64_t DOUBLE_SIGN = 0x8000000000000000;
constexpr uint64_t DOUBLE_EXPONENT = 0x7FF0000000000000;
constexpr uint64_t DOUBLE_FRACTION = 0x000FFFFFFFFFFFFF;
constexpr uint64_t DOUBLE_SPECIAL = DOUBLE_EXPONENT;
constexpr uint64_t DOUBLE_NEGATIVE_ZERO = DOUBLE_SIGN;

void print(const std::string& message);
void print_progress_bar(const double& progress = 0, const double& total = 100);
void erase_progress_bar();
void assert(const bool& assertion, const std::string& message, const std::string& filePath, const std::string& function, const std::string& source, const uint32_t& line);
std::string byte_to_string(const uint8_t& byte);

class Bytecode;
class Ast;
class Lua;

#include "bytecode/bytecode.h"
#include "ast/ast.h"
#include "lua/lua.h"

#endif /* LJD_MAIN_COMPAT_H */
