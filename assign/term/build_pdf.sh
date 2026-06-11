#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/../.."

SRC="assign/term/termproject_initial.md"
OUT="assign/term/termproject_initial.pdf"
STYLE="assign/term/pandoc_style.tex"

pandoc "$SRC" \
  -o "$OUT" \
  --pdf-engine=xelatex \
  -V documentclass=article \
  -V papersize=a4 \
  -V geometry:margin=22mm \
  -V mainfont="Noto Sans CJK KR" \
  -V sansfont="Noto Sans CJK KR" \
  -V monofont="Noto Sans Mono CJK KR" \
  -V colorlinks=true \
  -H "$STYLE"

echo "Wrote $OUT"
