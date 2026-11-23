#!/bin/sh
#
# Convert the tutorial notebooks to Jupyter notebooks for the standard
# Python kernel, for publication in kpeeters/cadabra2-colab. The list
# of tutorials is taken from the tutorials page of the web site.
#
# Used by the 'colab' CMake target ('make colab' / 'ninja colab') and
# by .github/workflows/colab.yml.
#
# Usage: colab_notebooks.sh <cadabra2ipynb> <source dir> <output dir>

set -e

if [ $# -ne 3 ]; then
	echo "usage: $0 <cadabra2ipynb> <source dir> <output dir>" >&2
	exit 1
fi

CONVERTER=$1
SRC=$2
OUT=$3

warn() {
	if [ -n "${GITHUB_ACTIONS}" ]; then
		echo "::warning::$1"
	else
		echo "warning: $1"
	fi
}

rm -rf "${OUT}"
mkdir -p "${OUT}"

for name in $(grep -o 'notebooks/[A-Za-z0-9_]*\.cnb' "${SRC}/web2/cadabra2/source/tutorials.html" | sort -u | sed -e 's|notebooks/||' -e 's|\.cnb$||'); do
	if [ -f "${SRC}/examples/${name}.cnb" ]; then
		"${CONVERTER}" --python "${SRC}/examples/${name}.cnb" "${OUT}/${name}.ipynb"
		echo "converted ${name}"
	else
		warn "tutorial ${name}.cnb is listed on the web site but not in examples/"
	fi
done

cat > "${OUT}/README.md" <<EOF
# Cadabra tutorials for Google Colab

Generated automatically from \`examples/*.cnb\` in
[kpeeters/cadabra2](https://github.com/kpeeters/cadabra2) by the workflow
\`.github/workflows/colab.yml\` (or \`make colab\`) there; do not edit this
repository by hand.

Open a notebook with
\`https://colab.research.google.com/github/kpeeters/cadabra2-colab/blob/main/<name>.ipynb\`,
or use the links on <https://cadabra.science/tutorials.html>.
EOF

echo
echo "Notebooks written to ${OUT}"
echo "To publish by hand: cp -r ${OUT}/. <cadabra2-colab checkout>/ and commit there."
