#!/usr/bin/env python3
"""
Extract Lua API definitions from C++ binding files.

This script parses sol2-based C++ binding files and extracts:
- Usertype definitions
- Properties (sol::property)
- Methods (lambdas and member function pointers)
- Metamethods (sol::meta_function)

Output: YAML file with placeholder descriptions ready for documentation.

Usage: python docs/extract_api.py [--output docs/api_definitions.yaml]
"""

import re
import argparse
from pathlib import Path
from dataclasses import dataclass, field
from typing import Dict, List, Optional


@dataclass
class ExtractedProperty:
    name: str
    return_type: str = "any"
    is_readonly: bool = True
    has_setter: bool = False


@dataclass
class ExtractedMethod:
    name: str
    return_type: str = "any"
    params: List[Dict[str, str]] = field(default_factory=list)
    is_metamethod: bool = False


@dataclass
class ExtractedUsertype:
    name: str
    metatable: str
    cpp_type: str
    properties: Dict[str, ExtractedProperty] = field(default_factory=dict)
    methods: Dict[str, ExtractedMethod] = field(default_factory=dict)
    metamethods: List[str] = field(default_factory=list)
    has_constructor: bool = False
    source_file: str = ""


class APIExtractor:
    """Extract API definitions from C++ binding files"""

    def __init__(self, project_root: Path):
        self.project_root = project_root
        # Auto-discover all binding .cpp files
        bindings_dir = project_root / "bindings"
        if bindings_dir.exists():
            self.binding_files = sorted(
                str(p.relative_to(project_root)).replace("\\", "/")
                for p in bindings_dir.glob("*.cpp")
            )
        else:
            self.binding_files = []
        self.usertypes: Dict[str, ExtractedUsertype] = {}

    def extract_all(self) -> Dict[str, ExtractedUsertype]:
        """Extract from all binding files"""
        for binding_file in self.binding_files:
            filepath = self.project_root / binding_file
            if filepath.exists():
                print(f"Extracting from {binding_file}...")
                self._extract_from_file(filepath, binding_file)
            else:
                print(f"Warning: {binding_file} not found")
        return self.usertypes

    def _extract_from_file(self, filepath: Path, rel_path: str):
        """Extract usertypes from a single file"""
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()

        # Find all usertype definitions
        # Pattern: lua.new_usertype<CppType>("MetatableName",
        usertype_pattern = r'lua\.new_usertype<([^>]+)>\s*\(\s*([A-Z_]+|"[^"]+"),\s*'

        for match in re.finditer(usertype_pattern, content):
            cpp_type = match.group(1).strip()
            metatable_raw = match.group(2).strip()

            # Handle both constant names and string literals
            if metatable_raw.startswith('"'):
                metatable = metatable_raw.strip('"')
            else:
                metatable = metatable_raw  # Keep the constant name

            # Extract the friendly name from metatable
            # "BinaryNinja.BinaryView" -> "BinaryView"
            # BINARYVIEW_METATABLE -> need to look up
            if '.' in metatable:
                name = metatable.split('.')[-1]
            elif metatable.endswith('_METATABLE'):
                name = self._metatable_to_name(metatable)
            else:
                name = cpp_type

            # Find the end of this usertype definition
            start_pos = match.end()
            usertype_content = self._extract_usertype_block(content, start_pos)

            usertype = ExtractedUsertype(
                name=name,
                metatable=metatable,
                cpp_type=cpp_type,
                source_file=rel_path
            )

            # Extract properties and methods from the block
            self._extract_properties(usertype_content, usertype)
            self._extract_methods(usertype_content, usertype)
            self._extract_metamethods(usertype_content, usertype)
            self._check_constructor(usertype_content, usertype)

            # Store by friendly name
            self.usertypes[name] = usertype
            print(f"  Found: {name} ({len(usertype.properties)} props, {len(usertype.methods)} methods)")

    def _metatable_to_name(self, metatable: str) -> str:
        """Convert METATABLE constant to friendly name"""
        # BINARYVIEW_METATABLE -> BinaryView
        # HEXADDRESS_METATABLE -> HexAddress
        name_map = {
            'BINARYVIEW_METATABLE': 'BinaryView',
            'FUNCTION_METATABLE': 'Function',
            'BASICBLOCK_METATABLE': 'BasicBlock',
            'SECTION_METATABLE': 'Section',
            'SYMBOL_METATABLE': 'Symbol',
            'HEXADDRESS_METATABLE': 'HexAddress',
            'SELECTION_METATABLE': 'Selection',
            'INSTRUCTION_METATABLE': 'Instruction',
            'VARIABLE_METATABLE': 'Variable',
            'DATAVARIABLE_METATABLE': 'DataVariable',
            'TYPE_METATABLE': 'Type',
            'TAG_METATABLE': 'Tag',
            'TAGTYPE_METATABLE': 'TagType',
            'FLOWGRAPH_METATABLE': 'FlowGraph',
            'FLOWGRAPHNODE_METATABLE': 'FlowGraphNode',
            'LLIL_FUNCTION_METATABLE': 'LowLevelILFunction',
            'LLIL_INSTRUCTION_METATABLE': 'LowLevelILInstruction',
            'MLIL_FUNCTION_METATABLE': 'MediumLevelILFunction',
            'MLIL_INSTRUCTION_METATABLE': 'MediumLevelILInstruction',
            'HLIL_FUNCTION_METATABLE': 'HighLevelILFunction',
            'HLIL_INSTRUCTION_METATABLE': 'HighLevelILInstruction',
        }
        return name_map.get(metatable, metatable.replace('_METATABLE', '').title())

    def _extract_usertype_block(self, content: str, start_pos: int) -> str:
        """Extract the content of a usertype definition (handling nested parens)"""
        depth = 1
        pos = start_pos
        while pos < len(content) and depth > 0:
            if content[pos] == '(':
                depth += 1
            elif content[pos] == ')':
                depth -= 1
            pos += 1
        return content[start_pos:pos-1]

    def _extract_properties(self, content: str, usertype: ExtractedUsertype):
        """Extract property definitions from usertype content"""
        # Pattern: "prop_name", sol::property(...)
        # Also handles: "prop_name", sol::readonly_property(...)
        prop_pattern = r'"(\w+)",\s*sol::(readonly_)?property\s*\('

        for match in re.finditer(prop_pattern, content):
            prop_name = match.group(1)
            is_readonly = match.group(2) is not None

            # Try to extract return type from the lambda
            # Look for -> Type pattern after the property(
            start = match.end()
            prop_block = content[start:start+500]  # Look ahead

            return_type = self._extract_return_type(prop_block)

            # Check if there's a setter (second lambda in property)
            has_setter = not is_readonly and ',' in prop_block.split(')')[0]

            usertype.properties[prop_name] = ExtractedProperty(
                name=prop_name,
                return_type=return_type,
                is_readonly=is_readonly or not has_setter,
                has_setter=has_setter
            )

    def _extract_methods(self, content: str, usertype: ExtractedUsertype):
        """Extract method definitions from usertype content"""
        # Pattern: "method_name", [](sol::this_state ts, Type& self, ...) -> ReturnType {
        # Or: "method_name", [](Type& self, ...) -> ReturnType {
        # Or: "method_name", &Class::Method

        # Lambda methods
        lambda_pattern = r'"(\w+)",\s*\[\]\s*\(([^)]*)\)\s*(?:->\s*([^{]+))?\s*\{'

        for match in re.finditer(lambda_pattern, content):
            method_name = match.group(1)
            params_str = match.group(2)
            return_type = match.group(3)

            # Skip if this looks like a property getter
            if method_name in usertype.properties:
                continue

            # Clean up return type
            if return_type:
                return_type = return_type.strip()
                return_type = self._cpp_type_to_lua(return_type)
            else:
                return_type = "nil"

            # Extract parameters (skip self and this_state)
            params = self._extract_params(params_str, usertype.cpp_type)

            usertype.methods[method_name] = ExtractedMethod(
                name=method_name,
                return_type=return_type,
                params=params
            )

        # Member variable/function pointer: "name", &Class::member
        # These can be either properties (data members) or methods (member functions)
        mfp_pattern = r'"(\w+)",\s*&(\w+)::(\w+)'
        for match in re.finditer(mfp_pattern, content):
            binding_name = match.group(1)
            class_name = match.group(2)
            member_name = match.group(3)

            if binding_name not in usertype.methods and binding_name not in usertype.properties:
                # Common data member names that should be properties
                data_member_hints = ['value', 'size', 'count', 'length', 'name', 'type', 'index',
                                    'start', 'end', 'address', 'offset']
                if member_name.lower() in data_member_hints or binding_name.lower() in data_member_hints:
                    usertype.properties[binding_name] = ExtractedProperty(
                        name=binding_name,
                        return_type="any",
                        is_readonly=True
                    )
                else:
                    usertype.methods[binding_name] = ExtractedMethod(
                        name=binding_name,
                        return_type="any"
                    )

    def _extract_metamethods(self, content: str, usertype: ExtractedUsertype):
        """Extract metamethod definitions"""
        meta_pattern = r'sol::meta_function::(\w+)'
        for match in re.finditer(meta_pattern, content):
            metamethod = match.group(1)
            if metamethod not in usertype.metamethods:
                usertype.metamethods.append(metamethod)

    def _check_constructor(self, content: str, usertype: ExtractedUsertype):
        """Check if usertype has a constructor"""
        usertype.has_constructor = 'sol::constructors' in content or 'sol::no_constructor' not in content

    def _extract_return_type(self, content: str) -> str:
        """Extract return type from lambda/property definition"""
        # Look for -> Type pattern
        arrow_match = re.search(r'->\s*([^{,\)]+)', content)
        if arrow_match:
            cpp_type = arrow_match.group(1).strip()
            return self._cpp_type_to_lua(cpp_type)
        return "any"

    def _cpp_type_to_lua(self, cpp_type: str) -> str:
        """Convert C++ type to Lua-friendly type string"""
        cpp_type = cpp_type.strip()

        # Handle common patterns
        type_map = {
            'bool': 'boolean',
            'int': 'integer',
            'int64_t': 'integer',
            'uint64_t': 'integer',
            'size_t': 'integer',
            'double': 'number',
            'float': 'number',
            'std::string': 'string',
            'HexAddress': 'HexAddress',
            'sol::table': 'table',
            'sol::object': 'any',
            'void': 'nil',
        }

        # Direct match
        if cpp_type in type_map:
            return type_map[cpp_type]

        # Handle optional types
        if cpp_type.startswith('std::optional<'):
            inner = cpp_type[14:-1]
            inner_lua = self._cpp_type_to_lua(inner)
            return f"{inner_lua}|nil"

        # Handle Ref<T>
        if cpp_type.startswith('Ref<'):
            inner = cpp_type[4:-1]
            return inner

        # Handle sol::table
        if 'sol::table' in cpp_type:
            return 'table'

        # Handle std::tuple
        if 'std::tuple' in cpp_type:
            return 'multiple'

        # Default: use the type name
        return cpp_type

    def _extract_params(self, params_str: str, self_type: str) -> List[Dict[str, str]]:
        """Extract parameters from lambda signature, skipping self and this_state"""
        params = []
        if not params_str.strip():
            return params

        # Split by comma, handling templates
        param_list = []
        depth = 0
        current = ""
        for char in params_str:
            if char == '<':
                depth += 1
            elif char == '>':
                depth -= 1
            elif char == ',' and depth == 0:
                param_list.append(current.strip())
                current = ""
                continue
            current += char
        if current.strip():
            param_list.append(current.strip())

        for param in param_list:
            param = param.strip()
            if not param:
                continue

            # Skip sol::this_state
            if 'sol::this_state' in param:
                continue

            # Skip self parameter (Type& or const Type&)
            if self_type in param and ('&' in param or '*' in param):
                continue

            # Extract parameter name and type
            # Handle: "const std::string& name" or "uint64_t addr"
            parts = param.rsplit(' ', 1)
            if len(parts) == 2:
                param_type, param_name = parts
                param_name = param_name.strip('&*')
                lua_type = self._cpp_type_to_lua(param_type.replace('const', '').replace('&', '').strip())
                params.append({
                    'name': param_name,
                    'type': lua_type,
                    'description': 'TODO: Add description'
                })

        return params


