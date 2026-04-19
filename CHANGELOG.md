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

- **R9.3: `HLILInstruction` value-usertype + HLIL operand walking
  + tree navigation** (extends `bindings/il_operand_conv.cpp`,
  `bindings/il.h`, `bindings/il_operands_table.inc`, and
  `bindings/il.cpp`). Introduces `BinaryNinja.HLILInstruction` as
  the third non-`Ref<T>` value-semantics usertype in the plugin
  after `LLILInstruction` and `MLILInstruction`. Mirrors the
  Python `HighLevelILInstruction` surface at
  `python/highlevelil.py:143`.
  `HLILFunction:instruction_at(i)` now returns a live instruction
  usertype (was a `{index = i}` stub). New companion accessors on
  the HighLevelILFunction side: `HLILFunction:root()` (wraps the
  C++ `GetRootExpr()`; Python does not expose this but C++ does
  and it is useful as a tree entry point) and
  `HLILFunction:get_expr(expr_idx, as_ast = true)` (flat
  expression lookup, parallel to Python
  `HighLevelILFunction.get_expr` at
  `python/highlevelil.py:2940`). Per-instruction properties:
  `address` (HexAddress), `size`, `expr_index`, `source_operand`,
  `attributes`, `operation` (short canonical string, dual-accept
  via `EnumFromString<BNHighLevelILOperation>` for the 126
  enumerators; commit `e13a340` landed the enum table),
  `il_function` (`Ref<HighLevelILFunction>`), `text`. HLIL-unique
  AST-context properties: `as_ast` (boolean), `ast`, `non_ast`.
  SSA cross-form: `ssa_form`, `non_ssa_form`, `ssa_expr_index`.
  HLIL-unique tree navigation: `parent` (returns
  `Ref<HLILInstruction>` or `nil` for root/detached expressions
  via `std::optional` marshalling, never using
  `sol::property + sol::this_state`), `:children()` (flattened
  union of operand slots tagged `"expr"` or `"expr_list"` in
  operand order), `:ancestors()` (iterates via repeated
  `GetParent()` dereferences, capped at 4096 steps defensively).
  Projection helpers: `operands` (1-indexed value list),
  `detailed_operands` (1-indexed `{name, value, type}` tables),
  `prefix_operands` (prefix-order flattened walk),
  `traverse(cb)` (depth-first pre-order collector). HLIL -> MLIL
  cross-references: `.mlil` (scalar, `Optional`-marshalled) and
  `:mlils()` (multi-result list from
  `GetMediumLevelILExprIndexes`); no direct `.llil` is exposed
  per Python's two-hop policy at
  `python/mediumlevelil.py:697`. Metamethods: `__eq` on
  `(function, expr_index)`, `__tostring` as
  `"<HLILInstruction OP @0x...>"`. Operand type-tag vocabulary
  matches `docs/il-metatable-design.md` section 13.3: shared
  core (`int`, `float`, `expr`, `expr_list`, `int_list`,
  `intrinsic`), shared with MLIL (`var`, `var_ssa`,
  `var_ssa_list`, `ConstantData`), plus HLIL-unique `label`
  (`{label_id, name}` with name eagerly resolved via
  `Function::GetGotoLabelName`) and `member_index` (int64 or
  `nil` when the high-bit sentinel is set). No `target_map` /
  `cond` / register-level tags (HLIL drops these). 114 of the
  126 HLIL opcodes carry non-empty operand specs; the 12
  empty-spec opcodes are `HLIL_NOP`, `HLIL_BREAK`,
  `HLIL_CONTINUE`, `HLIL_NORET`, `HLIL_UNREACHABLE`, `HLIL_BP`,
  `HLIL_UNDEF`, `HLIL_UNIMPL`, `HLIL_ASSERT`, `HLIL_ASSERT_SSA`,
  `HLIL_FORCE_VER`, `HLIL_FORCE_VER_SSA`. HLIL opcode short forms
  include several Lua reserved words as STRING values (`"if"`,
  `"while"`, `"for"`, `"do"`, `"return"`, `"break"`, `"goto"`,
  `"continue"`): safe because they are string values, never
  identifiers. Generator at `scripts/generate_il_tables.py`
  extended to walk `python/highlevelil.py::ILOperations` +
  per-subclass `detailed_operands`. Validation coverage in
  `examples/validation/15_hlil.lua` (suite: 15 scripts), with
  tree-navigation assertions as the centerpiece
  (root/parent/children round-trip, ancestor-walk termination,
  `HLIL_IF` / `HLIL_WHILE` / `HLIL_BLOCK` / `HLIL_ASSIGN` /
  `HLIL_CALL` / `HLIL_VAR_PHI` / `HLIL_GOTO` /
  `HLIL_STRUCT_FIELD` probe coverage, HLIL -> MLIL
  cross-references).
