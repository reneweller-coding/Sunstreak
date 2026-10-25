# Code style

The C++ of Sunstreak follows the programming guideline of the Computer Graphics and Virtual Reality group of the
Universität Bremen (CGVR, <https://cgvr.informatik.uni-bremen.de/progr/guidelines.shtml>): its layout, its names and
its comment template. Where the code does something else, this page says what and why. The rules are those of the
VRAudio family (Totality and its siblings); `.clang-format` is the same file.

## Layout

`.clang-format` at the root lays the code out; run it on what you change:

    clang-format -i --style=file <files>

- Braces on lines of their own, under each other (no K&R).
- Spaces inside parentheses: `if ( x > 0 )`, `f( a, b )`; a space either side of a binary operator.
- The pointer and reference sign at the name: `float *out`, `const Track &track`.
- Four spaces a level, no tabs.
- Declarations in a row and their trailing `///<` comments in columns.
- Comments are never reflowed and string literals never split, and the includes keep the order they were written in.

Generated files (`src/IconPng.h`, written by `tools/make_icon.py`) are listed in `.clang-format-ignore`. The commit that
brought the code into this layout is listed in `.git-blame-ignore-revs`; `git blame` passes over it after

    git config blame.ignoreRevsFile .git-blame-ignore-revs

## Names

| What | How | Example |
|---|---|---|
| classes, structs | upper camel case | `PlayerCar` |
| functions, methods | lower camel case | `frameAt()` |
| data members | `m_` | `m_speed` |
| static data members | `M_` | (none so far) |
| constants | upper camel case, no `k` | `SegmentLength` |
| enum types | upper camel case and `E` | `PropKindE` |
| enumerators | capitals | `PropKindE::GLASS_TOWER` |
| template type parameters, type aliases | `T` at the end | `BuildT`, `PhraseMapT` |
| macros | capitals, the prefix `SUNSTREAK_` | `SUNSTREAK_GLAPI` |
| namespace | short, small | `sunstreak` |

## Documentation

The comment template of the guideline, written for Doxygen:

- Every file starts with a `/** @file ... */` block: `@brief`, what the file is for, `@author`.
- Every function has a `/** ... */` block: `@brief`, one `@param[in]`, `@param[out]` or `@param[in,out]` per
  parameter in the order of the signature, `@return` for anything but `void`, `@pre` where the function asks something
  of its caller. There is no `@throw`: the code throws no exceptions.
- Members, enumerators, constants and type aliases have a `///<` comment behind them or a block before them; one
  declaration a line, so each has its own.

The test `doccheck` (ctest, `cmake/Doccheck.cmake`) runs Doxygen over `src` and `tests` with every warning switched on
and fails on any gap: something undocumented, a parameter or a return value not described, a documentation error, an
`@param` without its direction.

    ctest --preset msvc -R doccheck

## Where the code does something else

- **120 columns, not 80.** The guideline allows exceptions to its 80; the long names and the parameter lists of a
  world generator would otherwise break nearly every call.
- **Enum types start with a capital** (`PropKindE`, the guideline writes `myEnumTypeE`): they are types like the
  classes.
- **Local variables in lower camel case** (`maxCurve`, the guideline writes `mylocalvar` or `my_local_var`): they read
  like the parameters and members they come from.
- **Accessors are named after what they give** (`seed()`, `segment()`, `chunks()`), not `getSeed()`, as in the
  standard library; queries that answer yes or no start with `is`, `has`, `may`, `in` or `near` where that reads well.
- **The few global variables** of a file live in its anonymous namespace and start with `g_` (`g_errors`, the lines of a
  data file that did not parse); the guideline names none.
- **Names the outside world knows stay as they are**: the keywords and IDs of the data files (`serpentine`, `STELVIO`,
  `STAND_HONESTY_DE`), the keys of the settings and the records, the command line's options, the strings on the screen.
  Renaming one would break saved records and every data file.
- **The guideline's language rules are not taken over:**
  - copy constructor and assignment operator are not declared everywhere: the classes follow the rule of zero, and a
    class that must not be copied deletes the two;
  - only polymorphic classes have a virtual destructor;
  - a parameter may be a non-const reference: the documentation says `[out]` or `[in,out]`;
  - small inline functions stay in the class;
  - exceptions: none are thrown. A data file's broken line is counted and logged, a failed look-up returns `nullptr`
    or `false`.
