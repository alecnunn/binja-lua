#!/usr/bin/env python3
"""
Extract Lua extension API definitions from Lua source files.

This script parses lua-api/*.lua files and extracts:
- Module descriptions from @file/@brief
- Method definitions from @luaapi tags
- Descriptions, parameters, return types, and examples

Output: YAML file documenting the Lua extension APIs.

Usage: python docs/extract_lua_extensions.py [--output docs/lua_extensions.yaml]
"""

import re
import argparse
from pathlib import Path
from dataclasses import dataclass, field
from typing import Dict, List, Optional


@dataclass
class ExtractedParam:
    name: str
    type: str
    description: str = ""


@dataclass
class ExtractedMethod:
    name: str
    target_type: str  # e.g., "BinaryView", "Function", "Query"
    description: str = ""
    return_type: str = ""
    return_description: str = ""
    params: List[ExtractedParam] = field(default_factory=list)
    example: str = ""


@dataclass
class ExtractedModule:
    name: str
    description: str = ""
    source_file: str = ""
    methods: List[ExtractedMethod] = field(default_factory=list)


class LuaExtensionExtractor:
    """Extract API definitions from Lua extension files"""

    def __init__(self, project_root: Path):
        self.project_root = project_root
        # Auto-discover all Lua extension files
        lua_api_dir = project_root / "lua-api"
        if lua_api_dir.exists():
            self.lua_files = sorted(
                str(p.relative_to(project_root)).replace("\\", "/")
                for p in lua_api_dir.glob("*.lua")
            )
        else:
            self.lua_files = []
        self.modules: Dict[str, ExtractedModule] = {}

    def extract_all(self) -> Dict[str, ExtractedModule]:
        """Extract from all Lua extension files"""
        for lua_file in self.lua_files:
            filepath = self.project_root / lua_file
            if filepath.exists():
                print(f"Extracting from {lua_file}...")
                self._extract_from_file(filepath, lua_file)
            else:
                print(f"Warning: {lua_file} not found")
        return self.modules

    def _extract_from_file(self, filepath: Path, rel_path: str):
        """Extract module and method definitions from a Lua file"""
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()

        # Extract module name from filename
        module_name = filepath.stem

        # Extract module description from @file/@brief at top
        module_desc = self._extract_module_description(content)

        module = ExtractedModule(
            name=module_name,
            description=module_desc,
            source_file=rel_path
        )

        # Find all doc blocks with @luaapi
        doc_blocks = self._find_doc_blocks(content)
        for block in doc_blocks:
            method = self._parse_doc_block(block)
            if method:
                module.methods.append(method)

        self.modules[module_name] = module
        print(f"  Found: {module_name} ({len(module.methods)} methods)")

    def _extract_module_description(self, content: str) -> str:
        """Extract module description from @file/@brief comment at top"""
        # Look for opening --[[ block
        match = re.match(r'--\[\[\s*\n(.*?)\]\]', content, re.DOTALL)
        if not match:
            return ""

        block = match.group(1)

        # Extract @brief or general description
        brief_match = re.search(r'@brief\s+(.+?)(?=\n@|\n\n|$)', block, re.DOTALL)
        if brief_match:
            return brief_match.group(1).strip()

        # Otherwise use first non-tag line
        lines = []
        for line in block.split('\n'):
            line = line.strip()
            if line.startswith('@'):
                continue
            if line:
                lines.append(line)
        return ' '.join(lines[:3])  # First few lines

    def _find_doc_blocks(self, content: str) -> List[str]:
        """Find all --[[ ]] blocks containing @luaapi"""
        blocks = []
        # Pattern to match --[[ ... ]] blocks
        pattern = r'--\[\[(.*?)\]\]'
        for match in re.finditer(pattern, content, re.DOTALL):
            block = match.group(1)
            if '@luaapi' in block:
                blocks.append(block)
        return blocks

    def _parse_doc_block(self, block: str) -> Optional[ExtractedMethod]:
        """Parse a doc block into an ExtractedMethod"""
        # Extract @luaapi line
        api_match = re.search(r'@luaapi\s+(\w+):(\w+)\s*\(([^)]*)\)', block)
        if not api_match:
            # Try alternative format: @luaapi ClassName.method() or just method()
            api_match = re.search(r'@luaapi\s+(?:(\w+)[.:])?(\w+)\s*\(([^)]*)\)', block)
            if not api_match:
                return None

        target_type = api_match.group(1) or "global"
        method_name = api_match.group(2)
        params_str = api_match.group(3)

        method = ExtractedMethod(
            name=method_name,
            target_type=target_type
        )

        # Extract @description (stop at first @ tag)
        desc_match = re.search(r'@description\s+(.+?)(?=\n\s*@|\Z)', block, re.DOTALL)
        if desc_match:
            desc_text = desc_match.group(1).strip()
            # Clean up: stop at any line that starts with @
            clean_lines = []
            for line in desc_text.split('\n'):
                if line.strip().startswith('@'):
                    break
                clean_lines.append(line.strip())
            method.description = ' '.join(clean_lines)

        # Extract @return
        return_match = re.search(r'@return\s+(\w+)\s+([^\n@]+)', block)
        if return_match:
            method.return_type = return_match.group(1)
            method.return_description = return_match.group(2).strip().replace('\n', ' ')

        # Extract @param entries
        for param_match in re.finditer(r'@param\s+(\w+)\s+\((\w+)\)\s+(.*?)(?=\n@|$)', block, re.DOTALL):
            method.params.append(ExtractedParam(
                name=param_match.group(1),
                type=param_match.group(2),
                description=param_match.group(3).strip().replace('\n', ' ')
            ))

        # Extract @example
        example_match = re.search(r'@example\s*\n(.*?)(?=\]\]|$)', block, re.DOTALL)
        if example_match:
            example = example_match.group(1).strip()
            # Clean up indentation
            lines = example.split('\n')
            if lines:
                # Find minimum indentation
                min_indent = float('inf')
                for line in lines:
                    if line.strip():
                        indent = len(line) - len(line.lstrip())
                        min_indent = min(min_indent, indent)
                if min_indent < float('inf'):
                    lines = [line[min_indent:] if len(line) >= min_indent else line for line in lines]
            method.example = '\n'.join(lines)

        return method


