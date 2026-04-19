#!/usr/bin/env python3
"""Generate bindings/il_enums.inc and bindings/il_operands_table.inc for
binja-lua.

Commit 1 scope (R9.1): il_enums.inc with EnumToString / EnumFromString
for BNLowLevelILOperation (143) and BNLowLevelILFlagCondition (22).

Commit 2a scope (R9.1): il_operands_table.inc with
LLILOperandSpecsForOperation driven by the per-subclass
detailed_operands overrides in python/lowlevelil.py. Approach ratified
by architect-2 as path 1; slot layout encodes {slot_first, slot_last}
to cover explicit-pair SSA tags (reg_ssa / flag_ssa / reg_stack_ssa)
alongside single-slot scalars and single-slot list accessors.

Run manually after each binaryninja-api submodule bump:

    python scripts/generate_il_tables.py

This script is tracked alongside its generated output but is NOT wired
into CMake. Build determinism is preserved by running the generator
out-of-band and diff-reviewing the resulting `.inc` files before
committing.
"""

from __future__ import annotations

import ast
import re
import sys
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Tuple

REPO_ROOT = Path(__file__).resolve().parent.parent
BN_HEADER = REPO_ROOT / "binaryninja-api" / "binaryninjacore.h"
PY_LLIL = REPO_ROOT / "binaryninja-api" / "python" / "lowlevelil.py"
PY_MLIL = REPO_ROOT / "binaryninja-api" / "python" / "mediumlevelil.py"
PY_HLIL = REPO_ROOT / "binaryninja-api" / "python" / "highlevelil.py"
ENUMS_OUT = REPO_ROOT / "bindings" / "il_enums.inc"
OPERANDS_OUT = REPO_ROOT / "bindings" / "il_operands_table.inc"

LLIL_OP_LINES = (586, 737)
LLFC_LINES = (739, 765)
# BNMediumLevelILOperation block verified by grep on
# binaryninjacore.h:1338-1488 (140 enumerators). Added for R9.2.
MLIL_OP_LINES = (1338, 1488)
# BNHighLevelILOperation block verified by grep on
# binaryninjacore.h:1521-1658 (126 enumerators). Added for R9.3.
HLIL_OP_LINES = (1521, 1658)

# Map Python _get_X accessor name -> (type_tag, arg_count). arg_count
# tells us how many slot indexes to consume: 1 for single-slot and
# single-slot-internally-paired list accessors (BN helper consumes the
# (size, operand_idx) pair internally via a single slot argument);
# 2 for explicit-pair SSA accessors. Shared between LLIL and MLIL.
# The MLIL-specific accessors (_get_var, _get_var_ssa, _get_var_list,
# _get_var_ssa_list, _get_var_ssa_dest_and_src, _get_constant_data)
# were added for R9.2 per docs/il-metatable-design.md section 12.3.
# _get_var_list and _get_expr_list on MediumLevelILInstruction accept
# TWO positional ints in Python but the second is ignored inside the
# body (see python/mediumlevelil.py:1092-1097); expose them with
# arity 1 so the reflection pass only walks slot_first.
ACCESSOR_TO_TAG: Dict[str, Tuple[str, int]] = {
    "_get_reg":                    ("reg",                    1),
    "_get_flag":                   ("flag",                   1),
    "_get_intrinsic":              ("intrinsic",              1),
    "_get_reg_stack":              ("reg_stack",              1),
    "_get_int":                    ("int",                    1),
    "_get_float":                  ("float",                  1),
    "_get_expr":                   ("expr",                   1),
    "_get_target_map":             ("target_map",             1),
    "_get_reg_stack_adjust":       ("reg_stack_adjust",       1),
    "_get_sem_class":              ("sem_class",              1),
    "_get_sem_group":              ("sem_group",              1),
    "_get_cond":                   ("cond",                   1),
    "_get_int_list":               ("int_list",               1),
    "_get_expr_list":              ("expr_list",              1),
    "_get_reg_or_flag_list":       ("reg_or_flag_list",       1),
    "_get_reg_ssa_list":           ("reg_ssa_list",           1),
    "_get_reg_stack_ssa_list":     ("reg_stack_ssa_list",     1),
    "_get_flag_ssa_list":          ("flag_ssa_list",          1),
    "_get_reg_or_flag_ssa_list":   ("reg_or_flag_ssa_list",   1),
    "_get_constraint":             ("constraint",             1),
    "_get_flag_ssa":               ("flag_ssa",               2),
    "_get_reg_ssa":                ("reg_ssa",                2),
    "_get_reg_stack_ssa":          ("reg_stack_ssa",          2),
    # MLIL-specific accessors.
    "_get_var":                    ("var",                    1),
    "_get_var_ssa":                ("var_ssa",                2),
    "_get_var_list":               ("var_list",               1),
    "_get_var_ssa_list":           ("var_ssa_list",           1),
    "_get_var_ssa_dest_and_src":   ("var_ssa_dest_and_src",   2),
    # ConstantData tag is CamelCase per Python's ConstantDataType rather
    # than the snake_case used by every other MLIL tag (see docs
    # section 12.3 gotcha); py-researcher-2 flagged this during the
    # R9.2 spec review.
    "_get_constant_data":          ("ConstantData",           2),
    # HLIL-specific accessors (R9.3 per docs section 13.3).
    # _get_label returns a GotoLabel struct; Lua projection emits
    # {label_id, name}. _get_member_index returns Optional[int];
    # high-bit sentinel maps to nil on the Lua side.
    "_get_label":                  ("label",                  1),
    "_get_member_index":           ("member_index",           1),
}


