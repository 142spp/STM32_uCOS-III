#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

pandoc "$SCRIPT_DIR/report.md" \
  -o "$SCRIPT_DIR/report.pdf" \
  --pdf-engine=xelatex \
  --include-in-header "$SCRIPT_DIR/term_style.tex" \
  --resource-path "$SCRIPT_DIR" \
  --toc \
  --toc-depth=2 \
  -V mainfont="Noto Sans CJK KR" \
  -V sansfont="Noto Sans CJK KR" \
  -V monofont="Noto Sans Mono CJK KR" \
  -V documentclass=article \
  -V papersize=a4 \
  -V fontsize=11pt \
  -V geometry:top=24mm \
  -V geometry:bottom=24mm \
  -V geometry:left=25mm \
  -V geometry:right=25mm \
  -V colorlinks=true \
  -V linkcolor=black \
  -V urlcolor=black
