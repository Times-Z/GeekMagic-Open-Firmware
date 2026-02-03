#!/usr/bin/env python3
"""
Simple OpenAPI generator for @openapi annotations in C++ sources

Scans the src/ and include/ folders for lines containing
  // @openapi {METHOD} /path summary="..." [requestBody=TYPE] [requestBodySchema=field:type,...] responses=CODE:CONTENTTYPE[,...]

and emits openapi.json (v3) to stdout
"""
import re
import json
import sys
import argparse
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PATTERN = re.compile(
    r"@openapi\s+\{(?P<method>\w+)\}\s+(?P<path>\S+)"
    r"(?:\s+version=(?P<version>\S+))?"
    r"(?:\s+group=(?P<group>\S+))?"
    r"(?:\s+summary=\"(?P<summary>[^\"]*)\")?"
    r"(?:\s+requiresAuth=(?P<requiresAuth>\w+))?"
    r"(?:\s+requestBody=(?P<requestBody>\S+))?"
    r"(?:\s+requestBodySchema=(?P<requestBodySchema>\S+))?"
    r"(?:\s+example=(?P<example>\{[^}]+\}))?"
    r"(?:\s+responses=(?P<responses>[^\n]+))?"
)


def _derive_version(path):
    match = re.search(r"/api/(v\d+)(?:/|$)", path)
    if match:
        return match.group(1)
    return None


def parse_request_body_schema(schema_str):
    """
    Parse request body schema string like 'field1:string,field2:number'
    Returns a schema object with properties and required fields
    """
    if not schema_str:
        return {'type': 'object'}
    
    schema = {
        'type': 'object',
        'properties': {},
        'required': []
    }
    
    for field_def in schema_str.split(','):
        field_def = field_def.strip()
        if ':' in field_def:
            field_name, field_type = field_def.split(':', 1)
            field_name = field_name.strip()
            field_type = field_type.strip()
            
            # Map common types to OpenAPI types
            type_map = {
                'string': 'string',
                'int': 'integer',
                'integer': 'integer',
                'number': 'number',
                'bool': 'boolean',
                'boolean': 'boolean',
                'array': 'array',
                'object': 'object',
            }
            
            openapi_type = type_map.get(field_type.lower(), 'string')
            schema['properties'][field_name] = {'type': openapi_type}
            schema['required'].append(field_name)
    
    return schema


def collect_annotations():
    annotations = []
    for folder in (ROOT / 'src', ROOT / 'include'):
        if not folder.exists():
            continue
        for p in folder.rglob('*.cpp'):
            text = p.read_text(errors='ignore')
            text = _preprocess_multiline_annotations(text)
            for m in PATTERN.finditer(text):
                d = m.groupdict()
                d['file'] = str(p.relative_to(ROOT))
                annotations.append(d)
        for p in folder.rglob('*.h'):
            text = p.read_text(errors='ignore')
            text = _preprocess_multiline_annotations(text)
            for m in PATTERN.finditer(text):
                d = m.groupdict()
                d['file'] = str(p.relative_to(ROOT))
                annotations.append(d)
    return annotations


def _preprocess_multiline_annotations(text):
    """
    Combine multiline @openapi comments into single lines.
    Handles cases where comment lines starting with // are continuation lines.
    """
    lines = text.split('\n')
    result = []
    i = 0
    while i < len(lines):
        line = lines[i]
        # Check if this line contains @openapi
        if '@openapi' in line:
            # Extract the base line and accumulate continuation lines
            combined = line
            j = i + 1
            # Look ahead for continuation lines (lines that start with // but don't have @openapi)
            while j < len(lines):
                next_line = lines[j]
                stripped = next_line.strip()
                # Check if it's a continuation (starts with // and is likely a continuation)
                if stripped.startswith('//') and '@openapi' not in next_line:
                    # Remove the // prefix and any leading/trailing whitespace, then append
                    continuation = stripped[2:].strip()
                    combined += ' ' + continuation
                    j += 1
                else:
                    break
            result.append(combined)
            i = j
        else:
            result.append(line)
            i += 1
    return '\n'.join(result)


