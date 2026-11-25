"""Write the source archive that goes with a release: the repository at HEAD (git archive),
which includes the scripts that build the installer (the GPL's "Corresponding Source" counts
the scripts that control compilation and installation), plus a SOURCE.txt naming the commit.

    make-source-zip.py --repo <repo> --version 2.0.0 --out <file.zip>

Fails when tracked files have uncommitted changes: the archive has to be the source of the
binary built from the same tree.
"""
import argparse
import io
import subprocess
import sys
import zipfile
from pathlib import Path

ap = argparse.ArgumentParser()
ap.add_argument("--repo", required=True)
ap.add_argument("--version", required=True)
ap.add_argument("--out", required=True)
ap.add_argument("--allow-dirty", action="store_true", help="for a test build only")
a = ap.parse_args()

repo = Path(a.repo)
git = ["git", "-C", str(repo)]
dirty = subprocess.run(git + ["status", "--porcelain", "--untracked-files=no"],
                       capture_output=True, text=True, check=True).stdout.strip()
if dirty and not a.allow_dirty:
    sys.exit("ERROR: uncommitted changes in tracked files, commit them first:\n" + dirty)
commit = subprocess.run(git + ["rev-parse", "HEAD"], capture_output=True, text=True, check=True).stdout.strip()
prefix = "xscopes-qt-%s/" % a.version

tree = subprocess.run(git + ["archive", "--format=zip", "--prefix=" + prefix, "HEAD"],
                      capture_output=True, check=True).stdout
with zipfile.ZipFile(io.BytesIO(tree)) as src, \
        zipfile.ZipFile(a.out, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as out:
    for info in src.infolist():
        out.writestr(info, src.read(info))
    out.writestr(prefix + "SOURCE.txt",
                 "XScopes Interface %s\n"
                 "Repository: https://github.com/ganzziani/xscopes-qt\n"
                 "Commit: %s%s\n"
                 "packaging/windows/ holds the scripts that build the Windows installer.\n"
                 % (a.version, commit, " (with uncommitted changes)" if dirty else ""))
print("  source archive: %s (commit %s)" % (a.out, commit[:12]))
