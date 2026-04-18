# Changelog

All notable changes to `binja-lua` are documented in this file.

The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project
follows [Semantic Versioning](https://semver.org/) with the pre-1.0
interpretation documented in [`docs/versioning.md`](docs/versioning.md):
during the 0.y.z series a MINOR bump signals a breaking change and a
PATCH bump signals additive-only changes.

## [Unreleased]

### Added

- **R8: `Settings` binding (new file, `bindings/settings.cpp`).**
  Introduces the `BinaryNinja.Settings` usertype and the `Settings.new`
  static factory wrapping `Settings::Instance` at
  `binaryninja-api/binaryninjaapi.h:19286`. Mirrors the Python
  `binaryninja.settings.Settings` surface at `python/settings.py:31`
  with one method per scalar kind: `get_bool` / `get_double` /
  `get_integer` / `get_string` / `get_string_list` / `get_json` (plus
  matching `*_with_scope` variants that return `(value, scope_string)`
  tuples) and `set_bool` / `set_double` / `set_integer` / `set_string`
  / `set_string_list` / `set_json`. Every accessor takes an optional
  `resource` argument that can be a `BinaryView`, a `Function`, or
  nil, and an optional scope argument that dual-accepts the short
  canonical form (`"default"`, `"user"`, `"project"`, `"resource"`,
  `"auto"`, `"invalid"`) OR the verbatim `BNSettingsScope` enumerator
  name (e.g. `"SettingsUserScope"`). Schema-side: `contains`, `keys`,
  `is_empty`, `query_property_string`, `query_property_string_list`,
  `register_group`, `register_setting`, `update_property`,
  `deserialize_schema`, `serialize_schema`. Persistence:
  `serialize_settings`, `deserialize_settings`, `reset`, `reset_all`,
  `load_settings_file`, `set_resource_id`. Adds `SETTINGS_METATABLE`
  in `bindings/common.h` and `EnumToString` /
  `EnumFromString<BNSettingsScope>` specializations alongside the
  existing R2 enum helpers. Validation coverage in
  `examples/validation/12_settings.lua` (suite: 12 scripts).

### Changed

- _nothing yet_

### Fixed

- _nothing yet_

### Removed

- _nothing yet_

## [0.2.0] - unreleased

First release under the new semver practice. Captures every
Lua-visible change that landed on `main` between `9a00ea6`
("bindings: split section/symbol/selection into their own files")
and the 0.2.0 cut. Because the pre-1.0 rule interprets 0.y.z MINOR
bumps as "contains breaking changes" (see
[`docs/versioning.md`](docs/versioning.md) section 1.1), this is a
MINOR bump rather than a PATCH: the wave carries the R3d alias
removals, the R4/R5/R6 method-syntax conversions, the
`Function.arch` / `BinaryView.arch` / `BasicBlock.arch` /
`InstructionWrapper.arch` type swaps, and several enum-vocabulary
corrections. Scripts written against `v0.1.0` or `v0.1.1` must be
updated when moving to `v0.2.0`; the migration rows in
`docs/api-reference.md` are the authoritative per-break guide.

### Added

- **Version source of truth plumbing.** `CMakeLists.txt` now declares
  `project(BinjaLua VERSION 0.2.0)` and propagates the version to C++
  via `BINJALUA_VERSION_MAJOR` / `MINOR` / `PATCH` / `STRING` compile
  definitions. C++ code accesses the version via
  `BinjaLua::kVersionString` (and the matching numeric constants).
  The plugin logs its version at load time through the existing
  Binary Ninja `Logger`.
- **`binjalua` Lua namespace table.** New global table set during
  `RegisterGlobalFunctions` exposing `binjalua.version` (the full
  `"MAJOR.MINOR.PATCH"` string), `binjalua.version_major`,
  `binjalua.version_minor`, and `binjalua.version_patch`. Lua
  scripts can use this for compatibility gating; see
  `docs/versioning.md` section 3.3 for the recommended pattern.
- **R1: Stub files split out of `bindings/common.cpp`.** Section,
  Symbol, and Selection usertypes now live in their own
  `bindings/section.cpp`, `bindings/symbol.cpp`,
  `bindings/selection.cpp` files. The file-per-class rule has no
  standing exceptions.
- **R2: Shared binding helpers.** `ToLuaTable` for uniform
  collection-to-table projection, `AsAddress` for accepting
  HexAddress / uint64 / int64 / double interchangeably,
  `EnumToString` and `EnumFromString` helpers with dual-accept
  vocabulary covering `BNBranchType`, `BNSymbolType`,
  `BNSectionSemantics`, `BNTypeClass`, `BNAnalysisState`,
  `BNAnalysisSkipReason`, `BNMetadataType`, `BNMemberAccess`,
  `BNSymbolBinding`, `BNEndianness`, `BNImplicitRegisterExtend`,
  `BNFlagRole`, `ReferenceSourceToTable`, and a shared
  `MetadataCodec` replacing the copy-paste between `Function` and
  `BinaryView`.
- **R3a: `Section` binding fleshed out.** Adds
  `GetLinkedSection`, `GetInfoSection`, `GetAlign`, `GetEntrySize`,
  `GetAutoDefined`. Replaces the inline permission inference with
  the direct `BNSectionSemantics` surface.
- **R3b: `Symbol` binding fleshed out.** Adds `GetRawName`,
  `GetAliases`, namespace accessors, and `BinaryView:
  DefineUserSymbol` / `DefineAutoSymbol` / `UndefineUserSymbol`.
- **R3c: Flow graph edge styling and branch type enum dedup.**
  `FlowGraph:add_outgoing_edge` now takes an optional style argument
  instead of hardcoding `SolidLine` / white.
- **R4: `Architecture` read-only binding (new file,
  `bindings/architecture.cpp`).** Introduces the
  `BinaryNinja.Architecture` usertype with `name`, `endianness`
  (string via R2 enum helper), `address_size`, `default_int_size`,
  `instr_alignment`, `max_instr_length`, `opcode_display_length`,
  `stack_pointer`, `link_reg` (with `0xffffffff` -> nil sentinel
  rule), `can_assemble`, `regs` (dict keyed by name), `full_width_regs`,
  `global_regs`, `system_regs`, `flags`, `flag_write_types`,
  `semantic_flag_classes`, `semantic_flag_groups`, `flag_roles`,
  `get_reg_index`, `get_reg_name`, `get_flag_role`,
  `get_instruction_info`, `get_instruction_text`,
  `get_associated_arch_by_address`, `get_by_name`, `list`, and
  `contains` statics.
- **R5: `CallingConvention` read-only binding (new file,
  `bindings/callingconvention.cpp`).** Introduces the
  `BinaryNinja.CallingConvention` usertype with `name`, `arch`,
  `confidence`, `caller_saved_regs`, `callee_saved_regs`,
  `int_arg_regs`, `float_arg_regs`, `required_arg_regs`,
  `required_clobbered_regs`, `implicitly_defined_regs`, the four
  single-register return properties with `0xffffffff` -> nil, the
  five heuristic-flag boolean properties (`arg_regs_share_index`,
  `arg_regs_for_varargs`, `stack_reserved_for_arg_regs`,
  `stack_adjusted_on_return`, `eligible_for_heuristics`), and
  `with_confidence`.
- **R6: `Platform` read-only binding (new file,
  `bindings/platform.cpp`).** Introduces the `BinaryNinja.Platform`
  usertype with `name`, `arch`, `address_size`, the five
  `*_calling_convention` properties plus `calling_conventions`,
  `global_regs`, `get_global_register_type`, `get_related_platform`,
  `related_platforms`, `get_associated_platform_by_address`, `types`,
  `variables`, `functions`, `system_calls`, `get_type_by_name`,
  `get_variable_by_name`, `get_function_by_name`,
  `get_system_call_name`, `get_system_call_type`, and the four
  explicit list statics (`get_by_name`, `list`, `list_by_arch`,
  `list_by_os`, `os_list`).
- **R6 follow-up: `platform` property on `Function` and
  `BinaryView`.** Both usertypes now expose a canonical
  `Ref<Platform>` property.
- **`MetadataCodec` int64 precision and embedded-null round-trip
  (task #13).** Metadata integers now route through `lua_isinteger`
  and preserve full 64-bit precision (previously capped at 2^52).
  Metadata strings now route through `BNCreateMetadataRawData` so
  strings containing embedded NUL bytes survive round-trip.
  Validation coverage in `examples/validation/07_metadata_roundtrip.lua`.
- **Legacy retrofit: `BasicBlock.arch` and `InstructionWrapper.arch`
  now return `Ref<Architecture>`** (task #20) - these were the last
  two files still returning `arch` as a string after R4's sweep of
  `Function` and `BinaryView`. See **Changed** for the breaking
  effect.
- **Legacy retrofit: four enum switches routed through the R2
  module** (task #21, task #24). `BNVariableSourceType`,
  `BNTypeReferenceType`, and `BNTagTypeType` now have dedicated
  dual-accept `EnumToString` / `EnumFromString` helpers instead
  of hand-rolled inline switches. (The fourth site is the second
  `BNVariableSourceType` switch inside `Variable:location()`
  that was missed in the first pass.)
- **R7: `Symbol.new` factory.** `Symbol` previously had
  `sol::no_constructor`, so Lua code had no way to build a
  `Ref<Symbol>` to feed into `BinaryView:define_user_symbol` /
  `define_auto_symbol`. The new factory mirrors
  `binaryninja.types.Symbol.__init__` at `python/types.py:421`:
  `Symbol.new(sym_type, addr, short_name[, full_name[, raw_name[,
  binding[, ordinal]]]])`. `sym_type` and `binding` both accept
  either the short canonical BN enum string or the verbatim
  Python/C enumerator name via the R2 dual-accept helpers.
  Namespace selection is intentionally not exposed because the
  `NameSpace` usertype is not bound.
- **R7: `BinaryView:create_auto_tag`.** New companion to the
  existing `create_user_tag` so scripts that drive analysis
  passes can produce auto-generated tags.
- **R7: `BinaryView:get_tag_type_by_id`.** Persistent lookup
  companion to `get_tag_type` (which looks up by name). Useful
  for tag references that should survive a rename.

### Changed

- **BREAKING: `Function.arch` now returns `Ref<Architecture>`
  (R4, task #11).** Previously a string of the architecture name.
  Scripts using `func.arch` as a string must update to
  `func.arch.name`. There is no compatibility alias per the R3d
  "no property aliases" ground rule. Migration note lives in
  `docs/api-reference.md`.
- **BREAKING: `BinaryView.arch` now returns `Ref<Architecture>`
  (R4, task #11).** Same migration as `Function.arch`. Use
  `bv.arch.name`.
- **BREAKING: `BasicBlock.arch` now returns `Ref<Architecture>`
  (task #20).** Previously a string. Use `bb.arch.name`.
- **BREAKING: `InstructionWrapper.arch` now returns
  `Ref<Architecture>` (task #20).** Previously a string. Use
  `instr.arch.name`.
- **BREAKING: Section permission model replaced by direct semantics
  (R3a).** The previous `permissions` property inferred
  `read/write/execute` booleans from `BNSectionSemantics`, which
  was lossy. The canonical field is now `section.semantics` as a
  string (via the R2 enum helper). Scripts that used the boolean
  inference must query `section.semantics` directly. Migration note
  in `docs/api-reference.md`.
- **BREAKING: Property aliases removed (R3d).** `start` -> `start_addr`,
  `end` -> `end_addr` (or `obj["end"]` for Lua-keyword fields),
  `auto` -> `auto_discovered`, `type` -> `type_name` on Variable,
  `file` -> `filename` on BinaryView. The canonical long forms are
  the only way to access these properties.
- **BREAKING: `Variable:location().type` returns R2 canonical
  strings (task #21 follow-up).** The `"stack"` string previously
  returned for `BNVariableSourceType::StackVariableSourceType` is
  replaced by `"local"` to match `Variable.source_type` and the
  shared `BNVariableSourceType` vocabulary. Scripts that branched
  on `loc.type == "stack"` must update to `loc.type == "local"`.
- **`EnumFromString` helpers dual-accept the canonical short form
  AND the verbatim Python/C enumerator name (R2.1).** For example
  `"little"` and `"LittleEndian"` both resolve to
  `BNEndianness::LittleEndian`. The short form is the canonical
  Lua-visible value returned by `EnumToString`.

### Fixed

- **sol2 3.3.0 + MSVC crash pattern purged from R4/R5/R6 files
  (tasks #12, #14, #15, #16).** The combination of `sol::property`
  wrapping a lambda whose parameter list captures `sol::this_state`
  crashes at registration time on sol2 3.3.0 / MSVC. All affected
  accessors in `bindings/architecture.cpp`,
  `bindings/callingconvention.cpp`, and `bindings/platform.cpp`
  were converted to bare-method form. Task #12 resolved Architecture,
  task #14 resolved CallingConvention, tasks #15/#16 resolved
  Platform and the follow-ups. Legacy pre-R4 files were audited
  (task #18) and found to be clean - they dodged the crash pattern
  by historical accident. See `docs/extension-plan.md` section 4.0
  for the audit result.
- **MetadataCodec: integer precision preserved to 2^63-1** (task
  #13). Previously the `lua_Number` -> double round-trip capped at
  2^52, silently corrupting large integer metadata values.
- **MetadataCodec: embedded-NUL strings preserved** (task #13).
  Previously `BNCreateMetadataStringData` truncated at the first
  NUL byte.
- **R3c: Duplicated `BranchTypeToString` removed** from
  `bindings/basicblock.cpp` and `bindings/flowgraph.cpp`. Both sites
  now route through the R2 `EnumToString<BNBranchType>` helper.

### Removed

- **R14 Workflow read path struck from the active roadmap.** Per
  user direction 2026-04-15, the read-only `Workflow` / `Activity`
  surface is permanently parked. Quote: "those are more suited for
  C++/Rust, not from scripts." The design spec remains in
  `docs/extension-plan.md` as historical context but is not
  scheduled. The write-side (`RegisterActivity`, custom `Activity`
  subclasses) stays parked alongside the broader callback phase.
- _No other public-surface removals._ The R3d alias removal is
  documented under **Changed** because it is a rename with a
  migration path, not a feature deletion.

### Documentation

- **`docs/versioning.md` (new, tracked).** Establishes the semver
  policy this release inaugurates.
- **`CHANGELOG.md` (new, tracked).** This file.
- **`docs/api-reference.md` updated for all R4/R5/R6 surface and
  the task #20 retrofit.** Migration notes for every breaking
  change live in the reference doc alongside the canonical API.

### Internal

- Roadmap re-sequencing (task #17), legacy retrofit plan (task #18),
  and value-axis analysis of R7 through R15 landed in
  `docs/extension-plan.md` (local-only). These planning documents
  are NOT tracked and are not user-visible; listed here only so the
  project history has a pointer to the decisions made during the
  0.2.0 preparation window.

## [0.1.1]

Retroactive tag. Captures the single submodule bump on top of the
0.1.0 baseline. PATCH bump because the change is non-breaking: no
Lua-visible API or vocabulary changed; only the upstream
`binaryninja-api` pointer moved.

### Changed

- Bump `binaryninja-api` submodule to `stable/5.3.9434` (commit
  `87e8935`).

## [0.1.0]

Retroactive tag for the initial public release (commit `6b945ea`,
"Initial public release, here we go"). The CHANGELOG discipline
described in [`docs/versioning.md`](docs/versioning.md) section 2.1
post-dates this tag, so the 0.1.0 entry is deliberately a stub;
anyone wanting the full contents of the initial release should
read the README and the 6b945ea commit itself. This entry exists
so that the [0.1.1] and [0.2.0] reference-link graph has a valid
base to compare against.

### Added

- Initial public release.

[Unreleased]: https://github.com/alecnunn/binja-lua/compare/v0.2.0...HEAD
[0.2.0]: https://github.com/alecnunn/binja-lua/compare/v0.1.1...v0.2.0
[0.1.1]: https://github.com/alecnunn/binja-lua/compare/v0.1.0...v0.1.1
[0.1.0]: https://github.com/alecnunn/binja-lua/releases/tag/v0.1.0
