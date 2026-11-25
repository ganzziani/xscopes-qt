"""Writes the notices of the third-party code inside the Qt libraries that the Windows build
ships (Qt-third-party.txt in the installed licenses folder). build-installer.bat runs it for
every installer, with the Qt version of the kit it builds with, so the notices always match
the Qt DLLs in the payload.

    python qt-third-party.py --version 6.12.0 --docs C:/Qt/Docs --out <file>

The source is the attribution pages in Qt's offline documentation (the Qt installer's
documentation component), as Qt's "Licenses Used in Qt" gives them.
"""
import argparse
import html
import os
import re
import sys

# Modules whose DLLs or plugins windeployqt ships (Core, Gui + its plugins, Widgets, Network).
# Qt Widgets, Qt Svg and Qt Serial Port have no third-party attributions.
MODULES = ['qtcore', 'qtgui', 'qtnetwork']
# Not part of the Windows binaries: build scripts, other platforms, fonts that are not deployed.
SKIP = {
    'android-gradle-wrapper', 'extra-cmake-modules', 'kwin', 'forkfd', 'qeventdispatcher-cf',
    'android-native-style', 'cocoa-platform-plugin', 'dejayvu', 'xcb-xinput', 'xserverhelper',
}

HEADER = """Third-party code inside the Qt libraries
========================================

The Qt {v} libraries and plugins shipped with the XScopes Interface (Qt6Core, Qt6Gui,
Qt6Widgets, Qt6Network, Qt6Svg, Qt6SerialPort and the plugins next to them) contain the
third-party code listed below. Its copyright notices and licences follow, as Qt's own
documentation gives them ("Licenses Used in Qt", Qt {v}). Components that Qt uses only
on other platforms, or only to build Qt, are left out.

"""


def text(fragment):
    fragment = re.sub(r'<[^>]+>', '', fragment)
    return html.unescape(fragment).strip()


def render(path):
    s = open(path, encoding='utf-8').read()
    title = text(re.search(r'<h1 class="title">(.*?)</h1>', s, re.S).group(1))
    body = s[s.find('<div class="descr" id="details">'):]
    body = body[:body.find('<!-- @@@')] if '<!-- @@@' in body else body
    out = [title, '-' * len(title)]
    for kind, content in re.findall(r'<(p|pre)[^>]*>(.*?)</\1>', body, re.S):
        if kind == 'p':
            t = re.sub(r'\s+', ' ', text(content))
            if t.startswith('The sources can be found'):
                continue
            out.append(t)
        else:
            lines = html.unescape(re.sub(r'<[^>]+>', '', content)).rstrip().split('\n')
            out.append('\n'.join(('    ' + (l[1:] if l.startswith(' ') else l)).rstrip()
                                 for l in lines))
        out.append('')
    return '\n'.join(out).rstrip() + '\n'


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('--version', required=True, help='Qt version, e.g. 6.12.0')
    ap.add_argument('--docs', required=True, help='Qt documentation root, e.g. C:/Qt/Docs')
    ap.add_argument('--out', required=True, help='file to write')
    args = ap.parse_args()

    docs = os.path.join(args.docs, 'Qt-' + args.version)
    sections = []
    for module in MODULES:
        folder = os.path.join(docs, module)
        if not os.path.isdir(folder):
            sys.exit('No Qt %s documentation at %s (install it with the Qt Maintenance Tool)'
                     % (args.version, folder))
        for name in sorted(os.listdir(folder)):
            m = re.match(module + r'-attribution-(.+)\.html$', name)
            if not m or m.group(1) in SKIP:
                continue
            sections.append(render(os.path.join(folder, name)))
    if not sections:
        sys.exit('No attribution pages found under ' + docs)
    with open(args.out, 'w', encoding='utf-8', newline='\n') as f:
        f.write(HEADER.format(v=args.version) + '\n\n'.join(sections))
    print('%d components' % len(sections))


main()