def extract_enumerators(path: Path, start: int, end: int,
                        prefix: str) -> List[str]:
    pattern = re.compile(rf"^\s*({re.escape(prefix)}[A-Z_0-9]+)")
    seen: set[str] = set()
    out: List[str] = []
    with path.open("r", encoding="utf-8") as f:
        for lineno, line in enumerate(f, 1):
            if lineno < start:
                continue
            if lineno > end:
                break
            m = pattern.match(line)
            if not m:
                continue
            name = m.group(1)
            if name in seen:
                continue
            seen.add(name)
            out.append(name)
    return out


def short_form(enumerator: str, prefix: str) -> str:
    assert enumerator.startswith(prefix), enumerator
    return enumerator[len(prefix):].lower()


def emit_enum_to_string(enum_type: str, values: Iterable[str],
                        prefix: str) -> str:
    lines: List[str] = []
    lines.append(f"const char* EnumToString({enum_type} v) {{")
    lines.append("    switch (v) {")
    for v in values:
        lines.append(f"        case {v}: return \"{short_form(v, prefix)}\";")
    lines.append("    }")
    lines.append("    return \"unknown\";")
    lines.append("}")
    return "\n".join(lines) + "\n"


def emit_enum_from_string(enum_type: str, values: Iterable[str],
                          prefix: str) -> str:
    lines: List[str] = []
    lines.append("template <>")
    lines.append(
        f"std::optional<{enum_type}> EnumFromString<{enum_type}>("
    )
    lines.append("    const std::string& s) {")
    for v in values:
        short = short_form(v, prefix)
        lines.append(
            f"    if (s == \"{short}\" || s == \"{v}\") return {v};"
        )
    lines.append("    return std::nullopt;")
    lines.append("}")
    return "\n".join(lines) + "\n"


