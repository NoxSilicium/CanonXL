# Coding Convention

## Scope

This convention is independent of business domain, architecture and build tools.
It applies to all maintained application code, internal libraries, tests, scripts,
tools and benchmarks, including existing code. Generated code and third-party
dependencies retain their supplier's conventions; identify local adaptations.
For other languages, use established conventions and record naming/formatting
choices in a project appendix. Shared readability, contract, documentation and
verification rules still apply. Exceptions must state their reason and scope;
a file's age alone is not an exception.

In CanonXL, `archives/` is historical, unmaintained material. Never format,
correct, rename or otherwise modify its contents. Reused implementations belong
outside the archives. Raw captures and firmware backups remain immutable evidence.

Only the project README is bilingual: `README.md` is English and `README.fr.md`
is its French translation. Keep their facts, links and illustrations synchronized.
Code, comments, technical guides and project context remain English.

## Principles

Prefer short interfaces, explicit responsibilities and visible resource ownership.
Structural changes must not silently change behavior.

- Give each function or module an identifiable responsibility.
- Use English for identifiers, comments and maintained documentation, including
  project context. The product UI retains French/English localization.
- Avoid ambiguous abbreviations and type prefixes.
- Include units in names (`timeoutMs`, `sizeBytes`) or document them explicitly.
- Express Boolean state/capability: `isReady`, `hasPendingWork`, `canRetry`.
  Action options may use verbs such as `enableLogging`.
- Name repeated or difficult-to-interpret domain values as constants.
- Verify functional fixes separately from convention-only changes.

## Naming

PascalCase starts uppercase; camelCase starts lowercase. snake_case separates
words with underscores, UPPER_SNAKE_CASE also capitalizes them, and kebab-case
separates words with hyphens.

| Element | C++ | PHP | JavaScript / TypeScript | Python | Shell |
| --- | --- | --- | --- | --- | --- |
| Class, structure, type | PascalCase | PascalCase | PascalCase | PascalCase | Not applicable |
| Function or method | camelCase | camelCase | camelCase | camelCase | camelCase |
| Variable or parameter | camelCase | camelCase | camelCase | camelCase | camelCase |
| Named constant | camelCase | camelCase | UPPER_SNAKE_CASE | camelCase | camelCase |
| Private member | camelCase | camelCase | #camelCase or private camelCase | camelCase | Not applicable |
| Public member | camelCase | camelCase | camelCase | camelCase | camelCase |
| Enumeration value | PascalCase | PascalCase | PascalCase for TypeScript enums | UPPER_SNAKE_CASE | Not applicable |

All C++ variables use camelCase regardless of scope/visibility: local/global
variables, parameters, public/protected/private members, static members and
const/constexpr constants. No underscores, type prefixes or `k` constant prefix:
use `timeoutMs`, `itemCount`, `defaultRetryCount`. Use `this->` to distinguish
a member from a same-named parameter. Python and shell extend this convention
to applicable functions/methods.

JavaScript `const` means non-reassignable binding, not uppercase naming for every
variable. Reserve UPPER_SNAKE_CASE for fixed shared constants like DEFAULT_TIMEOUT_MS.

- C++ namespaces: lowercase, snake_case for multiple words (`image_processing`).
- C++ macros: UPPER_SNAKE_CASE; owned macros carry a collision-resistant prefix.
- New project environment variables: camelCase. Keep system/tool/external-contract
  names, including `HOME`, `PATH`, `LD_LIBRARY_PATH`.
- Treat acronyms like words: `HttpClient`, `parseJson`, `requestId`.
- Conventional scientific notation (`x`, `y`, `dx`) may remain short within
  an unambiguous local scope.
- Keep names required by callbacks, overloads, special methods and interfaces.

### Files and External Contracts

- C++: TypeName.hpp/TypeName.cpp for a primary type; camelCase for function
  collections. Entry points may retain conventional names such as main.cpp.
- PHP: TypeName.php for a primary type; camelCase for function collections;
  conventional entry points such as index.php and existing public paths remain.
