#ifndef LJD_CORE_SHIMS_H
#define LJD_CORE_SHIMS_H

/* Minimal public headers for library builds.
 * Do not add the original main.h here; the library builds
 * replace WinAPI calls through ljd_compat.h.
 */

#include "bytecode/bytecode.h"
#include "ast/ast.h"
#include "lua/lua.h"

#endif /* LJD_CORE_SHIMS_H */