ENUMS_HEADER = """\
// Auto-generated from binaryninja-api/binaryninjacore.h.
// Generator: scripts/generate_il_tables.py (tracked, run by hand).
// Do NOT hand-edit. Regenerate after bumping the BN submodule.
//
// Contents:
//   - EnumToString / EnumFromString for BNLowLevelILOperation
//     (143 enumerators from binaryninjacore.h:586-737).
//   - EnumToString / EnumFromString for BNLowLevelILFlagCondition
//     (22 enumerators from binaryninjacore.h:739-763: 14 integer +
//     8 floating-point).
//   - EnumToString / EnumFromString for BNMediumLevelILOperation
//     (140 enumerators from binaryninjacore.h:1338-1488). Added in
//     R9.2 commit A.
//   - EnumToString / EnumFromString for BNHighLevelILOperation
//     (126 enumerators from binaryninjacore.h:1521-1658). Added in
//     R9.3 commit A.
//
// Short-form vocabulary: mechanical prefix-strip + lowercase. So
// LLIL_SET_REG -> \"set_reg\", LLIL_CMP_SLT -> \"cmp_slt\",
// LLFC_E -> \"e\", LLFC_FUO -> \"fuo\", MLIL_SET_VAR -> \"set_var\",
// MLIL_CALL_SSA -> \"call_ssa\", HLIL_ASSIGN -> \"assign\",
// HLIL_VAR_DECLARE -> \"var_declare\". See
// docs/il-metatable-design.md section 4.2 / 4.2a for LLIL, section
// 12.2 for MLIL, and section 13.2 for HLIL.
//
// Dual-accept: EnumFromString accepts BOTH the short canonical form
// AND the verbatim BN/Python enumerator name (e.g. \"set_reg\" or
// \"LLIL_SET_REG\", \"set_var\" or \"MLIL_SET_VAR\", \"assign\" or
// \"HLIL_ASSIGN\"). Case-sensitive match.
//
// HLIL gotcha: a handful of HLIL opcodes short-form into Lua
// reserved words (\"if\", \"while\", \"for\", \"do\", \"return\",
// \"break\", \"goto\", \"continue\"). These are safe as STRING
// values; they only collide with identifiers if written as raw
// names, which this vocabulary never does.
//
// This file is included by bindings/il_operand_conv.cpp inside the
// BinjaLua namespace. It is a .inc fragment, not a standalone
// translation unit.
"""


OPERANDS_HEADER = """\
// Auto-generated from binaryninja-api/python/lowlevelil.py and
// binaryninja-api/python/mediumlevelil.py.
// Generator: scripts/generate_il_tables.py (tracked, run by hand).
// Do NOT hand-edit. Regenerate after bumping the BN submodule.
//
// Contents:
//   - LLILOperandSpecsForOperation(BNLowLevelILOperation) returning
//     the per-opcode const vector<LLILOperandSpec>& used by
//     LLILOperandToLua to project raw instruction operands into Lua
//     values. Originally landed in R9.1 commit 2a.
//   - MLILOperandSpecsForOperation(BNMediumLevelILOperation) returning
//     the per-opcode const vector<MLILOperandSpec>& used by
//     MLILOperandToLua. Added in R9.2 commit B.
//
// *OperandSpec shape (structs defined in bindings/il.h, field-identical
// between the two families):
//     struct LLILOperandSpec { const char* name; const char* type_tag;
//                              uint8_t slot_first; uint8_t slot_last; };
//     struct MLILOperandSpec { same; };
//
// For single-slot scalars and single-slot list/map accessors, slot_last
// == slot_first. For explicit-pair accessors (reg_ssa, flag_ssa,
// reg_stack_ssa, var_ssa, var_ssa_dest_and_src, ConstantData),
// slot_last == slot_first + 1. Encodings match the Python
// per-subclass detailed_operands overrides; see
// scripts/generate_il_tables.py ACCESSOR_TO_TAG mapping.
//
// Opcodes with empty detailed_operands (LLIL_NOP, LLIL_NORET, LLIL_POP,
// LLIL_SYSCALL, LLIL_BP, LLIL_UNDEF, LLIL_UNIMPL, MLIL_NOP,
// MLIL_NORET, MLIL_BP, MLIL_UNDEF, MLIL_UNIMPL, plus opcodes whose
// Python subclass is not concretely defined) map to an empty spec
// vector via each switch's default arm.
//
// This file is included by bindings/il_operand_conv.cpp inside the
// BinjaLua namespace.
"""


