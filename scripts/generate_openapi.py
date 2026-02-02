#!/usr/bin/env python3
"""
Simple OpenAPI generator for @openapi annotations in C++ sources

Scans the src/ and include/ folders for lines containing
  // @openapi {METHOD} /path summary="..." [requestBody=TYPE] responses=CODE:CONTENTTYPE[,...]

and emits openapi.json (v3) to stdout
"""
import re
import json
import sys
import argparse
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PATTERN = re.compile(r"@openapi\s+\{(?P<method>\w+)\}\s+(?P<path>\S+)(?:\s+summary=\"(?P<summary>[^\"]*)\")?(?:\s+requestBody=(?P<requestBody>\S+))?(?:\s+responses=(?P<responses>[^\n]+))?")


def collect_annotations():
    annotations = []
    for folder in (ROOT / 'src', ROOT / 'include'):
        if not folder.exists():
            continue
        for p in folder.rglob('*.cpp'):
            text = p.read_text(errors='ignore')
            for m in PATTERN.finditer(text):
                d = m.groupdict()
                d['file'] = str(p.relative_to(ROOT))
                annotations.append(d)
        for p in folder.rglob('*.h'):
            text = p.read_text(errors='ignore')
            for m in PATTERN.finditer(text):
                d = m.groupdict()
                d['file'] = str(p.relative_to(ROOT))
                annotations.append(d)
    return annotations


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
    }

    for a in annotations:
        path = a['path']
        method = a['method'].lower()
        summary = a.get('summary') or ''
        requestBody = a.get('requestBody')
        responses = a.get('responses') or '200:application/json'

        path_obj = api['paths'].setdefault(path, {})
        op = {'summary': summary, 'operationId': f"op_{method}_{path.strip('/').replace('/', '_')}", 'responses': {}}

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
            op['requestBody'] = {
                'content': {media: {'schema': {'type': 'object'}}},
                'required': True,
            }

        path_obj[method] = op

    return api


def _to_yaml(obj, indent=0):
    pad = '  ' * indent
    if isinstance(obj, dict):
        lines = []
        for k, v in obj.items():
            if isinstance(v, (dict, list)):
                lines.append(f"{pad}{k}:")
                lines.append(_to_yaml(v, indent + 1))
            else:
                # use JSON dumps for safe scalar representation
                lines.append(f"{pad}{k}: {json.dumps(v)}")
        return '\n'.join(lines)
    elif isinstance(obj, list):
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
