#!/bin/sh
#
# Download the CloudDrive2 core package from the upstream GitHub releases
# and stage it under opt/clouddrive2/ so that dpkg-buildpackage can pack
# it into the architecture-specific .deb.
#
# Usage:
#   ./build-deb.sh [stable|preview] [deb-architecture] [explicit-tag]
#
# Defaults: channel=stable, architecture=amd64, tag=resolved from GitHub.
#
# This file is part of OpenMediaVault CloudDrive2 plugin packaging.
# @license https://www.gnu.org/licenses/gpl.html GPL Version 3

set -eu

REPO="cloud-fs/cloud-fs.github.io"
API="https://api.github.com/repos/${REPO}/releases"
CHANNEL="${1:-stable}"
ARCH="${2:-amd64}"
TAG="${3:-}"

# Optional GitHub token to avoid the anonymous API rate limit in CI.
if [ -n "${GH_TOKEN:-}" ]; then
	AUTH_HEADER="Authorization: Bearer ${GH_TOKEN}"
else
	AUTH_HEADER=""
fi
api_get() {
	if [ -n "$AUTH_HEADER" ]; then
		curl -fsSL -H "$AUTH_HEADER" "$@"
	else
		curl -fsSL "$@"
	fi
}

# Map Debian architecture to the CloudDrive2 upstream architecture token.
case "$ARCH" in
	amd64) UARCH="x86_64" ;;
	arm64) UARCH="aarch64" ;;
	armhf) UARCH="armv7" ;;
	*)
		echo "ERROR: unsupported architecture '$ARCH' (use amd64|arm64|armhf)" >&2
		exit 1
	;;
esac

# Resolve the release tag for the requested channel when not given.
# - stable : the latest release that is NOT a prerelease.
# - preview: the latest release that IS a prerelease (falls back to the
#            latest release if none is marked prerelease).
# The GitHub API returns releases newest-first as pretty-printed JSON
# with one field per line, so a small awk pass over tag_name/prerelease
# (skipping drafts) is enough.
if [ -z "$TAG" ]; then
	echo "Resolving '${CHANNEL}' release tag from GitHub ..."
	if [ "$CHANNEL" = "preview" ]; then
		WANT="true"
	else
		WANT="false"
	fi
	TAG=$(api_get "${API}" \
		| awk '
			/"tag_name":/   { gsub(/.*"tag_name": *"|",?$/, ""); tag=$0 }
			/"draft":/      { gsub(/.*"draft": *|,?$/, ""); draft=$0 }
			/"prerelease":/ {
				gsub(/.*"prerelease": *|,?$/, ""); pre=$0
				if (draft != "true" && pre == want) { print tag; exit }
			}
		' want="$WANT" \
		| head -n 1)
	if [ -z "$TAG" ]; then
		echo "ERROR: could not resolve a '${CHANNEL}' release tag" >&2
		exit 1
	fi
fi

# The version without the leading 'v' (used for the asset filename).
VERSION="${TAG#v}"
ASSET="clouddrive-2-linux-${UARCH}-${VERSION}.tgz"
URL="https://github.com/${REPO}/releases/download/${TAG}/${ASSET}"

echo "Channel    : ${CHANNEL}"
echo "Tag        : ${TAG}"
echo "Version    : ${VERSION}"
echo "Debian arch: ${ARCH} (upstream: ${UARCH})"
echo "Asset      : ${ASSET}"
echo "URL        : ${URL}"

WORKDIR="$(mktemp -d)"
trap 'rm -rf "$WORKDIR"' EXIT

echo "Downloading ..."
curl -fL --retry 5 --retry-delay 2 -o "${WORKDIR}/${ASSET}" "$URL"

echo "Extracting ..."
tar xzf "${WORKDIR}/${ASSET}" -C "$WORKDIR"

# The archive contains a single top-level directory named after the asset.
SRC="${WORKDIR}/clouddrive-2-linux-${UARCH}-${VERSION}"
if [ ! -d "$SRC" ]; then
	# Fall back to whatever single directory was extracted.
	SRC=$(find "$WORKDIR" -mindepth 1 -maxdepth 1 -type d | head -n 1)
fi
if [ ! -f "${SRC}/clouddrive" ] || [ ! -d "${SRC}/wwwroot" ]; then
	echo "ERROR: unexpected archive layout in ${SRC}" >&2
	ls -la "$SRC" >&2 || true
	exit 1
fi

DEST="$(dirname "$0")/opt/clouddrive2"
echo "Staging into ${DEST} ..."
rm -rf "$DEST"
mkdir -p "$DEST"
cp -a "${SRC}/clouddrive" "$DEST/clouddrive"
cp -a "${SRC}/wwwroot" "$DEST/wwwroot"
chmod 0755 "$DEST/clouddrive"
printf '%s\n' "$VERSION" > "$DEST/VERSION"

echo "Done. Staged files:"
ls -la "$DEST"
