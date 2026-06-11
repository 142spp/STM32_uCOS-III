#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/../.."

SRC="assign/term/termproject_initial.md"
OUT="assign/term/termproject_initial_eisvogel.pdf"

pandoc "$SRC" \
  -o "$OUT" \
  --template eisvogel \
  --pdf-engine=xelatex \
  -V mainfont="Noto Sans CJK KR" \
  -V sansfont="Noto Sans CJK KR" \
  -V monofont="Noto Sans Mono CJK KR" \
  -V geometry:margin=20mm

echo "Wrote $OUT"
