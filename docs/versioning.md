# binja-lua versioning policy

This document defines how `binja-lua` is versioned and what guarantees a
consumer can rely on between releases. Until 1.0.0 lands the plugin is
explicitly **pre-release software**, but the policy below still applies:
we start practicing semver now so that the 1.0.0 cut is a graduation,
not a sudden discipline change.

Applies to: the plugin DLL produced from this repository, the Lua API
surface it exposes, and the Lua-side extensions under `lua-api/`. Does
NOT apply to the `binaryninja-api` submodule (tracked upstream) or to
internal planning docs under `docs/` that are excluded via
`.git/info/exclude`.

## 1. Scheme

The project uses [Semantic Versioning 2.0.0](https://semver.org/) with
one explicit pre-1.0 interpretation called out below. Versions have the
form `MAJOR.MINOR.PATCH` with optional pre-release / build metadata
suffixes.

### 1.1 Pre-1.0 rule (the 0.y.z series)

SemVer 2.0 leaves 0.y.z releases loosely defined and allows either
interpretation. This project picks the following:

- **Breaking changes bump MINOR during 0.y.z.** A release that breaks
  an existing Lua script is `0.y.z -> 0.(y+1).0`, not a PATCH bump.
- **New backward-compatible surface bumps PATCH during 0.y.z.**
  Adding a property to an existing usertype, adding a new usertype, or
  exposing a new binding file is a PATCH release, not a MINOR.
- **MAJOR stays at 0 until the explicit graduation to 1.0.0.** The
  intent is that users can track the MINOR counter as a "break
  budget" and pin to `~> 0.y` if they want stability across PATCHes.

This is the minor-for-breaks-in-0.y.z variant of semver. It is more
honest than calling every break a MAJOR bump while simultaneously
shipping pre-release software, and it is less wasteful than burning
MAJOR numbers on experimental API shape decisions.

### 1.2 The 1.0.0 graduation criteria

1.0.0 will be cut when ALL of the following are true:

1. The public Lua API surface is stable enough that the project is
   willing to commit to "no more breaking changes without a MAJOR
   bump."
2. R7 through R13 (Symbol/Tag CRUD, Settings, LLIL/MLIL/HLIL operand
   walking, TypeLibrary, DebugInfo) have landed and have been
   exercised against at least one real analysis script, not just
   validation smoke tests.
3. At least one full stable release cycle has elapsed with NO
   breaking changes. Concretely: a PATCH release that ships against
   the most recent MINOR with zero entries under its "Changed -
   BREAKING" CHANGELOG section.
4. `docs/api-reference.md` is complete for every usertype exposed by
   the plugin at 1.0.0 time.
5. A CHANGELOG entry exists for every 0.y.z release back to 0.1.0.
6. **The human user has explicitly green-lit the 1.0.0 cut.**
   Agents do not auto-promote; the graduation is a deliberate
   human decision, not a threshold-triggered event.

No calendar deadline. 1.0.0 ships when all six are true. Until then
the project stays on 0.y.z and the "MINOR bumps for breaks" rule
applies.

## 2. What counts as a breaking change

The **public contract** is everything a Lua script can observe through
the bindings. Specifically:

1. **Public Lua API surface** - every usertype, property, method,
   metamethod, and free function registered into the Lua state by
   `RegisterAllBindings` and its callees. Metatable names
   (`BinaryNinja.*`) are part of the contract because `lua-api/*.lua`
   extensions look them up via `debug.getregistry()`.
2. **Vocabulary strings** - the canonical Lua-visible strings returned
   by the R2 enum module (for example `"little"` for
   `Architecture.endianness`, or `"direct"` for a type reference).
   Changing the canonical string is a break. Adding a new dual-accept
   alias to `EnumFromString` is NOT a break.
3. **Return shape of methods that return Lua tables** - the table
   keys (e.g. `{full_width_reg, size, offset, extend, index}` on a
   RegisterInfo) are part of the contract. Adding a key is a PATCH
   bump; renaming or removing one is a break.
4. **`binjalua` globals** (see section 3) - anything in the `binjalua`
   table is part of the contract.
5. **Lua-side helpers under `lua-api/`** - `collections.*`, `fluent.*`,
   `utils.*`. Same rules.

Things that are explicitly NOT part of the contract:

- **C++ internals.** `bindings/common.h` helper signatures,
  `MetadataCodec` internals, `ToLuaTable` overload set, `HexAddress`
  member layout, sol2 trait specializations, and every `Register*`
  free function are private. Refactoring them freely is fine.
- **Binding file organisation.** Moving a usertype registration from
  `common.cpp` into its own `bindings/xyz.cpp` (as R1 did for Section,
  Symbol, Selection) is not a break because the Lua-visible surface
  is unchanged.
- **Metadata key names for internal planning documents.**
  `docs/extension-plan.md`, `docs/binding-audit.md`,
  `docs/python-api-gaps.md`, `docs/il-metatable-design.md`,
  `docs/callback-bindings-sketch.md` are local-only (excluded via
  `.git/info/exclude`) and are not part of the contract.
- **Build system choices.** CMake variables, compile definitions,
  target names, and the `BINDING_SOURCES` list are internal.
- **Logging messages.** `Logger::LogDebug` strings are not stable.
- **sol2 version bumps.** Upgrading sol2 is a PATCH if no Lua
  observable behavior changes, a MINOR if it does.

### 2.1 Changelog discipline

**Every commit that changes the public Lua-visible surface MUST
include a matching entry in `CHANGELOG.md` in the same commit.** Not
a separate follow-up commit, not "I will add it at release time" -
same commit. The enforcement mechanism is review: if a PR touches
`bindings/*.cpp`, `lua-api/*.lua`, `common.h` metatable constants,
or any Lua-exposed enum vocabulary without a CHANGELOG diff, it
should be sent back for a changelog entry before merging.

Internal refactors (C++ helper reshuffles, build system changes,
logging adjustments, test-only edits) do NOT need a CHANGELOG
entry. If you are unsure whether a change is public-surface, the
safe default is to add a CHANGELOG entry - the cost of a spurious
entry is one line of noise; the cost of a missing entry is a user
discovering a surprise break.

Entries land under the `## [Unreleased]` section at the top of
`CHANGELOG.md`. At release cut time (see section 4), the
`## [Unreleased]` heading is renamed to the new version with the
release date, and a fresh empty `## [Unreleased]` section is
added above it.

### 2.2 Deprecation policy

During the 0.y.z series there is **no formal deprecation window**.
Breaking changes land immediately when the architect and the
implementing contributor agree the break is correct; the MINOR bump
and the CHANGELOG "BREAKING" entry are the entire warning mechanism.
A "Changed - BREAKING" bullet in the CHANGELOG plus a migration-note
row in `docs/api-reference.md` is the minimum documentation for every
break. If a break is load-bearing (for example an R4 property rename
that every existing Lua script uses), the CHANGELOG entry should
include the old -> new mapping inline rather than only pointing at
`docs/api-reference.md`.

This policy will tighten at 1.0.0. From 1.0.0 onward, breaks require
a MAJOR bump, and the project will consider adding a one-MINOR
deprecation window where the old name continues to work with a Lua
warning. That is 1.0.0's problem, not 0.y.z's - the current approach
is "call it what it is, mark it in the CHANGELOG, move on" and
scripts adapt at the next MINOR bump.

## 3. Version source of truth

The canonical version number lives in exactly one place: the
`project(...)` call at the top of `CMakeLists.txt`. Everything else
derives from it.

### 3.1 CMake

```cmake
project(BinjaLua
    VERSION 0.2.0
    LANGUAGES C CXX)
```

CMake populates `PROJECT_VERSION_MAJOR`, `PROJECT_VERSION_MINOR`,
`PROJECT_VERSION_PATCH`, and `PROJECT_VERSION` automatically from the
`VERSION` argument. These variables are passed to the plugin target as
compile definitions:

```cmake
target_compile_definitions(${PROJECT_NAME} PRIVATE
    BINJALUA_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    BINJALUA_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    BINJALUA_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    BINJALUA_VERSION_STRING="${PROJECT_VERSION}")
```

The `BINJALUA_VERSION_STRING` definition is the human-readable
`"MAJOR.MINOR.PATCH"` form; the three numeric macros allow
preprocessor comparisons if they are ever needed. No CMake variable
other than the `project(VERSION ...)` argument is a source of truth.

### 3.2 C++

A small header `bindings/version.h` (or inline constants at the top
of `library.cpp` - cpp-lead's call) exposes the version to C++ code:

```cpp
namespace BinjaLua {
constexpr int kVersionMajor = BINJALUA_VERSION_MAJOR;
constexpr int kVersionMinor = BINJALUA_VERSION_MINOR;
constexpr int kVersionPatch = BINJALUA_VERSION_PATCH;
constexpr std::string_view kVersionString{BINJALUA_VERSION_STRING};
}  // namespace BinjaLua
```

The plugin logs its version at load time through the existing Binary
Ninja `Logger`:

```cpp
logger->LogInfo("binja-lua %.*s loaded",
    static_cast<int>(BinjaLua::kVersionString.size()),
    BinjaLua::kVersionString.data());
```

This gives users a one-line way to confirm which version of the plugin
is actually running when they file a bug report.

### 3.3 Lua global

The Lua side gains a **new namespace table** named `binjalua` populated
during `RegisterGlobalFunctions` in `bindings/common.cpp`. The
spelling decision is:

```lua
binjalua = {
    version = "0.2.0",  -- the full MAJOR.MINOR.PATCH string
    version_major = 0,
    version_minor = 2,
    version_patch = 0,
}
```

**Rationale for `binjalua` over `binja`, `BINJALUA_VERSION`, or
`_BINJA_VERSION`:**

- `binjalua` matches the C++ `BinjaLua::` namespace exactly, which is
  the canonical spelling across the codebase.
- Using a namespace table rather than a flat uppercase constant gives
  future Lua-visible plugin globals a natural home (e.g. `binjalua.
  plugin_path` someday) without polluting the global scope further.
- `binja` was rejected because it is ambiguous - scripts could
  reasonably expect `binja` to be the BN Python-style module namespace
  containing `BinaryView`, `Function`, etc. The plugin does not
  currently wrap BN types under such a namespace, and claiming the
  name here would make any future refactor toward a Python-style
  namespace harder.
- `BINJALUA_VERSION` (flat uppercase constant) was rejected because
  Lua idiom reserves ALL_CAPS globals for true constants set at
  program start, and a version number IS a constant, but the
  namespace-table form gives scripts a stable place to grow
  auxiliary version metadata without adding more top-level globals.
- `_BINJA_VERSION` (underscore-prefixed) was rejected because
  underscore-prefix globals in Lua traditionally signal "private to
  the runtime" (compare `_G`, `_VERSION`, `_ENV`). A plugin-author
  global should not look like a runtime internal.

**Version compatibility example.** A Lua script that depends on R6
Platform bindings (which landed in `v0.2.0`) can gate itself:

```lua
-- minimum version check
local want_major, want_minor = 0, 2
if binjalua.version_major < want_major
   or (binjalua.version_major == want_major
       and binjalua.version_minor < want_minor) then
    error("this script needs binja-lua " .. want_major .. "."
          .. want_minor .. "+, running " .. binjalua.version)
end
```

Until 1.0.0 ships, scripts should pin to an exact MINOR because MINOR
is the break marker in the 0.y.z series. After 1.0.0, scripts can
pin to MAJOR and treat MINOR as additive-only.

### 3.4 Runtime vs build-time

Both the C++ constant `BinjaLua::kVersionString` and the Lua global
`binjalua.version` are **build-time constants**. They reflect the
version the plugin was compiled against, NOT a runtime-negotiated
value. There is no wire protocol to speak of, so a mismatch between
the two can only happen if someone hand-edits a compile definition,
which is out of scope.

## 4. Release process

No release automation. The process is manual and deliberately simple.

1. Decide what to release (usually "everything on `main` since the
   last tag" but occasionally a curated cut).
2. Update `CMakeLists.txt` project VERSION to the new number.
3. Update `CHANGELOG.md`:
   - Rename the `## [Unreleased]` section to the new version with
     today's date in `YYYY-MM-DD` format.
   - Add a fresh empty `## [Unreleased]` section above it.
4. Build clean Windows Release and run the validation harness.
   `cmake --build build --config Release` green plus
   `examples/validation/run_all.lua` all-green is the acceptance
   bar.
5. Commit with a message of the form `chore: release vX.Y.Z`.
6. Tag the release commit as `vX.Y.Z` (note the lowercase `v`).
7. Push the commit and the tag **only after the user has given
   explicit go-ahead for that specific release**. Tag pushing is a
   destructive / visible action and is never pre-authorised by
   standing policy.

### 4.1 Tag format

`vMAJOR.MINOR.PATCH` with a lowercase `v` prefix. The `v` is standard
git tag convention and matches what GitHub's release UI expects. Do
NOT use `V` (uppercase) or the bare number without a prefix.

### 4.2 Cadence

On-demand. No calendar cadence. A release is cut when:

- A feature wave (R7, R8, R9, etc.) has landed and is validated.
- A critical bug fix needs shipping.
- The user explicitly requests a cut.

Routine commits between waves do NOT trigger a release.

### 4.3 Who bumps the version

Only the human user or a teammate explicitly authorised by the human
user. Agents (including cpp-lead-2 and architect) do NOT autonomously
bump the version or cut releases. Agents may PROPOSE a release and
prepare a dry-run branch, but the actual tag commit requires human
sign-off.

## 5. Versions 0.1.0, 0.1.1, 0.2.0

The project adopts semver retroactively across three version tags.
The first two are retroactive labels for commits that already
landed; the third is the forward-looking cut that captures every
Lua-visible change since then. See `CHANGELOG.md` at the repo root
for the per-version entry lists; this section provides the
narrative context.

### 5.1 v0.1.0 - initial public release (retroactive)

Tag target: commit `6b945ea` ("Initial public release, here we
go"). The original public release of `binja-lua`, cut before the
project adopted semver practice. Because the CHANGELOG discipline
described in section 2.1 post-dates this commit, the per-version
entry in `CHANGELOG.md` is a deliberate stub ("Initial public
release") rather than a full enumeration - anyone wanting the full
contents should read the `6b945ea` commit itself and the README as
they stood at that time. This tag exists primarily so that the
compare-range graph between `v0.1.1` and `v0.2.0` has a valid base
to anchor against.

### 5.2 v0.1.1 - submodule bump (retroactive)

Tag target: commit `87e8935` ("Update binaryninja-api to
stable/5.3.9434"). A single submodule pointer move on top of the
0.1.0 baseline. PATCH bump because the change is strictly
non-breaking from the perspective of this project's public
contract: the Lua API surface is identical, the metatable names
are identical, and the canonical enum vocabulary is identical.
Only the upstream `binaryninja-api` reference moved. Scripts
written against `v0.1.0` will run unchanged against `v0.1.1`.

### 5.3 v0.2.0 - first semver-discipline release (forward-looking)

Tag target: the first commit that carries the semver version
source-of-truth plumbing (task #23 in the working task queue).
MINOR bump rather than PATCH because the wave carries breaking
changes under the pre-1.0 rule in section 1.1. The full list is
in `CHANGELOG.md`; at a glance this version contains:

- **R1-R6 landed and validated:** Stub split (`9a00ea6`), R2
  shared helpers (ToLuaTable, EnumToString/FromString,
  AsAddress, MetadataCodec), R3a-R3d shallow-binding fixups plus
  alias removal, R4 Architecture read-only, R5 CallingConvention
  read-only, R6 Platform read-only.
- **sol2 3.3.0 MSVC crash pattern purged** across R4/R5/R6 files
  (tasks #12, #14, #15, #16).
- **MetadataCodec int64 and embedded-null fixes** (task #13
  landed).
- **`Function.arch` / `BinaryView.arch` / `BasicBlock.arch` /
  `InstructionWrapper.arch` type swap** from `std::string` to
  `Ref<Architecture>` (task #11 and the legacy retrofit #20).
- **Task #17 roadmap re-sequencing** and **task #18 legacy
  retrofit plan** authored in `docs/extension-plan.md`.
- **R14 Workflow read path struck from the active roadmap** per
  user direction 2026-04-15.
- **Legacy retrofit commits (task #20 basicblock/instruction
  arch, task #21 three-enum routing)** land as part of 0.2.0 -
  they are small and belong in the same MINOR cut rather than in
  an isolated 0.1.x pre-tag series.
- **Semver plumbing itself (task #23):** `CMakeLists.txt`
  `project(VERSION 0.2.0)`, `BinjaLua::kVersion*` C++ constants,
  `binjalua.version` Lua global table, plugin-load INFO log line.
- **R7 Symbol + Tag CRUD** may or may not land in 0.2.0
  depending on timing. If R7 ships before the cut it is part of
  0.2.0; if it slips to a later commit it becomes the first item
  in 0.3.0 and is the first post-baseline feature release.

**Why 0.2.0 rather than 0.1.2:** the wave contains breaking
changes. R3d dropped several long-form property aliases (`start`,
`end`, `auto`, `type`, `file`), R4/R5/R6 converted dozens of
`sol::property` accessors to method call syntax, the arch property
type swap is a hard type change, and several enum-vocabulary
strings moved (e.g. `Variable:location().type` "stack" -> "local").
Under the section 1.1 rule a 0.y.z release that contains ANY
breaking change is a MINOR bump, so 0.1.1 -> 0.2.0 skips 0.1.2
entirely.

**Migration from v0.1.0 or v0.1.1:** `docs/api-reference.md`
contains per-break migration rows for every Lua-visible change in
this wave. Scripts written against the initial public release will
need updates; most are mechanical renames of the
`obj.arch` -> `obj.arch.name` form plus method-call conversions of
the R4/R5/R6 accessors.

## 6. What this policy does NOT commit to

To set expectations honestly:

- **No backport branches.** Fixes land on `main` and ship in the next
  release. There is no `0.1.x` maintenance branch.
- **No deprecation period guarantees.** Breaking changes in the 0.y.z
  series may land without a prior deprecation window; the MINOR bump
  and the CHANGELOG entry are the entire warning mechanism. Starting
  with 1.0.0 this policy will tighten; see section 1.2.
- **No SLA on bug fix turnaround.** This is a hobby plugin; fixes
  land when they land.
- **No binary release artifacts yet.** The only distribution
  mechanism today is "clone the repo and build it." A CI-built
  prebuilt DLL is out of scope for 0.2.0 and will be revisited
  separately if and when a user asks for one.
- **No migration tooling.** The migration notes in
  `docs/api-reference.md` are hand-written prose, not executable
  codemods. Lua scripts have to be updated by hand when a break
  lands.

## 7. Open questions and future revisits

- **Tag signing.** Unsigned tags today. Revisit when there is a
  reason to care (external distribution, trust chain, reproducible
  builds). Until then, plain annotated tags are fine.
- **Separate versioning for `lua-api/*.lua`?** Currently bundled
  with the DLL version. If `lua-api/` ever ships independently we
  will need a second version channel; not in scope for 0.2.0.
- **Runtime compatibility shims.** If a future release wants to
  support old scripts gracefully, `binjalua.version` plus a small
  compat shim layer is the natural place. Not in scope for 0.2.0;
  the break-budget + CHANGELOG approach is sufficient for now.
