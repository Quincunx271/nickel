#!/usr/bin/env python3
import pathlib
import functools
import sys

import semver

# Run from root of gh-pages branch
cwd = pathlib.Path.cwd()
published_versions = [x.name for x in cwd.iterdir() if x.is_dir()]

# We explicitly want this at the front of the list, so remove for now
if 'main' in published_versions: published_versions.remove('main')
if '.git' in published_versions: published_versions.remove('.git')
published_versions = sorted(
    published_versions,
    key=functools.cmp_to_key(semver.compare),
    reverse=True
)

published_versions = [[version, version] for version in published_versions]
versions = [
    ['main', 'git-main'],
]
if published_versions: versions.append([published_versions[0][0], 'latest'])
versions += published_versions

with open(sys.argv[1], 'r') as f:
    version_selector_template = f.read()
    version_selector_template = version_selector_template.replace(
        'const versions = []',
        'const versions = ' + str(versions))

    with open('version-selector.js', 'w') as f:
        f.write(version_selector_template)

# We also want to update the root index.html file to redirect to the new version
with open(sys.argv[2], 'r') as f:
    index_html_template = f.read()
    index_html_template = index_html_template.replace('{{latest_version}}', published_versions[0][0])

    with open('index.html', 'w') as f:
        f.write(index_html_template)
