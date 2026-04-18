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
ENUMS_OUT = REPO_ROOT / "bindings" / "il_enums.inc"
OPERANDS_OUT = REPO_ROOT / "bindings" / "il_operands_table.inc"

LLIL_OP_LINES = (586, 737)
LLFC_LINES = (739, 765)

# Map Python _get_X accessor name -> (type_tag, arg_count). arg_count
# tells us how many slot indexes to consume: 1 for single-slot and
# single-slot-internally-paired list accessors (BN helper consumes the
# (size, operand_idx) pair internally via a single slot argument);
# 2 for explicit-pair SSA accessors.
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
    lines.append(f"inline const char* EnumToString({enum_type} v) {{")
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
        f"inline std::optional<{enum_type}> EnumFromString<{enum_type}>("
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
// Generator: scripts/generate_il_tables.py (local-only, NOT committed).
// Do NOT hand-edit. Regenerate after bumping the BN submodule.
//
// Contents (commit 1 scope, R9.1 enums-only):
//   - EnumToString / EnumFromString for BNLowLevelILOperation
//     (143 enumerators from binaryninjacore.h:586-737).
//   - EnumToString / EnumFromString for BNLowLevelILFlagCondition
//     (22 enumerators from binaryninjacore.h:739-763: 14 integer +
//     8 floating-point).
//
// Short-form vocabulary: mechanical prefix-strip + lowercase. So
// LLIL_SET_REG -> \"set_reg\", LLIL_CMP_SLT -> \"cmp_slt\",
// LLFC_E -> \"e\", LLFC_FUO -> \"fuo\". See
// docs/il-metatable-design.md section 4.2 / 4.2a for the ratification.
//
// Dual-accept: EnumFromString accepts BOTH the short canonical form
// AND the verbatim BN/Python enumerator name (e.g. \"set_reg\" or
// \"LLIL_SET_REG\"). Case-sensitive match.
//
// This file is included by bindings/il_operand_conv.cpp inside the
// BinjaLua namespace. It is a .inc fragment, not a standalone
// translation unit.
"""


OPERANDS_HEADER = """\
// Auto-generated from binaryninja-api/python/lowlevelil.py.
// Generator: scripts/generate_il_tables.py (local-only, NOT committed).
// Do NOT hand-edit. Regenerate after bumping the BN submodule.
//
// Contents (commit 2a scope, R9.1 operand-spec table):
//   - LLILOperandSpecsForOperation(BNLowLevelILOperation) returning
//     the per-opcode const vector<LLILOperandSpec>& used by
//     LLILOperandToLua to project raw instruction operands into Lua
//     values.
//
// LLILOperandSpec shape (struct defined in bindings/il.h):
//     struct LLILOperandSpec {
//         const char* name;
//         const char* type_tag;
//         uint8_t slot_first;
//         uint8_t slot_last;
//     };
//
// For single-slot scalars and single-slot list/map accessors, slot_last
// == slot_first. For explicit-pair SSA tags (reg_ssa, flag_ssa,
// reg_stack_ssa), slot_last == slot_first + 1. Both encodings match
// python/lowlevelil.py per-subclass detailed_operands overrides (see
// scripts/generate_il_tables.py ACCESSOR_TO_TAG mapping).
//
// Opcodes with empty detailed_operands (LLIL_NOP, LLIL_NORET,
// LLIL_POP, LLIL_SYSCALL, LLIL_BP, LLIL_UNDEF, LLIL_UNIMPL, plus
// opcodes whose Python subclass is not concretely defined) map to an
// empty spec vector via the switch's default arm.
//
// This file is included by bindings/il_operand_conv.cpp inside the
// BinjaLua namespace.
"""


class DetailedOperandsExtractor:
    """Parse python/lowlevelil.py, find every LowLevelIL subclass, and
    for each subclass determine the ordered list of
    (operand_name, type_tag, slot_first, slot_last) triples produced by
    its detailed_operands property."""

    def __init__(self, source: str) -> None:
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
                    node.name.startswith("LowLevelIL"):
                self._scan_class(node)
        # Locate the module-level `ILInstruction` dispatch dict at
        # python/lowlevelil.py:3085.
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

    @staticmethod
    def _extract_op_name(node: Optional[ast.expr]) -> Optional[str]:
        if isinstance(node, ast.Attribute) and node.attr.startswith("LLIL_"):
            return node.attr
        return None

    @staticmethod
    def _extract_cls_name(node: Optional[ast.expr]) -> Optional[str]:
        if isinstance(node, ast.Name) and node.id.startswith("LowLevelIL"):
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
                         extractor: DetailedOperandsExtractor) -> str:
    lines: List[str] = []
    lines.append("static const std::vector<LLILOperandSpec> "
                 "kLLILOperandSpecsEmpty{};")
    lines.append("")

    non_empty: List[Tuple[str, List[Tuple[str, str, int, int]]]] = []
    for op in operations:
        specs = extractor.specs_for_op(op)
        if specs:
            non_empty.append((op, specs))

    for op, specs in non_empty:
        lines.append(
            f"static const std::vector<LLILOperandSpec> "
            f"kLLILOperandSpecs_{op}{{"
        )
        for name, tag, sf, sl in specs:
            lines.append(
                f"    {{\"{name}\", \"{tag}\", "
                f"static_cast<uint8_t>({sf}), "
                f"static_cast<uint8_t>({sl})}},"
            )
        lines.append("};")
        lines.append("")

    lines.append("const std::vector<LLILOperandSpec>& "
                 "LLILOperandSpecsForOperation(BNLowLevelILOperation op) {")
    lines.append("    switch (op) {")
    for op, _ in non_empty:
        lines.append(f"        case {op}: return kLLILOperandSpecs_{op};")
    lines.append("        default: return kLLILOperandSpecsEmpty;")
    lines.append("    }")
    lines.append("}")
    return "\n".join(lines) + "\n"


def emit_enums() -> Tuple[int, int]:
    if not BN_HEADER.exists():
        print(f"error: missing {BN_HEADER}", file=sys.stderr)
        return -1, -1
    llil_ops = extract_enumerators(BN_HEADER, *LLIL_OP_LINES, "LLIL_")
    llfc = extract_enumerators(BN_HEADER, *LLFC_LINES, "LLFC_")
    if len(llil_ops) != 143:
        print(f"error: expected 143 LLIL opcodes, got {len(llil_ops)}",
              file=sys.stderr)
        return -1, -1
    if len(llfc) != 22:
        print(f"error: expected 22 LLFC conditions, got {len(llfc)}",
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
    output = "".join(parts)
    ENUMS_OUT.write_text(output, encoding="utf-8", newline="\n")
    print(f"wrote {ENUMS_OUT} ({len(output)} bytes, "
          f"{len(llil_ops)} LLIL ops + {len(llfc)} LLFC conds)")
    return len(llil_ops), len(llfc)


def emit_operands() -> int:
    if not PY_LLIL.exists():
        print(f"error: missing {PY_LLIL}", file=sys.stderr)
        return -1
    source = PY_LLIL.read_text(encoding="utf-8")
    extractor = DetailedOperandsExtractor(source)

    llil_ops = extract_enumerators(BN_HEADER, *LLIL_OP_LINES, "LLIL_")
    parts: List[str] = [OPERANDS_HEADER, "\n"]
    parts.append(emit_operands_table(llil_ops, extractor))

    output = "".join(parts)
    OPERANDS_OUT.write_text(output, encoding="utf-8", newline="\n")

    covered = sum(1 for op in llil_ops if extractor.specs_for_op(op))
    unresolved: List[Tuple[str, str]] = []
    for op in llil_ops:
        for name, tag, _, _ in extractor.specs_for_op(op):
            if tag == "unknown":
                unresolved.append((op, name))
    print(f"wrote {OPERANDS_OUT} ({len(output)} bytes, "
          f"{covered}/{len(llil_ops)} opcodes with non-empty specs, "
          f"{len(unresolved)} unresolved field(s))")
    if unresolved:
        print("unresolved fields (emitted with tag=\"unknown\"):",
              file=sys.stderr)
        for op, name in unresolved[:20]:
            print(f"  {op}.{name}", file=sys.stderr)
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