def generate_yaml(usertypes: Dict[str, ExtractedUsertype], existing_yaml: Optional[Dict] = None) -> str:
    """Generate YAML documentation from extracted usertypes"""
    import yaml

    output = {}

    for name, usertype in usertypes.items():
        # Check if we have existing documentation
        existing = existing_yaml.get(name, {}) if existing_yaml else {}

        class_def = {
            'description': existing.get('description', f'TODO: Add description for {name}'),
        }

        # Properties
        if usertype.properties:
            props = {}
            existing_props = existing.get('properties', {})
            for prop_name, prop in usertype.properties.items():
                existing_prop = existing_props.get(prop_name, {})
                props[prop_name] = {
                    'type': existing_prop.get('type', prop.return_type),
                    'description': existing_prop.get('description', 'TODO: Add description'),
                }
                if not prop.is_readonly:
                    props[prop_name]['writable'] = True
                # Preserve existing example
                if 'example' in existing_prop:
                    props[prop_name]['example'] = existing_prop['example']
                # Preserve aliases
                if 'aliases' in existing_prop:
                    props[prop_name]['aliases'] = existing_prop['aliases']
            class_def['properties'] = props

        # Methods
        if usertype.methods:
            methods = {}
            existing_methods = existing.get('methods', {})
            for method_name, method in usertype.methods.items():
                existing_method = existing_methods.get(method_name, {})
                method_def = {
                    'description': existing_method.get('description', 'TODO: Add description'),
                }
                if method.return_type and method.return_type != 'nil':
                    method_def['returns'] = existing_method.get('returns', method.return_type)
                if method.params:
                    # Use existing params if available, otherwise use extracted
                    method_def['params'] = existing_method.get('params', method.params)
                # Preserve existing example
                if 'example' in existing_method:
                    method_def['example'] = existing_method['example']
                methods[method_name] = method_def
            class_def['methods'] = methods

        output[name] = class_def

    # Custom YAML representer for multiline strings
    def str_representer(dumper, data):
        if '\n' in data:
            return dumper.represent_scalar('tag:yaml.org,2002:str', data, style='|')
        return dumper.represent_scalar('tag:yaml.org,2002:str', data)

    yaml.add_representer(str, str_representer)

    return yaml.dump(output, default_flow_style=False, sort_keys=False, allow_unicode=True, width=100)