- JavaScript/TypeScript/HTML/CSS: kebab-case; component files may use their type name.
- Python: snake_case modules. Shell: snake_case.sh scripts, kebab-case installed commands.
- Keep tool-mandated configuration filenames.
- Tests follow framework discovery; otherwise use test_ followed by a snake_case name.

Internal renaming must not implicitly modify JSON fields, SQL columns, command
options, environment variables, routes, persisted filenames or third-party
symbols. Use explicit conversion boundaries where conventions differ. Public
changes require documented migration and consumer verification; compatibility
aliases have an identified scope and removal condition.

## Formatting

- UTF-8, LF endings, final newline.
- Four-space indentation throughout maintained code/configuration. Formats
  requiring tabs, such as Make recipes, retain their required syntax.
- Allman opening braces on a new line in C++, PHP, JavaScript and TypeScript.
  Conditions/loops use braces even for one statement.
- Every braced body starts on the next line and is indented four spaces relative
  to its opening brace, including named and anonymous namespaces. Nested namespaces
  each add one indentation level. Object and aggregate contents are multiline too.
- Target 100 columns; unsplittable URLs, strings and data may exceed this limit.
- One statement per line; blank lines separate logical steps.
- Avoid manual column alignment that forces unrelated neighboring changes.
- JavaScript/TypeScript: semicolons, single-quoted ordinary strings, backtick templates.
- Python follows its formatter, without artificial brace-style adaptation.
- CSS/JSON and other declarative formats follow formatter syntax, with four spaces.

```javascript
if (isReady)
{
    processRequest();
}
```

Versioned formatter configuration is authoritative for presentation details
and must implement these rules.

## C++

### Headers

Use pragma once when supported, otherwise a project-prefixed UPPER_SNAKE_CASE
guard. Headers include declarations directly needed by their interfaces.
Include the associated .hpp first in a .cpp. Group includes: associated header,
internal dependencies, third-party dependencies, standard library. Sort groups
except for documented ordering dependencies. Prefer forward declarations when
pointers/references suffice. Avoid using-namespace directives in headers and
third-party types in APIs where standard/domain types suffice.

### Types and Lifetime

Use const for immutable values/access, auto when the type is obvious or obscures
the expression's role, explicit types when clarifying contracts. Prefer enum
class and using aliases, nullptr and override. Use explicit C++ conversions,
not C-style casts. Manage files/locks/allocations through RAII, including errors.
Owned members use values or unique_ptr; shared_ptr requires genuine shared
ownership. Raw pointers are non-owning; document validity if not obvious.
Required objects use references; pointers allow absence. Internal .cpp functions
and data belong in anonymous namespaces; macros serve preprocessing needs.

```cpp
namespace processing
{
    constexpr unsigned defaultRetryCount = 3;

    class RequestWorker
    {
        public:
        bool isReady() const;

        void setTimeoutMs(unsigned timeoutMs)
        {
            this->timeoutMs = timeoutMs;
        }

        private:
        unsigned timeoutMs = 1000;
    };
} // namespace processing
```

## Other Languages

### PHP

Follow C++ wherever syntax/interfaces permit: English identifiers, PascalCase
types/enum cases, camelCase functions/methods/variables/parameters/properties/
constants, no underscores/type prefixes. Preserve magic methods, superglobals
and externally required names. Use $this->memberName; do not invent PHP equivalents
for C++-only rules. Use PHP types where useful and PHPDoc for constraints, units,
results and side effects. Preserve JSON/SQL/route/persistence contracts.

### JavaScript and TypeScript

Default to const; use let for reassignment. Use ===/!==. Explicitly handle or
propagate promise errors; avoid silent rejections/unobserved async results.
Type public TypeScript boundaries and validate external data at runtime;
prefer precise types or unknown over any where possible.

### Python

Owned variables/parameters/attributes/constants/functions/methods use camelCase;
classes use PascalCase. This deliberate C++-aligned convention differs from
usual Python snake_case variables/functions and UPPER_SNAKE_CASE constants.
No underscores/type prefixes except mandated names. Document internal attribute
visibility; naming is not access control. Keep special names (__init__, __iter__,
__name__, __all__) and library/framework/interface names. Modules/test discovery
follow file rules. Configure naming checks without disabling other language checks.
Use context managers, avoid mutable defaults and overly broad exception catches.
Group imports: standard library, third-party, internal; avoid wildcard imports.
Use docstrings and useful signature type annotations.