- **R9.2: `MLILInstruction` value-usertype + MLIL operand walking**
  (extends `bindings/il_operand_conv.cpp`, `bindings/il.h`,
  `bindings/il_operands_table.inc`; `bindings/il.cpp` grows
  `RegisterMLILInstructionBindings`). Introduces
  `BinaryNinja.MLILInstruction` as the second non-`Ref<T>`
  value-semantics usertype in the plugin after
  `LLILInstruction`. Mirrors the Python
  `MediumLevelILInstruction` surface at
  `python/mediumlevelil.py:192`.
  `MLILFunction:instruction_at(i)` now returns a live instruction
  usertype (was a `{index = i}` stub). Per-instruction properties:
  `address` (HexAddress), `size`, `expr_index`, `instr_index`,
  `source_operand`, `operation` (short canonical string,
  dual-accept via `EnumFromString<BNMediumLevelILOperation>` for
  the 140 enumerators; commit `30bae89` landed the enum table),
  `il_function` (`Ref<MediumLevelILFunction>`; named
  `il_function` rather than `function` because `function` is a
  Lua reserved word - same rule as LLIL), `attributes`,
  `ssa_form` / `non_ssa_form` / `ssa_instr_index` /
  `ssa_expr_index`, `text`. Projection helpers: `operands`
  (1-indexed value list), `detailed_operands` (1-indexed
  `{name, value, type}` tables), `prefix_operands`
  (prefix-order flattened walk with `{operation, size}` marker
  tables), `traverse(cb)` (depth-first pre-order collector).
  Metamethods: `__eq` on `(function, expr_index)`, `__tostring`
  as `"<MLILInstruction OP @0x...>"`. Operand type-tag vocabulary
  matches `docs/il-metatable-design.md` section 12.3: shared
  with LLIL (`int`, `float`, `expr`, `expr_list`, `int_list`,
  `target_map`, `intrinsic`, `cond`) plus MLIL-specific tags
  `var` (`{source_type, index, storage}` table reusing the
  R2.1 `BNVariableSourceType` vocabulary), `var_ssa`
  (`{var, version}` table), `var_list` (array of var tables),
  `var_ssa_list` (array of var_ssa tables),
  `var_ssa_dest_and_src` (partial SSA variable source tuple),
  and `ConstantData` (CamelCase per Python's `ConstantDataType`,
  `{state, value, size}` table with `state` drawn from the new
  `EnumToString<BNRegisterValueType>` vocabulary). MLIL drops
  all register-level tags since MLIL operates on typed variables
  rather than raw registers. Generator at
  `scripts/generate_il_tables.py` extended to walk
  `python/mediumlevelil.py::ILOperations` + per-subclass
  `detailed_operands`; 131 of the 140 MLIL opcodes carry
  non-empty operand specs (the 9 empty-spec opcodes are
  `MLIL_NOP`, `MLIL_NORET`, `MLIL_BP`, `MLIL_UNDEF`,
  `MLIL_UNIMPL`, `MLIL_ASSERT`, `MLIL_ASSERT_SSA`,
  `MLIL_FORCE_VER`, `MLIL_FORCE_VER_SSA`). Validation coverage
  in `examples/validation/14_mlil.lua`.
- **`EnumToString` / `EnumFromString<BNRegisterValueType>`.**
  New dual-accept enum helpers in `bindings/common.h` covering
  all 17 `BNRegisterValueType` enumerators including the
  `0x8000`-bit `ConstantData*` variants (`constant_data`,
  `constant_data_zero_extend`, `constant_data_sign_extend`,
  `constant_data_aggregate`). Short form follows the R2.1
  mechanical rule (strip `Value` suffix, snake_case). Used by
  the MLIL `ConstantData` operand projection but generally
  applicable to any dataflow-state stringification.

### Changed

- _nothing yet_

### Fixed

- _nothing yet_

### Removed

- _nothing yet_

## [0.3.0] - unreleased

