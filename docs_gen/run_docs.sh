#!/bin/sh
set -e

# MkDoxy handles Doxygen generation automatically during build/serve.
# We just need to run mkdocs.

if [ "$1" = "serve" ]; then
    echo "Starting Preview Server at http://localhost:8000..."
    exec mkdocs serve -a 0.0.0.0:8000
else
    echo "Building Static Site..."
    exec mkdocs build
fi
