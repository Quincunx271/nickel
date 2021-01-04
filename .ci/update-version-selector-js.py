#!/usr/bin/env python3
import pathlib
import functools
import sys

import semver

# Run from root of gh-pages branch
cwd = pathlib.Path.cwd()
versions = [x.name for x in cwd.iterdir() if x.is_dir()]

# We explicitly want this at the front of the list, so remove for now
versions.remove('main')
versions = sorted(
    versions,
    key=functools.cmp_to_key(semver.compare),
    reverse=True
)

versions = [[version, version] for version in versions]
versions = [
    ['main', 'git-main'],
    *([versions[0][0], 'latest'] if versions else []),
    *versions,
]

with open(sys.argv[1], 'r') as f:
    version_selector_template = f.read()
    version_selector_template = version_selector_template.replace(
        'const versions = []',
        'const versions = ' + str(versions))

    with open('version-selector.js', 'w') as f:
        f.write(version_selector_template)
