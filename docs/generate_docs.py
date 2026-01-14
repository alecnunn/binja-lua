#!/usr/bin/env python3

"""
Binary Ninja Lua API Documentation Generator

This script parses the C++ binding files and extracts API documentation
to generate comprehensive markdown documentation.

Usage: python docs/generate_docs.py
Output: docs/api-reference.md, docs/getting-started.md
"""

import re
import yaml
from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict, Optional

@dataclass
class APIMethod:
    name: str
    description: str = ""
    parameters: List[Dict[str, str]] = None  # [{"name": "addr", "type": "integer", "desc": "..."}]
    return_type: str = ""
    return_description: str = ""
    example: str = ""
    lua_signature: str = ""  # e.g., "BinaryView:read(address, length)"
    
    def __post_init__(self):
        if self.parameters is None:
            self.parameters = []

@dataclass 
class APIClass:
    name: str
    description: str = ""
    methods: List[APIMethod] = None
    file_path: str = ""
    
    def __post_init__(self):
        if self.methods is None:
            self.methods = []

class LuaAPIParser:
    """Parser for extracting API information from C++ binding files"""
    
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

        # Auto-discover all Lua extension files
        lua_api_dir = project_root / "lua-api"
        if lua_api_dir.exists():
            self.lua_api_files = sorted(
                str(p.relative_to(project_root)).replace("\\", "/")
                for p in lua_api_dir.glob("*.lua")
            )
        else:
            self.lua_api_files = []
    
    def parse_binding_file(self, filepath: str) -> Optional[APIClass]:
        """Parse a single binding file to extract API information"""
        full_path = self.project_root / filepath
        
        if not full_path.exists():
            print(f"Warning: {filepath} not found")
            return None
        
        with open(full_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # Infer class name from file path since metatables are defined in common.cpp
        filename = Path(filepath).stem  # e.g., "binaryview" from "bindings/binaryview.cpp"
        class_name = filename.capitalize()
        
        # Apply naming conventions
        if class_name.endswith('view'):
            class_name = class_name.replace('view', 'View')
        elif class_name.endswith('block'):
            class_name = class_name.replace('block', 'Block')
        
        # Extract methods with documentation comments
        methods = self._parse_documented_methods(content)
        
        return APIClass(
            name=class_name,
            description=self._get_class_description(class_name),
            methods=methods,
            file_path=filepath
        )
    
    def _parse_documented_methods(self, content: str) -> List[APIMethod]:
        """Parse methods with structured documentation comments"""
        methods = []

        # Pattern to match JSDoc-style comments
        doc_pattern = r'/\*\*\s*((?:\*.*?\n?)*?)\s*\*/'

        # Sol2 binding patterns:
        # "method_name", sol::property(...)  - for properties
        # "method_name", [](...)             - for lambda methods
        sol2_pattern = r'"(\w+)",\s*(?:sol::property\(|sol::readonly_property\(|\[)'

        # Find all documentation comments
        for doc_match in re.finditer(doc_pattern, content, re.MULTILINE | re.DOTALL):
            doc_text = doc_match.group(1)
            doc_end = doc_match.end()

            # Look for sol2 binding immediately after the comment (within 200 chars)
            remaining_content = content[doc_end:doc_end + 200]
            sol2_match = re.search(sol2_pattern, remaining_content)

            if not sol2_match:
                continue

            # Parse the documentation comment
            method_info = self._parse_doc_comment(doc_text)
            if method_info:
                # If no name was extracted from @luaapi, use the binding name
                if not method_info.name:
                    method_info.name = sol2_match.group(1)
                methods.append(method_info)

        return methods
    
    def _parse_doc_comment(self, doc_text: str) -> Optional[APIMethod]:
        """Parse a JSDoc-style documentation comment"""
        lines = [line.strip().lstrip('*') for line in doc_text.split('\n')]
        lines = [line for line in lines if line]  # Remove empty lines

        method_info = APIMethod(name="", description="")
        current_example = []
        in_example = False

        for line in lines:
            if line.strip().startswith('@luaapi '):
                # Extract Lua API signature: @luaapi BinaryView:start_addr()
                signature = line.strip()[8:].strip()
                method_info.lua_signature = signature

                # Extract method name from signature
                if ':' in signature and '(' in signature:
                    method_part = signature.split(':')[1].split('(')[0]
                    method_info.name = method_part

            elif line.strip().startswith('@description '):
                method_info.description = line.strip()[12:].strip()
            
            elif line.strip().startswith('@param '):
                # Parse @param name type description
                param_text = line[7:].strip()
                parts = param_text.split(' ', 2)
                if len(parts) >= 3:
                    param_name, param_type, param_desc = parts[0], parts[1], ' '.join(parts[2:])
                    method_info.parameters.append({
                        "name": param_name,
                        "type": param_type, 
                        "description": param_desc
                    })
            
            elif line.strip().startswith('@return '):
                # Parse @return type description
                return_text = line[8:].strip()
                parts = return_text.split(' ', 1)
                method_info.return_type = parts[0]
                if len(parts) > 1:
                    method_info.return_description = parts[1]
            
            elif line.strip().startswith('@example'):
                in_example = True
                current_example = []
            
            elif in_example:
                example_line = line[1:]
                current_example.append(example_line)
        
        if current_example:
            method_info.example = '\n'.join(current_example)
        
        return method_info if method_info.name else None
    
    
    def _get_class_description(self, class_name: str) -> str:
        """Get description for API class - returns generic description as actual descriptions come from YAML"""
        # Return a generic description - the real descriptions are in api_definitions.yaml
        # and are used by DocumentationGenerator.generate_yaml_api_reference()
        return f"API class for {class_name}"
    
    def parse_lua_api_file(self, filepath: str) -> Optional[APIClass]:
        """Parse a Lua API file to extract extension documentation"""
        full_path = self.project_root / filepath

        if not full_path.exists():
            print(f"Warning: {filepath} not found")
            return None

        with open(full_path, 'r', encoding='utf-8') as f:
            content = f.read()

        # Extract module info from the file header
        module_info = self._parse_lua_file_header(content)
        if not module_info:
            return None

        # Parse extension methods
        methods = self._parse_lua_extension_methods(content)

        return APIClass(
            name=module_info['name'],
            description=module_info['description'],
            methods=methods,
            file_path=filepath
        )

    def _parse_lua_file_header(self, content: str) -> Optional[Dict]:
        """Parse Lua file header documentation from block comments"""
        # Look for the first --[[ ]] block comment at the start of the file
        block_comment_pattern = r'^--\[\[(.*?)\]\]'
        match = re.search(block_comment_pattern, content, re.DOTALL | re.MULTILINE)

        if not match:
            return None

        comment_content = match.group(1)
        lines = comment_content.split('\n')

        module_name = ""
        description_lines = []
        example_lines = []
        in_example = False

        for line in lines:
            stripped = line.strip()

            if stripped.startswith('@file '):
                module_name = stripped[6:].strip().replace('.lua', '').title()
            elif stripped.startswith('@brief '):
                description_lines.append(stripped[7:].strip())
            elif stripped.startswith('@description '):
                description_lines.append(stripped[12:].strip())
            elif stripped.startswith('@example'):
                in_example = True
                example_lines = []
            elif in_example:
                # Preserve indentation for examples
                if line.startswith('    '):  # Remove comment block indentation
                    example_lines.append(line[4:])
                else:
                    example_lines.append(line)
            elif not stripped.startswith('@') and not in_example:
                # Regular description lines
                if stripped:
                    description_lines.append(stripped)

        # Join description properly
        description = " ".join(description_lines)

        if module_name:
            return {'name': module_name, 'description': description.strip()}
        return None

    def _parse_lua_extension_methods(self, content: str) -> List[APIMethod]:
        """Parse Lua extension methods with block comment documentation"""
        methods = []

        # Find all --[[ ]] block comments followed by function definitions
        block_comment_pattern = r'--\[\[(.*?)\]\]'

        for comment_match in re.finditer(block_comment_pattern, content, re.DOTALL):
            comment_text = comment_match.group(1)
            comment_end = comment_match.end()

            # Look for function assignment or definition after the comment
            remaining_content = content[comment_end:]

            # Try to match different function patterns:
            # 1. Metatable method: bv_mt.__index.method = function
            # 2. Standalone function: function name(...)
            # 3. Function assignment: name = function(...)

            func_match = None
            func_line = ""

            # Pattern 1: Metatable methods
            metatable_match = re.search(r'(\w+_mt\.__index\.(\w+)\s*=\s*function)', remaining_content)
            if metatable_match:
                func_match = metatable_match
                func_line = metatable_match.group(1)

            # Pattern 2: Standalone functions like "function dump(...)"
            standalone_match = re.search(r'function\s+(\w+)\s*\(', remaining_content)
            if standalone_match and not func_match:
                func_match = standalone_match
                func_line = f"function {standalone_match.group(1)}"

            # Pattern 3: Function assignments like "dump = function(...)"
            assignment_match = re.search(r'(\w+)\s*=\s*function', remaining_content)
            if assignment_match and not func_match:
                func_match = assignment_match
                func_line = f"{assignment_match.group(1)} = function"

            if func_match:
                method_info = self._parse_lua_block_comment(comment_text, func_line)
                if method_info:
                    methods.append(method_info)

        return methods

    def _parse_lua_block_comment(self, comment_text: str, func_line: str) -> Optional[APIMethod]:
        """Parse a Lua block comment with JSDoc-style directives"""
        # Preserve original line structure for indentation handling
        raw_lines = comment_text.split('\n')

        method_info = APIMethod(name="", description="")
        current_example = []
        in_example = False

        for i, line in enumerate(raw_lines):
            stripped = line.strip()

            if stripped.startswith('@'):
                # End example if we hit another directive
                if in_example and current_example:
                    method_info.example = '\n'.join(current_example)
                    in_example = False
                    current_example = []

            if stripped.startswith('@luaapi '):
                # Extract Lua API signature: @luaapi BinaryView:method_name() or @luaapi function_name()
                signature = stripped[8:].strip()
                method_info.lua_signature = signature

                # Extract method name from signature
                if ':' in signature and '(' in signature:
                    # Class method: BinaryView:method_name()
                    method_part = signature.split(':')[1].split('(')[0]
                    method_info.name = method_part
                elif '(' in signature:
                    # Standalone function: function_name()
                    function_part = signature.split('(')[0].strip()
                    method_info.name = function_part

            elif stripped.startswith('@description '):
                method_info.description = stripped[12:].strip()

            elif stripped.startswith('@return '):
                # Parse @return type description
                return_text = stripped[8:].strip()
                parts = return_text.split(' ', 1)
                method_info.return_type = parts[0]
                if len(parts) > 1:
                    method_info.return_description = parts[1]

            elif stripped.startswith('@param '):
                # Parse @param name type description
                param_text = stripped[7:].strip()
                parts = param_text.split(' ', 2)
                if len(parts) >= 3:
                    param_name, param_type, param_desc = parts[0], parts[1], ' '.join(parts[2:])
                    method_info.parameters.append({
                        "name": param_name,
                        "type": param_type,
                        "description": param_desc
                    })

            elif stripped.startswith('@example'):
                in_example = True
                current_example = []

            elif in_example:
                # Preserve original indentation for example lines
                # Remove leading whitespace that matches the comment indentation
                example_line = line
                if line.startswith('    '):  # Remove comment block indentation
                    example_line = line[4:]
                current_example.append(example_line)

        # Handle example at end of comment
        if in_example and current_example:
            # Remove any trailing empty lines from examples
            while current_example and not current_example[-1].strip():
                current_example.pop()
            method_info.example = '\n'.join(current_example)

        return method_info if method_info.name else None

    def _parse_lua_doc_comment(self, doc_text: str, func_line: str = "") -> Optional[APIMethod]:
        """Parse a JSDoc-style documentation comment in Lua format"""
        # Process lines more carefully to preserve example indentation
        raw_lines = doc_text.split('\n')
        processed_lines = []

        for line in raw_lines:
            stripped = line.strip()
            if stripped.startswith('---'):
                # Remove the leading --- but preserve the rest
                content = line.strip()[3:]
                if content.startswith(' '):
                    content = content[1:]  # Remove single space after ---
                processed_lines.append(content)

        # Remove empty lines
        processed_lines = [line for line in processed_lines if line.strip()]

        method_info = APIMethod(name="", description="")
        current_example = []
        in_example = False
        description_lines = []

        for line in processed_lines:
            if line.strip().startswith('@'):
                # End example if we hit another directive
                if in_example and current_example:
                    method_info.example = '\n'.join(current_example)
                    in_example = False
                    current_example = []

            if line.strip().startswith('@return '):
                # Parse @return type description
                return_text = line.strip()[8:].strip()
                parts = return_text.split(' ', 1)
                method_info.return_type = parts[0]
                if len(parts) > 1:
                    method_info.return_description = parts[1]

            elif line.strip().startswith('@param '):
                # Parse @param name type description
                param_text = line.strip()[7:].strip()
                parts = param_text.split(' ', 2)
                if len(parts) >= 3:
                    param_name, param_type, param_desc = parts[0], parts[1], ' '.join(parts[2:])
                    method_info.parameters.append({
                        "name": param_name,
                        "type": param_type,
                        "description": param_desc
                    })

            elif line.strip().startswith('@example'):
                in_example = True
                current_example = []

            elif in_example:
                # Preserve indentation for example lines
                current_example.append(line)

            elif not line.strip().startswith('@'):
                # Non-directive lines are part of the description
                if line.strip():
                    description_lines.append(line.strip())

        # Handle example at end of comment
        if in_example and current_example:
            method_info.example = '\n'.join(current_example)

        # Join description lines
        if description_lines:
            method_info.description = ' '.join(description_lines)

        # Extract method name and signature from function line
        if func_line:
            if '_mt.__index.' in func_line:
                # Extract method name from "bv_mt.__index.method_name = function"
                method_match = re.search(r'(\w+)_mt\.__index\.(\w+)', func_line)
                if method_match:
                    class_prefix = method_match.group(1)
                    method_name = method_match.group(2)
                    method_info.name = method_name

                    # Create signature based on class
                    if class_prefix == 'bv':
                        method_info.lua_signature = f'BinaryView:{method_name}()'
                    elif class_prefix == 'func':
                        method_info.lua_signature = f'Function:{method_name}()'
                    else:
                        method_info.lua_signature = f'{class_prefix.title()}:{method_name}()'
            elif func_line.startswith('function '):
                # Extract standalone function name from "function name("
                func_name_match = re.search(r'function\s+(\w+)', func_line)
                if func_name_match:
                    method_info.name = func_name_match.group(1)
                    # Use lua_signature from comment if available, otherwise create basic one
                    if not method_info.lua_signature:
                        method_info.lua_signature = f'{method_info.name}()'
            elif '= function' in func_line:
                # Extract function name from "name = function"
                func_name_match = re.search(r'(\w+)\s*=\s*function', func_line)
                if func_name_match:
                    method_info.name = func_name_match.group(1)
                    # Use lua_signature from comment if available, otherwise create basic one
                    if not method_info.lua_signature:
                        method_info.lua_signature = f'{method_info.name}()'

        return method_info if method_info.name else None

    def parse_all_bindings(self) -> List[APIClass]:
        """Parse all binding files and Lua API files and return API classes"""
        api_classes = []

        # Parse C++ binding files
        for binding_file in self.binding_files:
            api_class = self.parse_binding_file(binding_file)
            if api_class:
                api_classes.append(api_class)

        # Parse Lua API extension files
        for lua_file in self.lua_api_files:
            api_class = self.parse_lua_api_file(lua_file)
            if api_class:
                api_classes.append(api_class)

        return api_classes

class DocumentationGenerator:
    """Generates markdown documentation from parsed API data"""

    def __init__(self, project_root: Path):
        self.project_root = project_root
        self.parser = LuaAPIParser(project_root)
        self.api_definitions = self._load_api_definitions()

    def _load_api_definitions(self) -> Dict:
        """Load API definitions from YAML file"""
        yaml_path = self.project_root / "docs" / "api_definitions.yaml"
        if yaml_path.exists():
            with open(yaml_path, 'r', encoding='utf-8') as f:
                return yaml.safe_load(f)
        return {}

    def _generate_class_docs_from_yaml(self, class_name: str, class_def: Dict) -> List[str]:
        """Generate documentation lines for a class from YAML definition"""
        lines = [
            f"## {class_name}",
            "",
            f"*{class_def.get('description', 'No description available.')}*",
            "",
        ]

        # Properties
        properties = class_def.get('properties', {})
        if properties:
            lines.extend(["### Properties", ""])
            for prop_name, prop_def in properties.items():
                prop_type = prop_def.get('type', 'any')
                prop_desc = prop_def.get('description', '')
                aliases = prop_def.get('aliases', [])

                lines.append(f"#### `{class_name}.{prop_name}` -> `{prop_type}`")
                lines.append("")
                lines.append(prop_desc)
                if aliases:
                    lines.append(f"\n**Aliases:** `{', '.join(aliases)}`")
                lines.append("")

                example = prop_def.get('example')
                if example:
                    lines.extend([
                        "**Example:**",
                        "```lua",
                        example.strip(),
                        "```",
                        ""
                    ])

        # Methods
        methods = class_def.get('methods', {})
        if methods:
            lines.extend(["### Methods", ""])
            for method_name, method_def in methods.items():
                returns = method_def.get('returns', '')
                desc = method_def.get('description', '')
                params = method_def.get('params', [])

                return_info = f" -> `{returns}`" if returns else ""
                lines.append(f"#### `{class_name}:{method_name}(...)`{return_info}")
                lines.append("")
                lines.append(desc)
                lines.append("")

                if params:
                    lines.append("**Parameters:**")
                    for param in params:
                        p_name = param.get('name', '')
                        p_type = param.get('type', 'any')
                        p_desc = param.get('description', '')
                        lines.append(f"- `{p_name}` ({p_type}) - {p_desc}")
                    lines.append("")

                if returns:
                    ret_desc = method_def.get('return_description', '')
                    if ret_desc:
                        lines.extend([
                            "**Returns:**",
                            f"`{returns}` - {ret_desc}",
                            ""
                        ])

                example = method_def.get('example')
                if example:
                    lines.extend([
                        "**Example:**",
                        "```lua",
                        example.strip(),
                        "```",
                        ""
                    ])

        lines.extend(["---", ""])
        return lines

    def generate_yaml_api_reference(self) -> str:
        """Generate API reference from YAML definitions"""
        if not self.api_definitions:
            return "# API Reference\n\nNo API definitions found."

        lines = [
            "# Binary Ninja Lua API Reference",
            "",
            "*Generated from API definitions*",
            "",
            "## Table of Contents",
            ""
        ]

        # Table of contents
        for class_name in self.api_definitions.keys():
            lines.append(f"- [{class_name}](#{class_name.lower()})")

        lines.extend([
            "- [Magic Variables](#magic-variables)",
            "",
            "---",
            ""
        ])

        # Generate each class
        for class_name, class_def in self.api_definitions.items():
            lines.extend(self._generate_class_docs_from_yaml(class_name, class_def))

        # Magic Variables section
        lines.extend([
            "## Magic Variables",
            "",
            "These variables are automatically available in the Lua scripting environment:",
            "",
            "| Variable | Type | Description |",
            "|----------|------|-------------|",
            "| `bv` | BinaryView | Current binary view instance |",
            "| `current_function` | Function | Currently selected function (may be nil) |",
            "| `current_address` | HexAddress | Current address in the UI |",
            "| `here` | HexAddress | Alias for current_address |",
            "| `current_selection` | Selection | Currently selected range |",
            ""
        ])

        return "\n".join(lines)
    
    def generate_api_reference(self) -> str:
        """Generate complete API reference documentation"""
        api_classes = self.parser.parse_all_bindings()
        
        lines = [
            "# Binary Ninja Lua API Reference",
            "",
            "*Auto-generated from binding source code*",
            "",
            f"**Total Classes:** {len(api_classes)} | **Total Methods:** {sum(len(cls.methods) for cls in api_classes)}",
            "",
            "## Table of Contents",
            ""
        ]
        
        # Table of contents
        for api_class in api_classes:
            lines.append(f"- [{api_class.name}](#{api_class.name.lower()})")
        
        lines.extend([
            "- [Magic Variables](#magic-variables)",
            "- [Examples](#examples)",
            ""
        ])
        
        # Generate documentation for each class
        for api_class in api_classes:
            lines.extend([
                f"## {api_class.name}",
                "",
                f"*{api_class.description}*",
                "",
                f"**Source:** `{api_class.file_path}`",
                "",
            ])
            
            if api_class.methods:
                lines.append("### Methods")
                lines.append("")
                
                for method in api_class.methods:
                    # Use lua_signature if available, otherwise construct from method name
                    signature = method.lua_signature if method.lua_signature else f"{method.name}()"
                    return_info = f"→ `{method.return_type}`" if method.return_type else ""
                    
                    lines.extend([
                        f"#### `{signature}` {return_info}",
                        "",
                        method.description,
                        ""
                    ])
                    
                    # Add parameters if available
                    if method.parameters:
                        lines.append("**Parameters:**")
                        for param in method.parameters:
                            lines.append(f"- `{param['name']}` ({param['type']}) - {param['description']}")
                        lines.append("")
                    
                    # Add return information if available
                    if method.return_description:
                        lines.extend([
                            "**Returns:**",
                            f"`{method.return_type}` - {method.return_description}",
                            ""
                        ])
                    
                    # Use parsed example if available
                    if method.example:
                        lines.extend([
                            "**Example:**",
                            "```lua",
                            method.example,
                            "```",
                            ""
                        ])
            
            lines.extend(["---", ""])

        # Magic Variables section
        lines.extend([
            "## Magic Variables",
            "",
            "These variables are automatically available in the Lua scripting environment:",
            "",
            "| Variable | Type | Description |",
            "|----------|------|-------------|",
            "| `bv` | BinaryView | Current binary view instance |",
            "| `current_function` | Function | Currently selected function |", 
            "| `current_address` | integer | Current address in the UI |",
            "| `here` | integer | Alias for current_address |",
            "",
            "## Examples",
            "",
            "### Quick Analysis",
            "```lua",
            "-- Basic binary info",
            'print("File:", bv:file())',
            'print("Arch:", bv:arch())',
            'print("Functions:", #bv:functions())',
            "```",
            "",
            "### Function Analysis", 
            "```lua",
            "if current_function then",
            '    print("Function:", current_function:name())',
            '    print("Size:", current_function:size(), "bytes")',
            '    print("Blocks:", #current_function:basic_blocks())',
            "end",
            "```",
            "",
            "For more examples, see the `examples/` directory.",
            ""
        ])
        
        return "\n".join(lines)
    
    
    def generate_getting_started(self) -> str:
        """Generate getting started guide"""
        return '''# Getting Started with Binary Ninja Lua Scripting

The Binja-Lua scripting provider enables powerful binary analysis through an intuitive Lua API.

## Installation

1. Build the plugin following the instructions in the README
2. Copy the built plugin to your Binary Ninja plugins directory
3. Restart Binary Ninja

## Quick Start

1. **Open Binary Ninja** with a binary file loaded
2. **Open the scripting console** (Tools -> Scripting Console)
3. **Select "Lua"** as the scripting language
4. **Start scripting**

## Your First Script

```lua
-- Get basic information about the loaded binary
print("File:", bv.file)
print("Architecture:", bv.arch)
print("Size:", bv.length, "bytes")
print("Functions:", #bv:functions())

-- Use the built-in table inspector for complex data
local info = {
    file = bv.file,
    arch = bv.arch,
    functions = #bv:functions()
}
dump(info)  -- Pretty prints the table
```

## Common Tasks

### List All Functions
```lua
local functions = bv:functions()
print("Found", #functions, "functions:")

for i = 1, math.min(10, #functions) do
    local func = functions[i]
    print(string.format("%2d. %-20s @ %s (%d bytes)",
          i, func.name, func.start_addr, func.size))
end
```

### Analyze Current Function
```lua
if current_function then
    local func = current_function
    print("Analyzing:", func.name)
    print("Address range:", func.start_addr, "-", func.end_addr)
    print("Size:", func.size, "bytes")

    local blocks = func:basic_blocks()
    print("Basic blocks:", #blocks)

    for i, block in ipairs(blocks) do
        print(string.format("  Block %d: %s (%d instructions)",
              i, block.start_addr, block:instruction_count()))
    end
end
```

### Examine Disassembly
```lua
if current_function then
    local blocks = current_function:basic_blocks()
    if #blocks > 0 then
        local first_block = blocks[1]
        local disasm = first_block:disassembly()

        print("Disassembly of first basic block:")
        for i, line in ipairs(disasm) do
            print(string.format("  %s: %s", line.addr, line.text))
            if i >= 5 then -- Show first 5 instructions
                print("  ... (" .. (#disasm - 5) .. " more)")
                break
            end
        end
    end
end
```

### Cross-Reference Analysis
```lua
-- Find what calls the current function
if current_function then
    local func_addr = current_function.start_addr.value
    local callers = bv:get_code_refs(func_addr)

    if #callers > 0 then
        print("Function is called by:")
        for _, caller in ipairs(callers) do
            print("  ", caller.addr)
        end
    else
        print("No callers found")
    end
end
```

### Memory Analysis
```lua
-- Read and display memory
local start = bv.start_addr.value
local data = bv:read(start, 32)

print("First 32 bytes:")
local hex = ""
for i = 1, #data do
    hex = hex .. string.format("%02x ", data:byte(i))
    if i % 16 == 0 then
        print(string.format("0x%08x: %s", start + i - 16, hex))
        hex = ""
    end
end
if #hex > 0 then
    print(string.format("0x%08x: %s", start + #data - (#data % 16), hex))
end
```

## Tips and Best Practices

### Error Handling
```lua
-- Always use pcall for potentially failing operations
local success, result = pcall(function()
    return bv:get_function_at(some_address)
end)

if success and result then
    print("Found function:", result.name)
else
    print("No function at address or error occurred")
end
```

### Address Formatting
```lua
-- HexAddress objects print in hex automatically
print("Current address:", here)  -- Prints as 0x...

-- For arithmetic, use .value to get the numeric value
local next_addr = here.value + 0x10
print(string.format("Next address: 0x%x", next_addr))
```

### Iterating Safely
```lua
-- When iterating over collections, check for nil/empty
local functions = bv:functions()
if functions and #functions > 0 then
    for i, func in ipairs(functions) do
        -- Process function
        print(i, func.name)
    end
else
    print("No functions found")
end
```

### Performance Considerations
```lua
-- Cache frequently accessed values
local funcs = bv:functions()
local func_count = #funcs

print("Analyzing", func_count, "functions...")
for i = 1, func_count do
    local func = funcs[i]
    -- Process func using properties: func.name, func.size, etc.
end
```

## API Conventions

Understanding the difference between properties and methods:

### Properties (use dot notation)
```lua
-- Properties are accessed with dot notation
print(bv.file)           -- File path
print(bv.arch)           -- Architecture name
print(bv.length)         -- Binary size
print(func.name)         -- Function name
print(func.size)         -- Function size
print(func.start_addr)   -- Function start (HexAddress)
print(block.start_addr)  -- Block start (HexAddress)
```

### Methods (use colon notation)
```lua
-- Methods are called with colon notation
local funcs = bv:functions()           -- Get all functions
local func = bv:get_function_at(addr)  -- Get function at address
local blocks = func:basic_blocks()     -- Get basic blocks
local count = block:instruction_count() -- Get instruction count
local disasm = block:disassembly()     -- Get disassembly
```

### HexAddress Values
```lua
-- HexAddress objects display in hex but need .value for arithmetic
local addr = func.start_addr
print(addr)              -- Prints "0x401000"
print(addr.value)        -- Prints 4198400 (decimal)
print(addr.value + 0x10) -- Arithmetic works with .value
```

## Magic Variables

The following variables are automatically available and updated based on Binary Ninja's UI context:

| Variable | Type | Description |
|----------|------|-------------|
| `bv` | BinaryView | Current binary view instance |
| `current_function` | Function | Currently selected function (may be nil) |
| `current_address` | HexAddress | Current address in the UI |
| `here` | HexAddress | Alias for current_address |
| `current_selection` | Selection | Currently selected range (may be nil) |

## Next Steps

- Explore the [API Reference](api-reference.md) for complete method documentation
- Check out the [Lua Extensions](lua-extensions.md) for fluent collection APIs
- See the `examples/` directory for more advanced usage

Happy analyzing!
'''
    
    def generate_lua_extensions_docs(self) -> str:
        """Generate documentation specifically for Lua API extensions"""
        # Get only Lua API classes
        all_classes = self.parser.parse_all_bindings()
        lua_classes = [cls for cls in all_classes if cls.file_path.startswith('lua-api/')]

        lines = [
            "# Binary Ninja Lua API Extensions",
            "",
            "*Auto-generated from Lua extension source code*",
            "",
            f"**Extension Modules:** {len(lua_classes)} | **Extension Methods:** {sum(len(cls.methods) for cls in lua_classes)}",
            "",
            "## Overview",
            "",
            "This document covers the optional Lua API extensions that provide idiomatic",
            "Lua patterns for Binary Ninja analysis. These extensions are automatically",
            "loaded when available and provide enhanced functionality beyond the core C++ bindings.",
            "",
            "## Table of Contents",
            ""
        ]

        # Table of contents for Lua extensions only
        for api_class in lua_classes:
            lines.append(f"- [{api_class.name}](#{api_class.name.lower()})")

        lines.extend(["", "---", ""])

        # Generate documentation for each Lua extension class
        for api_class in lua_classes:
            lines.extend([
                f"## {api_class.name}",
                "",
                f"*{api_class.description}*",
                "",
                f"**Source:** `{api_class.file_path}`",
                "",
            ])

            if api_class.methods:
                lines.append("### Methods")
                lines.append("")

                for method in api_class.methods:
                    # Use lua_signature if available, otherwise construct from method name
                    signature = method.lua_signature if method.lua_signature else f"{method.name}()"
                    return_info = f"→ `{method.return_type}`" if method.return_type else ""

                    lines.extend([
                        f"#### `{signature}` {return_info}",
                        "",
                        method.description,
                        ""
                    ])

                    # Add parameters if available
                    if method.parameters:
                        lines.append("**Parameters:**")
                        for param in method.parameters:
                            lines.append(f"- `{param['name']}` ({param['type']}) - {param['description']}")
                        lines.append("")

                    # Add return information if available
                    if method.return_description:
                        lines.extend([
                            "**Returns:**",
                            f"`{method.return_type}` - {method.return_description}",
                            ""
                        ])

                    # Use parsed example if available
                    if method.example:
                        lines.extend([
                            "**Example:**",
                            "```lua",
                            method.example,
                            "```",
                            ""
                        ])

            lines.extend(["---", ""])

        return "\n".join(lines)

    def generate_core_api_reference(self) -> str:
        """Generate API reference for core C++ bindings only"""
        all_classes = self.parser.parse_all_bindings()
        # Filter out Lua extension classes
        api_classes = [cls for cls in all_classes if not cls.file_path.startswith('lua-api/')]

        lines = [
            "# Binary Ninja Lua API Reference",
            "",
            "*Auto-generated from C++ binding source code*",
            "",
            f"**Core Classes:** {len(api_classes)} | **Core Methods:** {sum(len(cls.methods) for cls in api_classes)}",
            "",
            "## Table of Contents",
            ""
        ]

        # Table of contents
        for api_class in api_classes:
            lines.append(f"- [{api_class.name}](#{api_class.name.lower()})")

        lines.extend([
            "- [Magic Variables](#magic-variables)",
            "- [Examples](#examples)",
            ""
        ])

        # Generate documentation for each class
        for api_class in api_classes:
            lines.extend([
                f"## {api_class.name}",
                "",
                f"*{api_class.description}*",
                "",
                f"**Source:** `{api_class.file_path}`",
                "",
            ])

            if api_class.methods:
                lines.append("### Methods")
                lines.append("")

                for method in api_class.methods:
                    # Use lua_signature if available, otherwise construct from method name
                    signature = method.lua_signature if method.lua_signature else f"{method.name}()"
                    return_info = f"→ `{method.return_type}`" if method.return_type else ""

                    lines.extend([
                        f"#### `{signature}` {return_info}",
                        "",
                        method.description,
                        ""
                    ])

                    # Add parameters if available
                    if method.parameters:
                        lines.append("**Parameters:**")
                        for param in method.parameters:
                            lines.append(f"- `{param['name']}` ({param['type']}) - {param['description']}")
                        lines.append("")

                    # Add return information if available
                    if method.return_description:
                        lines.extend([
                            "**Returns:**",
                            f"`{method.return_type}` - {method.return_description}",
                            ""
                        ])

                    # Use parsed example if available
                    if method.example:
                        lines.extend([
                            "**Example:**",
                            "```lua",
                            method.example,
                            "```",
                            ""
                        ])

            lines.extend(["---", ""])

        # Magic Variables section
        lines.extend([
            "## Magic Variables",
            "",
            "These variables are automatically available in the Lua scripting environment:",
            "",
            "| Variable | Type | Description |",
            "|----------|------|-------------|",
            "| `bv` | BinaryView | Current binary view instance |",
            "| `current_function` | Function | Currently selected function |",
            "| `current_address` | integer | Current address in the UI |",
            "| `here` | integer | Alias for current_address |",
            "",
            "## Examples",
            "",
            "### Quick Analysis",
            "```lua",
            "-- Basic binary info",
            'print("File:", bv:file())',
            'print("Arch:", bv:arch())',
            'print("Functions:", #bv:functions())',
            "```",
            "",
            "### Function Analysis",
            "```lua",
            "if current_function then",
            '    print("Function:", current_function:name())',
            '    print("Size:", current_function:size(), "bytes")',
            '    print("Blocks:", #current_function:basic_blocks())',
            "end",
            "```",
            "",
            "For more examples, see the `examples/` directory.",
            ""
        ])

        return "\n".join(lines)

    def generate_all_docs(self):
        """Generate all documentation files"""
        print("Generating Binary Ninja Lua API Documentation...")

        # Create docs directory
        docs_dir = self.project_root / "docs"
        docs_dir.mkdir(exist_ok=True)

        # Generate API reference from YAML definitions (primary source)
        print("Generating API reference from YAML definitions...")
        if self.api_definitions:
            yaml_api_ref = self.generate_yaml_api_reference()
            with open(docs_dir / "api-reference.md", 'w', encoding='utf-8') as f:
                f.write(yaml_api_ref)
            print(f"  - Generated {docs_dir / 'api-reference.md'}")
        else:
            print("  - Warning: No YAML definitions found, generating from source...")
            core_api_ref = self.generate_core_api_reference()
            with open(docs_dir / "api-reference.md", 'w', encoding='utf-8') as f:
                f.write(core_api_ref)

        # Generate Lua extensions documentation
        print("Generating Lua extensions documentation...")
        lua_extensions = self.generate_lua_extensions_docs()
        with open(docs_dir / "lua-extensions.md", 'w', encoding='utf-8') as f:
            f.write(lua_extensions)
        print(f"  - Generated {docs_dir / 'lua-extensions.md'}")

        # Generate getting started guide
        print("Generating getting started guide...")
        getting_started = self.generate_getting_started()
        with open(docs_dir / "getting-started.md", 'w', encoding='utf-8') as f:
            f.write(getting_started)
        print(f"  - Generated {docs_dir / 'getting-started.md'}")

        print("\nDocumentation generation complete!")

def main():
    """Main entry point"""
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    
    generator = DocumentationGenerator(project_root)
    generator.generate_all_docs()

if __name__ == "__main__":
    main()