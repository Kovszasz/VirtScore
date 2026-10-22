"""Avoid aqt mistaking Qt's 7z archives with embedded ZIP data for ZIP files.

zipfile.is_zipfile() checks the archive's tail and can return True for qttools.
Check the 7z signature first so aqt uses py7zr for those archives.
"""
from pathlib import Path

import aqt.installer

installer = Path(aqt.installer.__file__)
original = "elif zipfile.is_zipfile(archive):"
replacement = "elif not py7zr.is_7zfile(archive) and zipfile.is_zipfile(archive):"
source = installer.read_text(encoding="utf-8")
if original in source:
    installer.write_text(source.replace(original, replacement), encoding="utf-8")
elif replacement not in source:
    raise RuntimeError("aqt archive detection changed; review the Qt archive workaround")