class DetailedOperandsExtractor:
    """Parse a python/<family>il.py file, find every per-opcode IL
    subclass, and for each subclass determine the ordered list of
    (operand_name, type_tag, slot_first, slot_last) triples produced
    by its detailed_operands property.

    family: one of "LLIL" or "MLIL". Selects the class-name prefix
    (LowLevelIL / MediumLevelIL), the operation-enum prefix
    (LLIL_ / MLIL_), and the detailed_operands tuple shape:
        - LLIL returns 2-tuples ``(name_str, self.field)``.
        - MLIL returns 3-tuples ``(name_str, self.field, type_str)``;
          the type_str is informational and discarded here since the
          tag comes from tracing self.field's accessor body.
    """

    _CLS_PREFIX = {
        "LLIL": "LowLevelIL",
        "MLIL": "MediumLevelIL",
        "HLIL": "HighLevelIL",
    }
    _OP_PREFIX = {
        "LLIL": "LLIL_",
        "MLIL": "MLIL_",
        "HLIL": "HLIL_",
    }

    def __init__(self, source: str, family: str) -> None:
        assert family in ("LLIL", "MLIL", "HLIL"), family
        self.family = family
        self.cls_prefix = self._CLS_PREFIX[family]
        self.op_prefix = self._OP_PREFIX[family]
        self.tree = ast.parse(source)
        # class_name -> { property_name -> (tag, slot_first, slot_last) }
        self.cls_prop_slots: Dict[
            str, Dict[str, Tuple[str, int, int]]] = {}
        # class_name -> ordered list of (name, field_name) from
        # detailed_operands return list, OR None if the class inherits.
        self.cls_detailed: Dict[str, Optional[List[Tuple[str, str]]]] = {}
        self.cls_bases: Dict[str, List[str]] = {}
        self.op_to_cls: Dict[str, str] = {}

        self._walk()

    def _walk(self) -> None:
        for node in self.tree.body:
            if isinstance(node, ast.ClassDef) and \
                    node.name.startswith(self.cls_prefix):
                self._scan_class(node)
        # Locate the module-level `ILInstruction` dispatch dict. LLIL
        # lives at python/lowlevelil.py:3085; MLIL at
        # python/mediumlevelil.py:3101; HLIL at
        # python/highlevelil.py:2381.
        for node in self.tree.body:
            if isinstance(node, ast.AnnAssign):
                target = node.target
                if isinstance(target, ast.Name) and \
                        target.id == "ILInstruction" and \
                        isinstance(node.value, ast.Dict):
                    self._harvest_dispatch_dict(node.value)
            elif isinstance(node, ast.Assign):
                for target in node.targets:
                    if isinstance(target, ast.Name) and \
                            target.id == "ILInstruction" and \
                            isinstance(node.value, ast.Dict):
                        self._harvest_dispatch_dict(node.value)

    @staticmethod
    def _base_names(node: ast.ClassDef) -> List[str]:
        names: List[str] = []
        for b in node.bases:
            if isinstance(b, ast.Name):
                names.append(b.id)
            elif isinstance(b, ast.Attribute):
                names.append(b.attr)
        return names

    def _scan_class(self, node: ast.ClassDef) -> None:
        self.cls_bases[node.name] = self._base_names(node)
        props: Dict[str, Tuple[str, int, int]] = {}
        detailed: Optional[List[Tuple[str, str]]] = None

        for item in node.body:
            if not isinstance(item, ast.FunctionDef):
                continue
            is_property = any(
                (isinstance(d, ast.Name) and d.id == "property")
                for d in item.decorator_list
            )
            if not is_property:
                continue

            if item.name == "detailed_operands":
                detailed = self._parse_detailed_operands(item)
                continue

            slot_info = self._find_accessor_in_body(item.body)
            if slot_info is not None:
                props[item.name] = slot_info

        self.cls_prop_slots[node.name] = props
        if detailed is not None:
            self.cls_detailed[node.name] = detailed

    def _find_accessor_in_body(
            self, body: List[ast.stmt]
    ) -> Optional[Tuple[str, int, int]]:
        for stmt in body:
            for sub in ast.walk(stmt):
                if not isinstance(sub, ast.Call):
                    continue
                f = sub.func
                if not isinstance(f, ast.Attribute):
                    continue
                if not isinstance(f.value, ast.Name) or f.value.id != "self":
                    continue
                name = f.attr
                if name not in ACCESSOR_TO_TAG:
                    continue
                tag, arity = ACCESSOR_TO_TAG[name]
                if len(sub.args) < arity:
                    continue
                slot_first = self._eval_int(sub.args[0])
                slot_last = slot_first
                if arity == 2:
                    slot_last = self._eval_int(sub.args[1])
                if slot_first is None or slot_last is None:
                    continue
                return (tag, slot_first, slot_last)
        return None

    @staticmethod
    def _eval_int(node: ast.expr) -> Optional[int]:
        if isinstance(node, ast.Constant) and isinstance(node.value, int):
            return node.value
        return None

    def _parse_detailed_operands(
            self, fn: ast.FunctionDef
    ) -> Optional[List[Tuple[str, str]]]:
        # LLIL emits 2-tuples (name_str, self.field); MLIL emits
        # 3-tuples (name_str, self.field, type_str). Both forms put
        # the bound field at index 1; the MLIL type string is
        # informational and discarded here because the concrete tag
        # is resolved by walking self.field's accessor body.
        for stmt in fn.body:
            if not isinstance(stmt, ast.Return):
                continue
            if not isinstance(stmt.value, ast.List):
                continue
            out: List[Tuple[str, str]] = []
            for elt in stmt.value.elts:
                if not isinstance(elt, ast.Tuple):
                    continue
                if len(elt.elts) < 2:
                    continue
                name_node = elt.elts[0]
                field_node = elt.elts[1]
                if not isinstance(name_node, ast.Constant) or \
                        not isinstance(name_node.value, str):
                    continue
                if not isinstance(field_node, ast.Attribute) or \
                        not isinstance(field_node.value, ast.Name) or \
                        field_node.value.id != "self":
                    continue
                out.append((name_node.value, field_node.attr))
            return out
        return None

    def _scan_create_dispatch(self, cls_node: ast.ClassDef) -> None:
        for item in cls_node.body:
            if isinstance(item, ast.FunctionDef) and item.name == "create":
                for sub in ast.walk(item):
                    if isinstance(sub, ast.Dict):
                        self._harvest_dispatch_dict(sub)

    def _harvest_dispatch_dict(self, d: ast.Dict) -> None:
        for k, v in zip(d.keys, d.values):
            op_name = self._extract_op_name(k)
            cls_name = self._extract_cls_name(v)
            if op_name and cls_name:
                self.op_to_cls[op_name] = cls_name

    def _extract_op_name(self, node: Optional[ast.expr]) -> Optional[str]:
        if isinstance(node, ast.Attribute) and \
                node.attr.startswith(self.op_prefix):
            return node.attr
        return None

    def _extract_cls_name(self, node: Optional[ast.expr]) -> Optional[str]:
        if isinstance(node, ast.Name) and \
                node.id.startswith(self.cls_prefix):
            return node.id
        return None

    def _resolve_prop_slot(
            self, cls_name: str, field_name: str
    ) -> Optional[Tuple[str, int, int]]:
        visited: set[str] = set()
        stack = [cls_name]
        while stack:
            cur = stack.pop(0)
            if cur in visited:
                continue
            visited.add(cur)
            props = self.cls_prop_slots.get(cur)
            if props is not None and field_name in props:
                return props[field_name]
            for base in self.cls_bases.get(cur, []):
                if base not in visited:
                    stack.append(base)
        return None

    def _resolve_detailed(
            self, cls_name: str
    ) -> Optional[List[Tuple[str, str]]]:
        visited: set[str] = set()
        stack = [cls_name]
        while stack:
            cur = stack.pop(0)
            if cur in visited:
                continue
            visited.add(cur)
            if cur in self.cls_detailed:
                return self.cls_detailed[cur]
            for base in self.cls_bases.get(cur, []):
                if base not in visited:
                    stack.append(base)
        return None

    def specs_for_op(
            self, op: str
    ) -> List[Tuple[str, str, int, int]]:
        cls_name = self.op_to_cls.get(op)
        if not cls_name:
            return []
        detailed = self._resolve_detailed(cls_name)
        if not detailed:
            return []
        out: List[Tuple[str, str, int, int]] = []
        for op_name, field_name in detailed:
            slot = self._resolve_prop_slot(cls_name, field_name)
            if slot is None:
                out.append((op_name, "unknown", 0, 0))
            else:
                tag, sf, sl = slot
                out.append((op_name, tag, sf, sl))
        return out


