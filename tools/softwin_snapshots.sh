#!/bin/bash
mkdir -p "$2"
out="$(cd "$2" && pwd)"
cd "$1/tests" || exit 1
for style in 0 1; do
  sname=original
  [ "$style" = 1 ] && sname=boxes
  for scale in 0.75 1 1.25 1.5 2; do
    for skin in Original Dark Warm; do
      args=(--scale "$scale" --style "$style")
      [ "$skin" != Original ] && args+=(--skin "./dtblkfx/skins/$skin")
      ./dtblkfix_gui_snapshot . "$out/soft-$sname-$skin-$scale.png" "${args[@]}" > /dev/null || echo "FAILED $sname $skin $scale"
    done
  done
done