Second release under the semver practice introduced in 0.2.0. Ships
the R8 Settings binding, the R9.1 LLIL operand-walking wave, a C++20
toolchain bump driven by BN header requirements, and a handful of
documentation and helper additions. Pre-1.0 semver interprets 0.y.z
MINOR bumps as "contains breaking changes" — `LLILFunction:instruction_at`
now returns a live `LLILInstruction` usertype instead of the prior
`{index = i}` table stub, so any script that reached into that stub
will need to migrate to the new surface documented in
`docs/api-reference.md`.

### Added

- **R9.1: `Function.llil` / `.mlil` / `.hlil` properties.** New
  sol::property accessors on `BinaryNinja.Function` mirroring the
  Python `Function.llil` / `.mlil` / `.hlil` surface at
  `python/function.py:1000` / `:1068` / `:1148`. Return
  `Ref<LowLevelILFunction>` / `MediumLevelILFunction` /
  `HighLevelILFunction` or `nil` when analysis has not produced the
  corresponding IL. The existing `f:get_llil()` / `get_mlil()` /
  `get_hlil()` method forms are retained; Python exposes both and
  scripts written against the R8 pre-R9 surface keep working.
- **R9.1: `LLILInstruction` value-usertype + operand walking**
  (new files `bindings/il_operand_conv.cpp`, `bindings/il.h`;
  generated `bindings/il_enums.inc` and
  `bindings/il_operands_table.inc`; `bindings/il.cpp` grows
  `RegisterLLILInstructionBindings`). Introduces
  `BinaryNinja.LLILInstruction` as the FIRST non-`Ref<T>`
  value-semantics usertype in the plugin. Mirrors the Python
  `LowLevelILInstruction` surface at `python/lowlevelil.py:316`.
  `LLILFunction:instruction_at(i)` now returns a live instruction
  usertype (was a `{index = i}` stub). Per-instruction properties:
  `address` (HexAddress), `size`, `expr_index`, `instr_index`,
  `source_operand`, `operation` (short canonical string, dual-accept
  via `EnumFromString<BNLowLevelILOperation>` for the 143
  enumerators), `il_function` (`Ref<LowLevelILFunction>`; named
  `il_function` rather than `function` because `function` is a Lua
  reserved word - same rule as the R3d `start_addr` / `end_addr`
  renames), `flags`,
  `attributes`, `ssa_form` / `non_ssa_form` / `ssa_instr_index` /
  `ssa_expr_index`, `has_mlil` / `has_mapped_mlil`, `text`.
  Projection helpers: `operands` (1-indexed value list),
  `detailed_operands` (1-indexed `{name, value, type}` tables),
  `prefix_operands` (prefix-order flattened walk with
  `{operation, size}` marker tables), `traverse(cb)` (depth-first
  pre-order collector). Metamethods: `__eq` on
  `(function, expr_index)`, `__tostring` as
  `"<LLILInstruction OP @0x...>"`. Operand type-tag vocabulary
  matches `docs/il-metatable-design.md` section 2c: `int`, `float`,
  `expr`, `expr_list`, `int_list`, `reg`, `flag`, `reg_stack`,
  `sem_class`, `sem_group`, `intrinsic`, `cond`, `target_map`,
  `reg_stack_adjust`, `reg_ssa`, `reg_stack_ssa`,
  `reg_stack_ssa_dest_and_src`, `flag_ssa`, `reg_ssa_list`,
  `reg_stack_ssa_list`, `flag_ssa_list`, `reg_or_flag_list` and
  `reg_or_flag_ssa_list` (discriminated `{kind, name}` entries),
  `constraint` (R9.1 stub). `BNLowLevelILFlagCondition` (22
  enumerators: 14 integer + 8 floating-point) round-trips via short
  canonical strings (`"e"`, `"slt"`, `"fuo"`, ...). Sentinel register
  id `0xffffffff` maps to nil on the Lua side. Generator at
  `scripts/generate_il_tables.py` reads `binaryninjacore.h` +
  `python/lowlevelil.py::ILOperations` to emit the enum vocabulary
  and per-opcode operand spec dispatch; regeneration is manual on
  each `binaryninja-api` submodule bump. Validation coverage in
  `examples/validation/13_llil.lua`.
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

[Unreleased]: https://github.com/alecnunn/binja-lua/compare/v0.3.0...HEAD
[0.3.0]: https://github.com/alecnunn/binja-lua/compare/v0.2.0...v0.3.0
[0.2.0]: https://github.com/alecnunn/binja-lua/compare/v0.1.1...v0.2.0
[0.1.1]: https://github.com/alecnunn/binja-lua/compare/v0.1.0...v0.1.1
[0.1.0]: https://github.com/alecnunn/binja-lua/releases/tag/v0.1.0