def emit_operands_table(operations: List[str],
                         extractor: DetailedOperandsExtractor,
                         family: str,
                         enum_type: str) -> str:
    """Emit the operand-spec fragment for a given IL family.

    family is the short prefix used in generated C++ identifiers
    ("LLIL" / "MLIL"). enum_type is the BN operation enum type
    ("BNLowLevelILOperation" / "BNMediumLevelILOperation").

    Uses the family's *OperandSpec struct. Per the R9.2 spec section
    12.5, MLIL reuses the same {name, type_tag, slot_first, slot_last}
    shape as LLIL via a typedef alias declared in bindings/il.h, so
    the C++ struct name differs per family while the fields are
    identical.
    """
    lines: List[str] = []
    lines.append(f"static const std::vector<{family}OperandSpec> "
                 f"k{family}OperandSpecsEmpty{{}};")
    lines.append("")

    non_empty: List[Tuple[str, List[Tuple[str, str, int, int]]]] = []
    for op in operations:
        specs = extractor.specs_for_op(op)
        if specs:
            non_empty.append((op, specs))

    for op, specs in non_empty:
        lines.append(
            f"static const std::vector<{family}OperandSpec> "
            f"k{family}OperandSpecs_{op}{{"
        )
        for name, tag, sf, sl in specs:
            lines.append(
                f"    {{\"{name}\", \"{tag}\", "
                f"static_cast<uint8_t>({sf}), "
                f"static_cast<uint8_t>({sl})}},"
            )
        lines.append("};")
        lines.append("")

    lines.append(f"const std::vector<{family}OperandSpec>& "
                 f"{family}OperandSpecsForOperation({enum_type} op) {{")
    lines.append("    switch (op) {")
    for op, _ in non_empty:
        lines.append(
            f"        case {op}: return k{family}OperandSpecs_{op};")
    lines.append(f"        default: return k{family}OperandSpecsEmpty;")
    lines.append("    }")
    lines.append("}")
    return "\n".join(lines) + "\n"


