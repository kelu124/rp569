#!/bin/sh
# Enable the repo's git hooks (.githooks/): command sync check + patch version bump on every commit.
set -e
cd "$(dirname "$0")/.."
git config core.hooksPath .githooks
echo "git hooks enabled: $(git config core.hooksPath)"
