import subprocess
import resource
import sys
import argparse

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--timeout', type=int, required=True, help='Timeout in seconds')
    parser.add_argument('cmd', nargs='+', help='Command to run')
    args = parser.parse_args()

    subprocess.run(args.cmd, timeout=args.timeout, check=True, stdout=sys.stderr)
    stats = resource.getrusage(resource.RUSAGE_CHILDREN)
    print(f'{{ "time": {stats.ru_utime}, "memory": {stats.ru_maxrss} }}')