def emit_enums() -> Tuple[int, int]:
    if not BN_HEADER.exists():
        print(f"error: missing {BN_HEADER}", file=sys.stderr)
        return -1, -1
    llil_ops = extract_enumerators(BN_HEADER, *LLIL_OP_LINES, "LLIL_")
    llfc = extract_enumerators(BN_HEADER, *LLFC_LINES, "LLFC_")
    mlil_ops = extract_enumerators(BN_HEADER, *MLIL_OP_LINES, "MLIL_")
    hlil_ops = extract_enumerators(BN_HEADER, *HLIL_OP_LINES, "HLIL_")
    if len(llil_ops) != 143:
        print(f"error: expected 143 LLIL opcodes, got {len(llil_ops)}",
              file=sys.stderr)
        return -1, -1
    if len(llfc) != 22:
        print(f"error: expected 22 LLFC conditions, got {len(llfc)}",
              file=sys.stderr)
        return -1, -1
    if len(mlil_ops) != 140:
        print(f"error: expected 140 MLIL opcodes, got {len(mlil_ops)}",
              file=sys.stderr)
        return -1, -1
    if len(hlil_ops) != 126:
        print(f"error: expected 126 HLIL opcodes, got {len(hlil_ops)}",
              file=sys.stderr)
        return -1, -1

    parts: List[str] = [ENUMS_HEADER, "\n"]
    parts.append("// ---- BNLowLevelILOperation ----\n\n")
    parts.append(emit_enum_to_string("BNLowLevelILOperation",
                                     llil_ops, "LLIL_"))
    parts.append("\n")
    parts.append(emit_enum_from_string("BNLowLevelILOperation",
                                       llil_ops, "LLIL_"))
    parts.append("\n")
    parts.append("// ---- BNLowLevelILFlagCondition ----\n\n")
    parts.append(emit_enum_to_string("BNLowLevelILFlagCondition",
                                     llfc, "LLFC_"))
    parts.append("\n")
    parts.append(emit_enum_from_string("BNLowLevelILFlagCondition",
                                       llfc, "LLFC_"))
    parts.append("\n")
    parts.append("// ---- BNMediumLevelILOperation ----\n\n")
    parts.append(emit_enum_to_string("BNMediumLevelILOperation",
                                     mlil_ops, "MLIL_"))
    parts.append("\n")
    parts.append(emit_enum_from_string("BNMediumLevelILOperation",
                                       mlil_ops, "MLIL_"))
    parts.append("\n")
    parts.append("// ---- BNHighLevelILOperation ----\n\n")
    parts.append(emit_enum_to_string("BNHighLevelILOperation",
                                     hlil_ops, "HLIL_"))
    parts.append("\n")
    parts.append(emit_enum_from_string("BNHighLevelILOperation",
                                       hlil_ops, "HLIL_"))
    output = "".join(parts)
    ENUMS_OUT.write_text(output, encoding="utf-8", newline="\n")
    print(f"wrote {ENUMS_OUT} ({len(output)} bytes, "
          f"{len(llil_ops)} LLIL ops + {len(llfc)} LLFC conds + "
          f"{len(mlil_ops)} MLIL ops + {len(hlil_ops)} HLIL ops)")
    return len(llil_ops), len(llfc)


