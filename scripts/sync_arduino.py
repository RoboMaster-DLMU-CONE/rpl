import os
import shutil
from pathlib import Path

# Configuration
SOURCE_SINGLE_HEADER = Path("include/RPL/RPL.hpp")
DEST_SRC_DIR = Path("rpl-arduino/src")
DEST_HEADER = DEST_SRC_DIR / "RPL.h"


def clean_dir(path):
    if path.exists():
        shutil.rmtree(path)
    path.mkdir(parents=True, exist_ok=True)


def sync_arduino():
    if not SOURCE_SINGLE_HEADER.exists():
        raise FileNotFoundError(
            f"Single header not found at {SOURCE_SINGLE_HEADER}. "
            "Please run scripts/amalgamate.py first."
        )

    # Clean and recreate src directory in target repo
    clean_dir(DEST_SRC_DIR)

    # Copy single header as RPL.h (Arduino convention uses .h extension)
    shutil.copy2(SOURCE_SINGLE_HEADER, DEST_HEADER)
    print(f"Copied single header to {DEST_HEADER}")


def main():
    sync_arduino()
    print("Arduino synchronization complete!")


if __name__ == "__main__":
    main()
