import subprocess
import sys

if __name__ == '__main__':
    cmd = sys.argv[2:] + sys.argv[1].split(';')
    subprocess.run(cmd, check=True)
