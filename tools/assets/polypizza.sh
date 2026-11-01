#!/usr/bin/env bash
# usage: polypizza.sh <id>...  -> id | título | autor | licença | glb (lê a página pública do modelo)
for id in "$@"; do
  h=$(curl -sL -A "Mozilla/5.0" "https://poly.pizza/m/$id")
  t=$(echo "$h" | grep -oE '<title>[^<]+' | head -1 | sed 's/<title>//;s/ - Poly Pizza.*//')
  a=$(echo "$h" | grep -oE '"(Creator|creator|author)"[^}]{0,200}' | grep -oE '"(Username|username|name)":"[^"]+"' | head -1 | sed 's/.*":"//;s/"$//')
  l=$(echo "$h" | grep -oE '"(Licence|licence|license)":"[^"]+"' | head -1 | sed 's/.*":"//;s/"$//')
  g=$(echo "$h" | grep -oE 'https://static\.poly\.pizza/[A-Za-z0-9_-]+\.glb' | head -1)
  tris=$(echo "$h" | grep -oE '"(Tris|tris|triCount)":[0-9]+' | head -1 | sed 's/.*://')
  echo "$id | $t | $a | $l | tris=$tris | $g"
done
