import sys
import os
import subprocess

def run(cmd):
    try:
        return subprocess.check_output(cmd, stderr=subprocess.STDOUT).decode().strip()
    except subprocess.CalledProcessError:
        return ""

if __name__ == '__main__':
    # Try to describe (use --always to fallback to commit if no tags)
    tag = run(['git', 'describe', '--tags', '--dirty', '--always'])

    if not tag:
        # Fetch tags/history then retry
        run(['git', 'fetch', '--tags', '--prune'])
        tag = run(['git', 'describe', '--tags', '--dirty', '--always'])

        if not tag:
            # Final fallback to short commit id
            tag = run(['git', 'rev-parse', '--short', 'HEAD'])

    # Default fallback if everything fails
    tag = tag or 'v0.0.0'

    # The original script assumes tags start with 'v' and strips it
    version = str(tag[1:]) if tag.startswith('v') else tag

    version_file = os.path.abspath(os.path.join(os.path.dirname(__file__), "../QtScrcpy/appversion"))
    with open(version_file, 'w') as file:
        file.write(version)
    sys.exit(0)
