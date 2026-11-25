#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""The step-by-step setup guide: site/guide/*.md made into the site's guide/*.html, with the standard
library only (site/build.py calls build()).

Each page is Markdown with what the guide needs and nothing more: headings (an id of its own as
`## Title {#id}`, else one made from the title), paragraphs, **bold**, *italics*, `code`, links, lists
(numbered or not, nested by indenting), fenced code, tables, `---`, and GitHub's notes (`> [!NOTE]`,
`[!TIP]`, `[!IMPORTANT]`, `[!WARNING]`, `[!CAUTION]`). A link to another page is written `install.md`
(it works on GitHub too) and becomes `install.html`.

PAGES is the guide's order: its sidebar, and each page's "Continue to" the next. The build stops on a
link to a page or a heading that is not there, and on any page the app's Setup check opens that is not
there (port/frontend/shell/setup.cpp's Guide("page#id")), so its QR codes never open a missing page.

    python3 site/guide.py [--out DIR]     the guide alone, into DIR (default _site/guide)
"""

import html
import os
import re
import sys

SITE = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.dirname(SITE)
SOURCE = os.path.join(SITE, "guide")
SETUP = os.path.join(REPO_ROOT, "port/frontend/shell/setup.cpp")
DISCORD = "https://discord.gg/grrqkv4xWw"
REPO = "https://github.com/premohq/PlayTendoh"

# (page, its name in the sidebar, its group); the first group's pages are the numbered steps
PAGES = [
    ("index", "Start here", "Set up"),
    ("get-started", "Get started", "Set up"),
    ("jailbreak", "Jailbreak and HEN", "Set up"),
    ("install", "Install Playtendoh", "Set up"),
    ("setup-check", "The first start", "Set up"),
    ("files", "Where your files go", "Set up"),
    ("wii-u", "Wii U", "Your systems"),
    ("3ds", "Nintendo 3DS", "Your systems"),
    ("ds", "Nintendo DS", "Your systems"),
    ("switch", "Nintendo Switch", "Your systems"),
    ("switch-2", "Switch 2", "Your systems"),
    ("gamecube-wii", "GameCube and Wii", "Your systems"),
    ("classics", "The classics", "Your systems"),
    ("updating", "Updating", "When something's wrong"),
    ("troubleshooting", "Troubleshooting", "When something's wrong"),
    ("help", "Getting help", "When something's wrong"),
]

NOTES = {"NOTE": "Note", "TIP": "Tip", "IMPORTANT": "Important", "WARNING": "Warning", "CAUTION": "Caution"}
HEADING = re.compile(r"^(#{1,4})\s+(.+?)(?:\s+\{#([A-Za-z0-9_-]+)\})?\s*$")
LIST = re.compile(r"^(\s*)([-*]|\d+\.)\s+(.*)$")
FENCE = re.compile(r"^\s*```")
RULE = re.compile(r"^\s*---+\s*$")
TABLE_RULE = re.compile(r"^\s*\|?\s*:?-{3,}:?\s*(\|\s*:?-{3,}:?\s*)*\|?\s*$")


def slug(text):
    text = re.sub(r"<[^>]+>", "", text)
    text = html.unescape(text).lower()
    text = re.sub(r"[^a-z0-9\s-]", "", text)
    return re.sub(r"[\s-]+", "-", text).strip("-")


def indent_of(line):
    return len(line) - len(line.lstrip(" "))


class Page:
    def __init__(self, name):
        self.name = name
        self.title = ""
        self.ids = set()
        self.links = []  # (page, id) of the guide's own links, to check

    # -- inline ---------------------------------------------------------------------------------------

    def href(self, url):
        url = html.unescape(url)
        if re.match(r"^[a-z]+:", url) or url.startswith("//"):
            return url
        path, _, anchor = url.partition("#")
        if path.endswith(".md"):
            path = path[:-3] + ".html"
        if not path or (path.endswith(".html") and "/" not in path):
            self.links.append(((path[:-5] if path else self.name), anchor))
        return path + ("#" + anchor if anchor else "")

    def inline(self, text):
        codes = []

        def keep(m):
            codes.append(m.group(1))
            return f"\x00{len(codes) - 1}\x00"

        text = re.sub(r"`([^`]+)`", keep, text)
        text = html.escape(text, quote=False)

        def link(m):
            url = self.href(m.group(2))
            outside = re.match(r"^https?:", url) is not None
            rel = ' rel="noopener"' if outside else ""
            return f'<a href="{html.escape(url)}"{rel}>{m.group(1)}</a>'

        text = re.sub(r"\[([^\]]+)\]\(([^)\s]+)\)", link, text)
        text = re.sub(r"\*\*(.+?)\*\*", r"<strong>\1</strong>", text)
        text = re.sub(r"(?<![\w*])\*(?![\s*])(.+?)(?<![\s*])\*(?![\w*])", r"<em>\1</em>", text)
        # a path may break after a slash, and nowhere else
        return re.sub(r"\x00(\d+)\x00",
                      lambda m: "<code>" + html.escape(codes[int(m.group(1))], quote=False).replace("/", "/<wbr>") + "</code>", text)

    # -- blocks ---------------------------------------------------------------------------------------

    def blocks(self, lines):
        out = []
        i = 0
        while i < len(lines):
            line = lines[i]
            if not line.strip():
                i += 1
            elif FENCE.match(line):
                pad = indent_of(line)
                code = []
                i += 1
                while i < len(lines) and not FENCE.match(lines[i]):
                    code.append(lines[i][pad:] if indent_of(lines[i]) >= pad else lines[i].lstrip(" "))
                    i += 1
                i += 1
                out.append("<pre><code>" + html.escape("\n".join(code), quote=False) + "</code></pre>")
            elif m := HEADING.match(line):
                level = len(m.group(1))
                text = self.inline(m.group(2))
                ident = m.group(3) or slug(text)
                if level == 1:
                    self.title = re.sub(r"<[^>]+>", "", text)
                    i += 1
                    continue
                if ident in self.ids:
                    sys.exit(f"guide: {self.name}.md has two headings with the id {ident}")
                self.ids.add(ident)
                out.append(f'<h{level} id="{ident}"><a class="anchor" href="#{ident}" aria-hidden="true">#</a>{text}</h{level}>')
                i += 1
            elif RULE.match(line):
                out.append("<hr>")
                i += 1
            elif line.lstrip().startswith("|") and i + 1 < len(lines) and TABLE_RULE.match(lines[i + 1]):
                rows = []
                while i < len(lines) and lines[i].lstrip().startswith("|"):
                    if not TABLE_RULE.match(lines[i]):
                        rows.append([c.strip() for c in lines[i].strip().strip("|").split("|")])
                    i += 1
                head = "".join(f"<th>{self.inline(c)}</th>" for c in rows[0])
                body = "".join("<tr>" + "".join(f"<td>{self.inline(c)}</td>" for c in row) + "</tr>" for row in rows[1:])
                out.append(f'<div class="table"><table><thead><tr>{head}</tr></thead><tbody>{body}</tbody></table></div>')
            elif line.lstrip().startswith(">"):
                quoted = []
                while i < len(lines) and lines[i].lstrip().startswith(">"):
                    quoted.append(re.sub(r"^\s*>\s?", "", lines[i]))
                    i += 1
                kind = re.match(r"^\[!([A-Z]+)\]\s*$", quoted[0]) if quoted else None
                if kind and kind.group(1) in NOTES:
                    name = kind.group(1)
                    out.append(f'<div class="note-box {name.lower()}"><p class="note-title">{NOTES[name]}</p>'
                               + self.blocks(quoted[1:]) + "</div>")
                else:
                    out.append("<blockquote>" + self.blocks(quoted) + "</blockquote>")
            elif LIST.match(line):
                html_list, i = self.list(lines, i)
                out.append(html_list)
            else:
                para = []
                while i < len(lines) and lines[i].strip() and not (
                        FENCE.match(lines[i]) or HEADING.match(lines[i]) or LIST.match(lines[i]) or RULE.match(lines[i])
                        or lines[i].lstrip().startswith(("|", ">"))):
                    para.append(lines[i].strip())
                    i += 1
                out.append("<p>" + self.inline(" ".join(para)) + "</p>")
        return "\n".join(out)

    def list(self, lines, i):
        first = LIST.match(lines[i])
        base = len(first.group(1))
        ordered = first.group(2)[0].isdigit()
        start = int(first.group(2)[:-1]) if ordered else 1
        items = []
        while i < len(lines):
            m = LIST.match(lines[i])
            if not m or len(m.group(1)) != base or m.group(2)[0].isdigit() != ordered:
                break
            column = len(m.group(1)) + len(m.group(2)) + 1
            body = [m.group(3)]
            i += 1
            while i < len(lines):
                if not lines[i].strip():
                    j = i
                    while j < len(lines) and not lines[j].strip():
                        j += 1
                    if j < len(lines) and indent_of(lines[j]) > base:
                        body.extend([""] * (j - i))
                        i = j
                        continue
                    break
                if indent_of(lines[i]) > base:
                    body.append(lines[i][min(column, indent_of(lines[i])):])
                    i += 1
                    continue
                break
            inner = self.blocks(body)
            # one paragraph alone is the item's text, without a paragraph around it
            if inner.startswith("<p>") and inner.count("<p>") == 1 and inner.endswith("</p>"):
                inner = inner[3:-4]
            elif inner.startswith("<p>") and inner.count("<p>") == 1:
                inner = inner[3:].replace("</p>", "", 1)
            items.append(inner)
            j = i
            while j < len(lines) and not lines[j].strip():
                j += 1
            following = LIST.match(lines[j]) if j < len(lines) else None
            if following and len(following.group(1)) == base and following.group(2)[0].isdigit() == ordered:
                i = j
            else:
                break
        tag = "ol" if ordered else "ul"
        attributes = f' start="{start}"' if ordered and start != 1 else ""
        return f"<{tag}{attributes}>" + "".join(f"<li>{item}</li>" for item in items) + f"</{tag}>", i


def sidebar(current):
    groups = []
    for page, name, group in PAGES:
        if not groups or groups[-1][0] != group:
            groups.append((group, []))
        here = ' aria-current="page"' if page == current else ""
        groups[-1][1].append(f'<li><a href="{page}.html"{here}>{html.escape(name)}</a></li>')
    return "".join(f'<p class="toc-group">{html.escape(g)}</p><ul>{"".join(items)}</ul>' for g, items in groups)


def template(page, title, body, description):
    index = [p[0] for p in PAGES].index(page)
    name, group = PAGES[index][1], PAGES[index][2]
    steps = [p[0] for p in PAGES if p[2] == PAGES[0][2]]
    kicker = f"Step {steps.index(page)} of {len(steps) - 1}" if page in steps and page != "index" else (
        "The Playtendoh guide" if page == "index" else group)
    previous = PAGES[index - 1] if index > 0 else None
    following = PAGES[index + 1] if index + 1 < len(PAGES) else None
    pager = '<nav class="guide-pager" aria-label="Previous and next">'
    pager += (f'<a class="btn" href="{previous[0]}.html"><span aria-hidden="true">←</span> {html.escape(previous[1])}</a>'
              if previous else "<span></span>")
    if following:
        pager += f'<a class="btn primary" href="{following[0]}.html">Continue to {html.escape(following[1])} <span aria-hidden="true">→</span></a>'
    pager += "</nav>"
    toc = sidebar(page)
    return f"""<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{html.escape(title)} · The Playtendoh guide</title>
<meta name="description" content="{html.escape(description)}">
<meta http-equiv="Content-Security-Policy" content="default-src 'self'; script-src 'self'; style-src 'self'; img-src 'self' data: https:; font-src 'self'; base-uri 'none'; form-action 'none'; object-src 'none'">
<meta name="referrer" content="strict-origin-when-cross-origin">
<meta name="color-scheme" content="light dark">
<meta property="og:type" content="article">
<meta property="og:site_name" content="Playtendoh">
<meta property="og:title" content="{html.escape(title)} · The Playtendoh guide">
<meta property="og:description" content="{html.escape(description)}">
<meta property="og:image" content="https://premohq.github.io/PlayTendoh/banner.png">
<link rel="icon" href="../mark.svg" type="image/svg+xml">
<link rel="icon" href="../icon.png" type="image/png" sizes="512x512">
<link rel="stylesheet" href="../style.css">
<link rel="stylesheet" href="../guide.css">
<script src="../theme.js"></script>
</head>
<body class="guide">
<a class="skip" href="#content">Skip to the content</a>
<header class="nav">
  <div class="wrap nav-row">
    <a class="brand" href="../"><img class="brand-mark" src="../mark.svg" alt=""><span>Playtendoh</span></a>
    <nav class="guide-links" aria-label="Site">
      <a href="index.html"{' aria-current="page"' if page == "index" else ""}>Guide</a>
      <a href="troubleshooting.html">Troubleshooting</a>
      <a href="../compatibility.html">Compatibility</a>
      <a href="../#release">Download</a>
      <a href="{DISCORD}">Discord</a>
    </nav>
  </div>
</header>
<div class="wrap guide-layout">
  <nav class="guide-toc" aria-label="The guide's pages">{toc}</nav>
  <main id="content" class="guide-main">
    <details class="guide-toc-small"><summary>All of the guide's pages</summary>{toc}</details>
    <p class="guide-kicker">{html.escape(kicker)}</p>
    <h1>{html.escape(title)}</h1>
{body}
    {pager}
    <p class="guide-foot">Something on this page wrong, out of date or unclear? <a href="{REPO}/edit/Playtendoh/site/guide/{page}.md">Fix it on GitHub</a>, or tell us on the <a href="{DISCORD}">Discord</a>.</p>
  </main>
</div>
<footer class="footer">
  <div class="wrap">
    <p>Playtendoh is an independent homebrew project, not affiliated with or endorsed by Sony Interactive Entertainment or Nintendo. No firmware, keys, games or BIOS files are included, and this guide never links to them: dump your own, from consoles and games you own.</p>
  </div>
</footer>
</body>
</html>
"""


def app_targets():
    """The pages and headings the app's Setup check opens: Guide("page#id") in setup.cpp."""
    try:
        with open(SETUP, encoding="utf-8") as f:
            text = f.read()
    except OSError:
        return []
    # every page a call names, those it chooses between too: Guide(n3ds ? "3ds#games" : "wii-u#games")
    calls = re.findall(r"\bGuide\(([^;\n]*)\)\s*;", text)
    return [t.partition("#")[::2] for call in calls for t in re.findall(r'"([^"]+)"', call)]


def build(out):
    os.makedirs(out, exist_ok=True)
    pages = {}
    for page, _, _ in PAGES:
        path = os.path.join(SOURCE, page + ".md")
        with open(path, encoding="utf-8") as f:
            text = f.read().replace("\r\n", "\n")
        doc = Page(page)
        body = doc.blocks(text.split("\n"))
        if not doc.title:
            sys.exit(f"guide: {page}.md has no title (a '# ' heading)")
        first = re.search(r"<p>(.*?)</p>", body, re.S)
        description = re.sub(r"<[^>]+>", "", first.group(1)) if first else doc.title
        pages[page] = (doc, body, html.unescape(re.sub(r"\s+", " ", description))[:200])
    missing = []
    for page, (doc, _, _) in pages.items():
        for target, anchor in doc.links:
            if target not in pages:
                missing.append(f"{page}.md links to {target}.md, which is not a page of the guide")
            elif anchor and anchor not in pages[target][0].ids:
                missing.append(f"{page}.md links to {target}.md#{anchor}, which has no such heading")
    for target, anchor in app_targets():
        if target not in pages:
            missing.append(f"setup.cpp opens guide/{target}, which is not a page of the guide")
        elif anchor and anchor not in pages[target][0].ids:
            missing.append(f"setup.cpp opens guide/{target}#{anchor}, which has no such heading")
    if missing:
        sys.exit("guide:\n  " + "\n  ".join(missing))
    for page, (doc, body, description) in pages.items():
        with open(os.path.join(out, page + ".html"), "w", encoding="utf-8", newline="\n") as f:
            f.write(template(page, doc.title, body, description))
    return len(pages)


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--out", default=os.path.join(REPO_ROOT, "_site", "guide"))
    args = parser.parse_args()
    print(f"guide.py: {build(args.out)} pages in {args.out}")