def generate_yaml(modules: Dict[str, ExtractedModule], existing_yaml: Optional[Dict] = None) -> str:
    """Generate YAML documentation from extracted modules"""
    import yaml

    output = {
        '_meta': {
            'title': 'Lua Extension APIs',
            'description': 'Optional & experimental Lua extensions for binja-lua',
            'status': 'experimental'
        }
    }

    for name, module in modules.items():
        existing_mod = existing_yaml.get(name, {}) if existing_yaml else {}

        mod_def = {
            'description': existing_mod.get('description', module.description),
            'source': module.source_file,
            'methods': {}
        }

        existing_methods = existing_mod.get('methods', {})

        # Group methods by target type
        by_target: Dict[str, List[ExtractedMethod]] = {}
        for method in module.methods:
            target = method.target_type
            if target not in by_target:
                by_target[target] = []
            by_target[target].append(method)

        for target, methods in by_target.items():
            if target not in mod_def['methods']:
                mod_def['methods'][target] = {}

            for method in methods:
                method_key = f"{target}:{method.name}"
                existing_method = existing_methods.get(method_key, {})

                method_def = {
                    'description': existing_method.get('description', method.description),
                }

                if method.return_type:
                    method_def['returns'] = {
                        'type': existing_method.get('returns', {}).get('type', method.return_type),
                        'description': existing_method.get('returns', {}).get('description', method.return_description)
                    }

                if method.params:
                    method_def['params'] = []
                    existing_params = existing_method.get('params', [])
                    for i, param in enumerate(method.params):
                        existing_param = existing_params[i] if i < len(existing_params) else {}
                        method_def['params'].append({
                            'name': param.name,
                            'type': existing_param.get('type', param.type),
                            'description': existing_param.get('description', param.description)
                        })

                if method.example:
                    method_def['example'] = existing_method.get('example', method.example)

                mod_def['methods'][target][method.name] = method_def

        output[name] = mod_def

    # Custom YAML representer for multiline strings
    def str_representer(dumper, data):
        if '\n' in data:
            return dumper.represent_scalar('tag:yaml.org,2002:str', data, style='|')
        return dumper.represent_scalar('tag:yaml.org,2002:str', data)

    yaml.add_representer(str, str_representer)

    return yaml.dump(output, default_flow_style=False, sort_keys=False, allow_unicode=True, width=120)


def main():
    parser = argparse.ArgumentParser(description='Extract Lua extension API definitions')
    parser.add_argument('--output', '-o', default='docs/lua_extensions.yaml',
                       help='Output YAML file path')
    parser.add_argument('--merge', '-m', action='store_true',
                       help='Merge with existing lua_extensions.yaml')
    args = parser.parse_args()

    # Find project root
    script_dir = Path(__file__).parent
    project_root = script_dir.parent

    # Extract API definitions
    extractor = LuaExtensionExtractor(project_root)
    modules = extractor.extract_all()

    print(f"\nExtracted {len(modules)} modules:")
    for name, mod in modules.items():
        print(f"  {name}: {len(mod.methods)} methods")

    # Load existing YAML for merging
    existing_yaml = None
    if args.merge:
        existing_path = project_root / 'docs' / 'lua_extensions.yaml'
        if existing_path.exists():
            import yaml
            with open(existing_path, 'r', encoding='utf-8') as f:
                existing_yaml = yaml.safe_load(f)
            print(f"\nMerging with existing definitions from {existing_path}")

    # Generate YAML
    yaml_output = generate_yaml(modules, existing_yaml)

    # Write output
    output_path = project_root / args.output
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write("# Lua Extension API definitions\n")
        f.write("# Generated by docs/extract_lua_extensions.py\n")
        f.write("# These are optional & experimental extensions\n\n")
        f.write(yaml_output)

    print(f"\nWritten to {output_path}")


if __name__ == '__main__':
    main()
