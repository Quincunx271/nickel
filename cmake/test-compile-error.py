import argparse
import subprocess
import sys
import re


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Builds a CMake target that might fail')
    parser.add_argument('--cmake', required=True, help='The ${CMAKE_COMMAND}')
    parser.add_argument('--config', required=True,
                        help='The build configuration')
    parser.add_argument('--binary_dir', required=True,
                        help='The ${CMAKE_BINARY_DIR}')
    parser.add_argument('--target', required=True,
                        help='The CMake target to build')
    parser.add_argument('--file', required=True, help='The source file')

    args = parser.parse_args()

    cmd = [args.cmake, '--build', args.binary_dir,
           '--config', args.config, '--target', args.target]
    print(' '.join(f"'{part}'" for part in cmd), flush=True)
    result = subprocess.run(cmd, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, encoding='utf-8')
    result.stderr = result.stdout

    with open(args.file, 'r') as f:
        contents = f.read()

    error_matches = re.finditer(
        r'// ERROR_MATCHES: (.*)$', contents, re.MULTILINE)
    error_not_matches = re.finditer(
        r'// ERROR_NOT_MATCHES: (.*)$', contents, re.MULTILINE)

    for re_match in error_matches:
        regex = re_match.group(1)
        try:
            m = re.search(regex, result.stderr)
        except:
            print(
                f'Regular Expression: {repr(regex)}', file=sys.stderr)
            raise
        if not m:
            print(
                f'Output did not match when it should have.\n'
                + f'Regular Expression: {regex}\n'
                + 'Stderr:\n'
                + result.stderr, file=sys.stderr)
            exit(1)

    for re_match in error_not_matches:
        regex = re_match.group(1)
        try:
            m = re.search(regex, result.stderr)
        except:
            print(
                f'Regular Expression: {repr(regex)}', file=sys.stderr)
            raise
        if m:
            print(
                f'Output matched when it should NOT have.\n'
                + f'Regular Expression: {regex}\n'
                + f'Match: {m.group()}\n'
                + 'Stderr:\n'
                + result.stderr, file=sys.stderr)
            exit(1)

    if not error_matches and not error_not_matches:
        if result.returncode == 0:
            print(f'Expected compile failure, but compile succeeded.', file=sys.stderr)
            exit(1)