```python
defaultRetryCount = 3


class RequestWorker:
    def __init__(self, timeoutMs: int = 1000) -> None:
        self.timeoutMs = timeoutMs

    def setTimeoutMs(self, timeoutMs: int) -> None:
        self.timeoutMs = timeoutMs
```

### Shell

Owned variables/constants/functions use camelCase without underscores/type
prefixes. Keep system/tool/contract names and positional/special syntax. Declare
constants readonly; use Bash local where appropriate (not guaranteed by POSIX).
Declare/respect the interpreter. Quote expansions unless deliberate, documented
splitting; Bash argument lists use arrays. Check errors: set -e is not a replacement
for handling expected failures. Diagnostics go to stderr, results to stdout.
Use dedicated temporary directories and trap cleanup. Never repurpose reserved
HOME/PATH/IFS variables for domain data.

```sh
defaultRetryCount=3
readonly defaultRetryCount
configFile="./settings.json"

logMessage()
{
    printf '%s\n' "$1" >&2
}
```

## Errors and Concurrency

Validate external inputs at network/file/user/library boundaries. Do not turn
errors silently into success or empty results; distinguish expected states from
failures. Messages identify operation/context without secrets/unnecessary personal
data. Document locks/atomics protecting shared data and thread-call restrictions.
Blocking work needs timeout/cancellation where it could prevent progress/shutdown.
Optimize from measurements while retaining invariants/readability.

## Documentation

All maintained comments and documentation are English; required protocol/library
names retain their spelling. Every maintained C++ source has a Doxygen file comment.

Do not add editorial reference, publication or update dates to maintained guides
or license headings. Use version identifiers when needed. Preserve copyright years
and dates intrinsic to cited sources or immutable measurement records.

Use native Markdown image embeds. Do not add empty lines directly before or
after photographs. Use a Markdown hard break if a caption would otherwise
flow beside the photograph.

Project-owned source files carry `SPDX-License-Identifier: LicenseRef-CanonXL-NC-1.0`
and their copyright attribution. Preserve third-party notices and licenses; do not
replace them with the project license. The root `LICENSE` contains the full terms.

```cpp
/**
 * @file RequestWorker.hpp
 * @brief Exposes request processing and its retry policy.
 */
```

Document classes/structures/enums/aliases, public/protected functions, non-obvious
private contracts/side effects/synchronization, fields needing unit/ownership/
lifetime/locking clarification, protocol constants and sentinels. Use @param,
@return, @retval, @throws, @pre, @post, @note, @warning and @remark as applicable.

```cpp
/**
 * @brief Waits for the current task to finish.
 * @param timeoutMs Maximum wait in milliseconds, or zero for no wait.
 * @return True if the task has completed.
 * @note May be called from a worker thread.
 */
bool waitForCompletion(std::uint32_t timeoutMs);
```

Preserve meaningful existing comments. Check behavior, correct obsolete units/
names/assumptions, convert declaration contracts to Doxygen, keep algorithm
rationale nearby, remove only obvious paraphrases/false information. Avoid
narrating trivial local statements.

## Verification and Adoption

Tests cover observable behavior/contracts/boundaries rather than copying
implementation. They are reproducible and state hardware/network/time/data needs.
Add regression tests where useful. Verification follows impact; full harmonization
requires all affected modules/configurations to compile and pass tests.
Provide reproducible format/check/analyse/build/test/documentation commands,
pin tool versions/configuration and use the same rules locally/automatically.
Suitable tools include clang-format/clang-tidy, Prettier/ESLint, Black/Ruff,
shfmt/ShellCheck; document selected tools separately. Review/fix warnings;
targeted suppressions explain their constraints.

1. Inventory maintained sources, generators, dependencies and compatibility contracts.
2. Configure tools and establish baseline builds/tests.
3. Format and rename separately using language-aware tools where available.
4. Update references/tests/docs, including dynamic symbol strings.
5. Compile/test supported configurations; compare public contracts to baseline.
6. Enable checks and record justified, narrowly scoped exceptions.