def emit_operands() -> int:
    for p in (PY_LLIL, PY_MLIL, PY_HLIL):
        if not p.exists():
            print(f"error: missing {p}", file=sys.stderr)
            return -1
    llil_source = PY_LLIL.read_text(encoding="utf-8")
    llil_extractor = DetailedOperandsExtractor(llil_source, "LLIL")
    mlil_source = PY_MLIL.read_text(encoding="utf-8")
    mlil_extractor = DetailedOperandsExtractor(mlil_source, "MLIL")
    hlil_source = PY_HLIL.read_text(encoding="utf-8")
    hlil_extractor = DetailedOperandsExtractor(hlil_source, "HLIL")

    llil_ops = extract_enumerators(BN_HEADER, *LLIL_OP_LINES, "LLIL_")
    mlil_ops = extract_enumerators(BN_HEADER, *MLIL_OP_LINES, "MLIL_")
    hlil_ops = extract_enumerators(BN_HEADER, *HLIL_OP_LINES, "HLIL_")

    parts: List[str] = [OPERANDS_HEADER, "\n"]
    parts.append("// ---- BNLowLevelILOperation operand specs ----\n\n")
    parts.append(emit_operands_table(
        llil_ops, llil_extractor, "LLIL", "BNLowLevelILOperation"))
    parts.append("\n")
    parts.append("// ---- BNMediumLevelILOperation operand specs ----\n\n")
    parts.append(emit_operands_table(
        mlil_ops, mlil_extractor, "MLIL", "BNMediumLevelILOperation"))
    parts.append("\n")
    parts.append("// ---- BNHighLevelILOperation operand specs ----\n\n")
    parts.append(emit_operands_table(
        hlil_ops, hlil_extractor, "HLIL", "BNHighLevelILOperation"))

    output = "".join(parts)
    OPERANDS_OUT.write_text(output, encoding="utf-8", newline="\n")

    llil_covered = sum(
        1 for op in llil_ops if llil_extractor.specs_for_op(op))
    mlil_covered = sum(
        1 for op in mlil_ops if mlil_extractor.specs_for_op(op))
    hlil_covered = sum(
        1 for op in hlil_ops if hlil_extractor.specs_for_op(op))
    unresolved: List[Tuple[str, str, str]] = []
    for op in llil_ops:
        for name, tag, _, _ in llil_extractor.specs_for_op(op):
            if tag == "unknown":
                unresolved.append(("LLIL", op, name))
    for op in mlil_ops:
        for name, tag, _, _ in mlil_extractor.specs_for_op(op):
            if tag == "unknown":
                unresolved.append(("MLIL", op, name))
    for op in hlil_ops:
        for name, tag, _, _ in hlil_extractor.specs_for_op(op):
            if tag == "unknown":
                unresolved.append(("HLIL", op, name))
    print(f"wrote {OPERANDS_OUT} ({len(output)} bytes, "
          f"LLIL {llil_covered}/{len(llil_ops)} + "
          f"MLIL {mlil_covered}/{len(mlil_ops)} + "
          f"HLIL {hlil_covered}/{len(hlil_ops)} opcodes with non-empty "
          f"specs, {len(unresolved)} unresolved field(s))")
    if unresolved:
        print("unresolved fields (emitted with tag=\"unknown\"):",
              file=sys.stderr)
        for fam, op, name in unresolved[:20]:
            print(f"  {fam} {op}.{name}", file=sys.stderr)
        if len(unresolved) > 20:
            print(f"  ... and {len(unresolved) - 20} more",
                  file=sys.stderr)
    return 0


def main() -> int:
    ops_n, _conds_n = emit_enums()
    if ops_n < 0:
        return 1
    rc = emit_operands()
    if rc != 0:
        return rc
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