def build_openapi(annotations):
    api = {
        'openapi': '3.0.0',
        'info': {'title': ROOT.name + ' API', 'version': '1.0.0'},
        'servers': [
            {
                'url': 'http://{host}/',
                'description': 'API Server for version 1',
                'variables': {'host': {'default': 'localhost'}},
            }
        ],
        'paths': {},
        'components': {
            'securitySchemes': {
                'bearerAuth': {
                    'type': 'http',
                    'scheme': 'bearer',
                    'bearerFormat': 'Token',
                    'description': 'Bearer token authentication. Include the token in the Authorization header as: Bearer <token>'
                }
            }
        }
    }

    tags_seen = set()

    for a in annotations:
        path = a['path']
        method = a['method'].lower()
        summary = a.get('summary') or ''
        requestBody = a.get('requestBody')
        responses = a.get('responses') or '200:application/json'
        version = a.get('version') or _derive_version(path)
        requiresAuth = a.get('requiresAuth') == 'true'

        full_path = path
        if version and not path.startswith('/api/'):
            normalized = path if path.startswith('/') else f"/{path}"
            full_path = f"/api/{version}{normalized}"

        path_obj = api['paths'].setdefault(full_path, {})
        operation_id_parts = ["op"]
        if version:
            operation_id_parts.append(version)
        operation_id_parts.append(method)
        operation_id_parts.append(full_path.strip('/').replace('/', '_'))

        op = {
            'summary': summary,
            'operationId': "_".join(operation_id_parts),
            'responses': {},
        }

        # Add tags for group only (not version)
        group = a.get('group')
        if group:
            op['tags'] = [group]
            tags_seen.add(group)

        # Add security requirement and description for authenticated endpoints
        if requiresAuth:
            op['security'] = [{'bearerAuth': []}]
            op['description'] = '**Requires Authentication** - This endpoint requires a valid bearer token in the Authorization header.'
            if summary:
                op['description'] = f"**Requires Authentication** - {summary}. This endpoint requires a valid bearer token in the Authorization header."
        
        # parse responses like 200:application/json,404:application/json
        for resp in [r.strip() for r in responses.split(',') if r.strip()]:
            parts = resp.split(':')
            code = parts[0]
            content_type = parts[1] if len(parts) > 1 else 'application/json'
            op['responses'][code] = {
                'description': '',
                'content': {content_type: {'schema': {'type': 'object'}}},
            }

        if requestBody:
            media = requestBody
            requestBodySchema = a.get('requestBodySchema')
            schema = parse_request_body_schema(requestBodySchema)
            
            # Add example if provided
            example_str = a.get('example')
            if example_str:
                try:
                    example = json.loads(example_str)
                    schema['example'] = example
                except json.JSONDecodeError:
                    pass  # Skip invalid example JSON
            
            op['requestBody'] = {
                'content': {media: {'schema': schema}},
                'required': True,
            }

        path_obj[method] = op

    if tags_seen:
        api['tags'] = [{'name': tag, 'description': f"API {tag} endpoints"} for tag in sorted(tags_seen)]

    return api


def _to_yaml(obj, indent=0):
    pad = '  ' * indent
    if isinstance(obj, dict):
        lines = []
        for k, v in obj.items():
            if isinstance(v, dict):
                lines.append(f"{pad}{k}:")
                lines.append(_to_yaml(v, indent + 1))
            elif isinstance(v, list):
                if len(v) == 0:
                    # Handle empty list
                    lines.append(f"{pad}{k}: []")
                else:
                    lines.append(f"{pad}{k}:")
                    lines.append(_to_yaml(v, indent + 1))
            else:
                # use JSON dumps for safe scalar representation
                lines.append(f"{pad}{k}: {json.dumps(v)}")
        return '\n'.join(lines)
    elif isinstance(obj, list):
        if len(obj) == 0:
            return f"{pad}[]"
        lines = []
        for item in obj:
            if isinstance(item, (dict, list)):
                lines.append(f"{pad}- ")
                # indent nested content one more level
                nested = _to_yaml(item, indent + 1)
                lines.append(nested)
            else:
                lines.append(f"{pad}- {json.dumps(item)}")
        return '\n'.join(lines)
    else:
        return f"{pad}{json.dumps(obj)}"


def dump_yaml(obj, fp):
    fp.write(_to_yaml(obj))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--json-out', help='Write openapi JSON to this path')
    parser.add_argument('--yaml-out', help='Write openapi YAML to this path')
    args = parser.parse_args()

    annotations = collect_annotations()
    api = build_openapi(annotations)

    if args.json_out:
        Path(args.json_out).write_text(json.dumps(api, indent=2))

    if args.yaml_out:
        with open(args.yaml_out, 'w', encoding='utf-8') as f:
            dump_yaml(api, f)

    if not args.json_out and not args.yaml_out:
        json.dump(api, sys.stdout, indent=2)


if __name__ == '__main__':
    main()
