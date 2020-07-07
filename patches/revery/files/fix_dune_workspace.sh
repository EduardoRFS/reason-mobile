#! /bin/sh

set -e
set -u

cat > dune-workspace <<EOF
(lang dune 2.0)
(context (default
 (targets $ESY_TOOLCHAIN)
 (disable_dynamically_linked_foreign_archives true)))
EOF
