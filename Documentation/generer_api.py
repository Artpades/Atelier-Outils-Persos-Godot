"""Convertit la référence de classes Godot (doc_classes/*.xml) en données JavaScript pour le site de documentation.

    python Documentation/generer_api.py

Produit Documentation/assets/api-data.js (window.GTK_API), lu par assets/doc.js pour remplir les tableaux
« 💻 API Accessible » de chaque page de module. Les XML restent la source unique de vérité : relancer ce script
après chaque `scons gen_doc` ou modification d'un XML. Le fichier généré ne doit pas être modifié à la main.
"""

import html
import json
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

RACINE = Path(__file__).resolve().parent.parent
DOSSIER_XML = RACINE / "doc_classes"
SORTIE = Path(__file__).resolve().parent / "assets" / "api-data.js"

# Balises BBCode de référence : [member x], [method Classe.x]...
REFERENCE = re.compile(r"\[(member|method|signal|constant|enum|param|annotation|theme_item) ([^\]]+)\]")
# Référence à une classe : [Node2D], [Effect]... (commence par une majuscule, pas d'espace)
CLASSE = re.compile(r"\[([A-Z@][A-Za-z0-9_]*)\]")
STYLES = {"b": "strong", "i": "em", "u": "u", "s": "s", "kbd": "kbd"}


def _en_ligne(texte: str) -> str:
    """BBCode d'une ligne de texte (hors bloc de code) -> HTML."""
    texte = html.escape(texte, quote=False)
    texte = re.sub(r"\[code\](.*?)\[/code\]", r"<code>\1</code>", texte)
    for bb, balise in STYLES.items():
        texte = re.sub(rf"\[{bb}\](.*?)\[/{bb}\]", rf"<{balise}>\1</{balise}>", texte)
    texte = re.sub(r"\[url=(.*?)\](.*?)\[/url\]", r'<a href="\1" target="_blank" rel="noopener">\2</a>', texte)
    texte = re.sub(r"\[url\](.*?)\[/url\]", r'<a href="\1" target="_blank" rel="noopener">\1</a>', texte)
    texte = texte.replace("[br]", "<br>")
    texte = REFERENCE.sub(
        lambda m: f'<code>{m.group(2)}</code>' if m.group(1) == "param"
        else f'<a class="xref" data-kind="{m.group(1)}" data-ref="{m.group(2)}"><code>{m.group(2)}</code></a>',
        texte,
    )
    texte = CLASSE.sub(r'<a class="xref" data-kind="class" data-ref="\1"><code>\1</code></a>', texte)
    return texte


def bbcode_vers_html(texte: str | None) -> str:
    """Description BBCode d'un XML Godot -> fragment HTML (paragraphes, listes « - », blocs de code)."""
    if not texte or not texte.strip():
        return ""
    lignes = texte.strip("\n").split("\n")
    # Retire l'indentation de tabulations commune (indentation XML)
    retrait = min((len(l) - len(l.lstrip("\t")) for l in lignes if l.strip()), default=0)
    lignes = [l[retrait:] for l in lignes]

    morceaux, liste, code = [], [], None
    for ligne in lignes:
        if code is not None:
            if "[/codeblock]" in ligne:
                morceaux.append('<pre><code class="gdscript">' + html.escape("\n".join(code)) + "</code></pre>")
                code = None
            else:
                code.append(ligne)
            continue
        if ligne.strip().startswith("[codeblock"):
            code = []
            continue
        if ligne.strip().startswith("- "):
            liste.append("<li>" + _en_ligne(ligne.strip()[2:]) + "</li>")
            continue
        if liste:
            morceaux.append("<ul>" + "".join(liste) + "</ul>")
            liste = []
        if ligne.strip():
            morceaux.append("<p>" + _en_ligne(ligne.strip()) + "</p>")
    if liste:
        morceaux.append("<ul>" + "".join(liste) + "</ul>")
    return "".join(morceaux)


def _description(element) -> str:
    if element is None:
        return ""
    noeud = element.find("description")
    return bbcode_vers_html(noeud.text if noeud is not None else element.text)


def _type(element) -> str:
    """Type d'un membre / retour / paramètre, en préférant l'énumération quand elle existe."""
    return element.get("enum") or element.get("type") or "void"


def lire_classe(chemin: Path) -> dict:
    racine = ET.parse(chemin).getroot()
    classe = {
        "name": racine.get("name"),
        "inherits": racine.get("inherits", ""),
        "brief": bbcode_vers_html((racine.findtext("brief_description") or "")),
        "description": bbcode_vers_html(racine.findtext("description") or ""),
        "members": [],
        "methods": [],
        "signals": [],
        "constants": [],
    }
    for membre in racine.iterfind("members/member"):
        if membre.get("overrides"):
            continue  # simple changement de valeur par défaut d'une propriété du moteur
        classe["members"].append({
            "name": membre.get("name"),
            "type": _type(membre),
            "default": membre.get("default"),
            "setter": membre.get("setter", ""),
            "getter": membre.get("getter", ""),
            "description": bbcode_vers_html(membre.text),
        })
    for methode in racine.iterfind("methods/method"):
        retour = methode.find("return")
        classe["methods"].append({
            "name": methode.get("name"),
            "return": _type(retour) if retour is not None else "void",
            "qualifiers": methode.get("qualifiers", ""),
            "params": [{"name": p.get("name"), "type": _type(p), "default": p.get("default")} for p in methode.iterfind("param")],
            "description": _description(methode),
        })
    for signal in racine.iterfind("signals/signal"):
        classe["signals"].append({
            "name": signal.get("name"),
            "params": [{"name": p.get("name"), "type": _type(p)} for p in signal.iterfind("param")],
            "description": _description(signal),
        })
    for constante in racine.iterfind("constants/constant"):
        classe["constants"].append({
            "name": constante.get("name"),
            "value": constante.get("value"),
            "enum": constante.get("enum", ""),
            "description": bbcode_vers_html(constante.text),
        })
    return classe


def couverture(classe: dict) -> tuple[int, int]:
    """(éléments documentés, éléments au total) : présentation de la classe + chaque membre."""
    elements = [classe["brief"], classe["description"]]
    for cle in ("members", "methods", "signals", "constants"):
        elements += [e["description"] for e in classe[cle]]
    return sum(1 for e in elements if e), len(elements)


def main() -> None:
    classes = {}
    for chemin in sorted(DOSSIER_XML.glob("*.xml")):
        try:
            classe = lire_classe(chemin)
        except ET.ParseError as erreur:
            print(f"⚠️  {chemin.name} illisible : {erreur}")
            continue
        classe["coverage"] = couverture(classe)
        classes[classe["name"]] = classe

    SORTIE.parent.mkdir(parents=True, exist_ok=True)
    donnees = json.dumps({"classes": classes}, ensure_ascii=False, separators=(",", ":"))
    SORTIE.write_text(
        "// Généré par Documentation/generer_api.py depuis doc_classes/*.xml : NE PAS MODIFIER À LA MAIN.\n"
        f"window.GTK_API = {donnees};\n",
        encoding="utf-8",
    )
    faites = sum(1 for c in classes.values() if c["coverage"][0] == c["coverage"][1])
    print(f"✅ {len(classes)} classes exportées vers {SORTIE.relative_to(RACINE)} ({faites} entièrement documentées).")


if __name__ == "__main__":
    main()
