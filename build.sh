#!/usr/bin/env bash
# ==========================================================
# Edge Gateway build helper (Linux/WSL)
#   build.sh [--debug|--release] [--arch x64|x86] [--target linux|windows] [--clean] [--docs]
# ==========================================================
set -euo pipefail

# ---- defaults ----
BUILD_TYPE="debug"
ARCH="x64"
TARGET="linux"
CLEAN=""
BUILD_DOCS=""

# ---- parse CLI flags ----
while [[ $# -gt 0 ]]; do
  case "$1" in
    --debug)    BUILD_TYPE="debug"; shift ;;
    --release)  BUILD_TYPE="release"; shift ;;
    --arch)     ARCH="${2:-}"; shift 2 ;;
    --target)   TARGET="${2:-}"; shift 2 ;;
    --clean)    CLEAN="1"; shift ;;
    --docs)     BUILD_DOCS="1"; shift ;;
    *) echo "Unknown option $1"; exit 1 ;;
  esac
done

# ---- paths ----
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd -P)"
ROOT_DIR="$SCRIPT_DIR"

# ---- compose preset name from CMakePresets.json ----
if [[ "$TARGET" == "linux" ]]; then
  CONFIGURE_PRESET="linux-$ARCH"
  BUILD_PRESET="linux-$ARCH-$BUILD_TYPE"
elif [[ "$TARGET" == "windows" ]]; then
  CONFIGURE_PRESET="windows-$ARCH"
  BUILD_PRESET="windows-$ARCH-$BUILD_TYPE"
  echo "Info: Cross-compiling for Windows using MinGW toolchain."
  echo "Note: Ensure MinGW toolchain is installed (e.g., apt install mingw-w64)."
else
  echo "Error: Unsupported target '$TARGET'. Supported: linux, windows"
  exit 1
fi

BUILD_DIR="$ROOT_DIR/out/build/$CONFIGURE_PRESET"

echo
echo "****  Building using configure preset '$CONFIGURE_PRESET' and build preset '$BUILD_PRESET' for $TARGET  ****"
echo

# ---- Check if vcpkg submodule exists ----
if [[ ! -f "$ROOT_DIR/vcpkg/scripts/buildsystems/vcpkg.cmake" ]]; then
  echo "Error: vcpkg not found in 'vcpkg/'."
  echo "Please run: git submodule update --init --recursive"
  exit 1
fi

# ---- bootstrap vcpkg (first run) ----
if [[ ! -f "$ROOT_DIR/vcpkg/vcpkg" ]]; then
  echo "Bootstrapping vcpkg..."
  pushd "$ROOT_DIR/vcpkg" >/dev/null
  ./bootstrap-vcpkg.sh || { echo "ERROR: Failed to bootstrap vcpkg"; popd >/dev/null; exit 1; }
  popd >/dev/null
  echo "vcpkg bootstrapped successfully."
fi

# ---- optional clean ----
if [[ -n "${CLEAN}" ]]; then
  echo "Cleaning $BUILD_DIR ..."
  rm -rf "$BUILD_DIR" || true
fi

# ---- runtime env helpers (informational) ----
if [[ "$TARGET" == "linux" ]]; then
  if [[ "$ARCH" == "x86" ]]; then
    TRIPLET="x86-linux"
  else
    TRIPLET="x64-linux"
  fi
else
  if [[ "$ARCH" == "x86" ]]; then
    TRIPLET="x86-windows"
  else
    TRIPLET="x64-windows"
  fi
fi
# Note: Triplet is already configured in CMakePresets.json for Linux builds.

# ---- configure using CMake presets ----
echo "Configuring with CMake preset: $CONFIGURE_PRESET"
cmake --preset="$CONFIGURE_PRESET"

# ---- build using CMake presets ----
echo "Building with CMake preset: $BUILD_PRESET"
cmake --build --preset "$BUILD_PRESET"

# ---- optional docs ----
if [[ -n "${BUILD_DOCS}" ]]; then
  echo "Docs build requested, but no docs target is wired yet."
  echo "Add a docs target to CMake and extend this script if needed."
fi