def main():
    parser = argparse.ArgumentParser(description='Extract Lua API definitions from C++ bindings')
    parser.add_argument('--output', '-o', default='docs/api_definitions_extracted.yaml',
                       help='Output YAML file path')
    parser.add_argument('--merge', '-m', action='store_true',
                       help='Merge with existing api_definitions.yaml')
    args = parser.parse_args()

    # Find project root
    script_dir = Path(__file__).parent
    project_root = script_dir.parent

    # Extract API definitions
    extractor = APIExtractor(project_root)
    usertypes = extractor.extract_all()

    print(f"\nExtracted {len(usertypes)} usertypes:")
    for name, ut in usertypes.items():
        print(f"  {name}: {len(ut.properties)} properties, {len(ut.methods)} methods")

    # Load existing YAML for merging
    existing_yaml = None
    if args.merge:
        existing_path = project_root / 'docs' / 'api_definitions.yaml'
        if existing_path.exists():
            import yaml
            with open(existing_path, 'r', encoding='utf-8') as f:
                existing_yaml = yaml.safe_load(f)
            print(f"\nMerging with existing definitions from {existing_path}")

    # Generate YAML
    yaml_output = generate_yaml(usertypes, existing_yaml)

    # Write output
    output_path = project_root / args.output
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write("# Auto-extracted API definitions from C++ bindings\n")
        f.write("# Generated by docs/extract_api.py\n")
        f.write("# TODO items need manual documentation\n\n")
        f.write(yaml_output)

    print(f"\nWritten to {output_path}")

    if not args.merge:
        print("\nTip: Use --merge to merge with existing api_definitions.yaml")


if __name__ == '__main__':
    main()
