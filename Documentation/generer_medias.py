#!/usr/bin/env python3
"""🎥 Génère les médias des pages de documentation depuis le projet GameplayToolkit.

Les démos sont déclarées dans GameplayToolkit/🎥Démos de documentation/catalogue.json, par page
(chemin relatif à Documentation/modules/, sans .html) :
  - « sequences » : script de plans filmés (une vidéo par plan, voir socle/sequences.gd) ;
  - « apercu »    : scène interactive de l'export web, lancée par demos/web/index.html?demo=<page>.

Commandes (depuis la racine du dépôt ou Documentation/) :
  python Documentation/generer_medias.py videos                       -> toutes les vidéos de toutes les pages
  python Documentation/generer_medias.py videos effets/secousse       -> les vidéos d'une page
  python Documentation/generer_medias.py videos effets/secousse --plan ex-3d   -> un seul plan
  python Documentation/generer_medias.py integrer [pages...]          -> figures, légendes et schémas dans les pages
  python Documentation/generer_medias.py web                          -> export web (Documentation/demos/web/)

Les vidéos : Documentation/medias/<page>/<plan>.mp4 (H.264, 960×540, 30 i/s, sans son sauf « son » : true
ou la liste des plans sonores dans le catalogue), plus apercu.jpg (affiche de l'aperçu web) si la page a un aperçu. Chaque plan ouvre une
fenêtre Godot le temps de sa capture (--write-movie) : ne pas toucher au clavier ni à la manette pendant ce temps,
la scène recevrait ces entrées. « videos » enchaîne sur « integrer » pour les pages filmées.

Intégration dans Documentation/modules/<page>.html (relancer met à jour, sans doublon) :
  - plan « mode-emploi » : vidéo après la liste d'étapes du Mode d'emploi ; « mode-emploi-<id> » : après
    celle de la sous-partie <h3 id="<id>"> ;
  - plan « ex-... »      : vidéo (ou schéma de même clé) à la fin de l'exemple de même id ;
  - plan ou schéma « figure-N » : remplace le N-ième emplacement « Image à insérer » (figure-todo) ;
  - aperçu web : remplace l'emplacement de la Présentation (sinon placé à la fin de la Présentation) ;
  - « classe-X », « X.membre », les schémas de classes / membres et les réutilisations (une ligne d'API
    montrée par un plan ou schéma existant) : window.GTK_DEMOS et <template> SVG, entre les commentaires
    « 🎥 medias:debut / fin » (cartes de classe et boutons 🎬 des tableaux d'API).

L'export web demande les modèles d'export web 4.7.2 et les bibliothèques compilées pour le web :
  source ~/emsdk/emsdk_env.sh && scons platform=web threads=no
La page doit être servie en HTTP (pas en file://) :  python -m http.server -d Documentation
"""
import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile

RACINE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PROJET = os.path.join(RACINE, "GameplayToolkit")
DOCUMENTATION = os.path.join(RACINE, "Documentation")
MEDIAS = os.path.join(DOCUMENTATION, "medias")
WEB = os.path.join(DOCUMENTATION, "demos", "web")
CATALOGUE = os.path.join(PROJET, "🎥Démos de documentation", "catalogue.json")
CAPTURE = "res://🎥Démos de documentation/socle/capture.tscn"
LANCEUR_WEB = "res://🎥Démos de documentation/socle/lanceur_web.tscn"
PRESET_WEB = "Web documentation"

# Même exécutable que le Sconstruct : variable d'environnement GODOT, sinon chemin par défaut.
GODOT = os.environ.get("GODOT", "/home/artpade/.var/app/io.github.MakovWait.Godots/data/godot/app_userdata/Godots/"
                       "versions/Godot_v4_7_2-stable_linux_x86_64/Godot_v4.7.2-stable_linux.x86_64")
IPS = 30
DUREE_MAX = 90  # secondes : garde-fou si un plan ne se termine jamais


def charger_catalogue():
    with open(CATALOGUE, encoding="utf-8") as f:
        return {page: infos for page, infos in json.load(f).items() if not page.startswith("_")}


def lister_plans(page):
    sortie = subprocess.run(
        [GODOT, "--headless", "--path", PROJET, "--quit-after", "600", CAPTURE, "--", "--page=" + page, "--lister"],
        capture_output=True, text=True, errors="replace").stdout
    trouve = re.search(r"^PLANS: (.*)$", sortie, re.M)
    if not trouve:
        sys.exit("Impossible de lister les plans de {} :\n{}".format(page, sortie[-2000:]))
    return json.loads(trouve.group(1))


# 🧩 Intégration dans les pages

DEBUT, FIN = "<!-- 🎥 medias:debut (généré par generer_medias.py integrer : ne pas modifier à la main) -->", "<!-- 🎥 medias:fin -->"
TODO = re.compile(r'<div class="figure-todo">(?:\s*<div>.*?</div>)*\s*</div>', re.S)
FIGURE = re.compile(r'<figure class="media[^"]*"[^>]*>.*?</figure>', re.S)


def html_figure(video=None, web=None, schema=None, legende="", numero=None):
    attributs = ' data-figure="{}"'.format(numero) if numero else ""
    if web:
        return '<figure class="media media-web" data-web="{}"{}>\n    <figcaption>{}</figcaption>\n  </figure>'.format(web, attributs, legende)
    if schema:
        return '<figure class="media schema" data-schema="{}"{}><div class="schema-svg">{}</div><figcaption>{}</figcaption></figure>'.format(
            schema[0], attributs, schema[1], legende)
    return '<figure class="media" data-video="{}"{}><figcaption>{}</figcaption></figure>'.format(video, attributs, legende)


def lire_svg(chemin):
    with open(chemin, encoding="utf-8") as f:
        svg = f.read()
    return re.sub(r"<\?xml[^>]*>\s*", "", svg).strip()


def section(html, debut, fin_motif=r'\n\s*(?:<h2|<h3|<!-- ═)'):
    """(début, fin) du contenu qui suit la position `debut` jusqu'au prochain titre ou séparateur."""
    suite = re.compile(fin_motif).search(html, debut)
    return debut, (suite.start() if suite else len(html))


def placer(html, figure, cle_regex, position):
    """Remplace la figure existante qui correspond à cle_regex, sinon insère `figure` à `position`."""
    for existante in FIGURE.finditer(html):
        if re.search(cle_regex, existante.group(0).split(">", 1)[0]):
            return html[:existante.start()] + figure + html[existante.end():]
    return html[:position] + "\n  " + figure + html[position:]


def integrer(page, infos, liste):
    chemin = os.path.join(DOCUMENTATION, "modules", page + ".html")
    with open(chemin, encoding="utf-8") as f:
        html = f.read()
    plans, legendes, schemas = set(liste["plans"]), liste["legendes"], liste["schemas"]
    legende = lambda nom: legendes.get(nom, "") or (schemas.get(nom, [None, ""])[1])

    # Emplacements « Image à insérer » : numérotés dans l'ordre de la page, ceux déjà remplacés compris
    emplacements = sorted([(m.start(), m.end(), None) for m in TODO.finditer(html)]
                          + [(m.start(), m.end(), int(re.search(r'data-figure="(\d+)"', m.group(0)).group(1)))
                             for m in FIGURE.finditer(html) if 'data-figure="' in m.group(0).split(">", 1)[0]])
    presentation = re.search(r'<h2 id="presentation"', html)
    fin_presentation = section(html, presentation.end(), r'\n\s*(?:<h2|<!-- ═)')[1] if presentation else None
    remplacements = []
    for numero, (debut, fin, _) in enumerate(emplacements, 1):
        cle = "figure-{}".format(numero)
        if cle in plans:
            remplacements.append((debut, fin, html_figure(video=cle, legende=legende(cle), numero=numero)))
        elif cle in schemas:
            remplacements.append((debut, fin, html_figure(schema=(cle, lire_svg(schemas[cle][0])), legende=schemas[cle][1], numero=numero)))
        elif "apercu" in infos and presentation and presentation.end() < debut < fin_presentation \
                and not any("data-web" in r[2] for r in remplacements):
            remplacements.append((debut, fin, html_figure(web=page, legende=infos.get("legende_apercu", ""), numero=numero)))
    for debut, fin, figure in sorted(remplacements, reverse=True):
        html = html[:debut] + figure + html[fin:]

    if "apercu" in infos and "data-web=" not in html and presentation:
        fin_presentation = section(html, re.search(r'<h2 id="presentation"', html).end(), r'\n\s*(?:<h2|<!-- ═)')[1]
        html = html[:fin_presentation] + "\n  " + html_figure(web=page, legende=infos.get("legende_apercu", "")) + html[fin_presentation:]
    elif "apercu" in infos:
        html = re.sub(r'(<figure class="media media-web" data-web="[^"]*"[^>]*>\s*<figcaption>).*?(</figcaption>)',
                      lambda m: m.group(1) + infos.get("legende_apercu", "") + m.group(2), html, count=1, flags=re.S)

    # Mode d'emploi : après la liste d'étapes
    if "mode-emploi" in plans:
        titre = re.search(r'<h2 id="mode-emploi"', html)
        if titre:
            debut, fin = section(html, titre.end(), r'\n\s*(?:<h2|<!-- ═)')
            etapes = html.rfind("</ol>", debut, fin)
            position = etapes + len("</ol>") if etapes >= 0 else fin
            html = placer(html, html_figure(video="mode-emploi", legende=legende("mode-emploi")), r'data-video="mode-emploi"', position)

    # Sous-parties du Mode d'emploi (« mode-emploi-<id du h3> ») : après leur liste d'étapes
    for plan in sorted(p for p in plans if p.startswith("mode-emploi-")):
        titre = re.search(r'<h3 id="{}"'.format(re.escape(plan[len("mode-emploi-"):])), html)
        if not titre:
            print("  ⚠ sous-partie absente du Mode d'emploi : " + plan)
            continue
        debut, fin = section(html, titre.end())
        etapes = html.rfind("</ol>", debut, fin)
        position = etapes + len("</ol>") if etapes >= 0 else fin
        html = placer(html, html_figure(video=plan, legende=legende(plan)), r'data-video="{}"'.format(re.escape(plan)), position)

    # Exemples : vidéo, ou schéma de même nom, à la fin de leur section
    for plan in sorted(p for p in plans | set(schemas) if p.startswith("ex-")):
        titre = re.search(r'<h3 id="{}"'.format(re.escape(plan)), html)
        if not titre:
            print("  ⚠ exemple absent de la page : " + plan)
            continue
        fin = section(html, titre.end())[1]
        if plan in plans:
            figure = html_figure(video=plan, legende=legende(plan))
        else:
            figure = html_figure(schema=(plan, lire_svg(schemas[plan][0])), legende=schemas[plan][1])
        html = placer(html, figure, r'data-(?:video|schema)="{}"'.format(re.escape(plan)), fin)

    # API : window.GTK_DEMOS et schémas
    classes = " ".join(re.findall(r'data-api="([^"]*)"', html)).split()
    demos, modeles = {}, []
    for cle in sorted(plans | set(schemas), key=lambda c: (c.split(".")[0], "." in c, c)):
        cible = cle[len("classe-"):] if cle.startswith("classe-") else cle
        if cle.startswith(("ex-", "figure-", "mode-emploi")) or cible.split(".")[0] not in classes:
            continue
        entrees = demos.setdefault(cible, [])
        if cle in plans:
            entrees.append({"video": cle, "legende": legende(cle) if cle in legendes else ""})
        if cle in schemas:
            identifiant = "schema-" + re.sub(r"[^A-Za-z0-9_-]", "-", cle)
            modeles.append('<template id="{}">\n{}\n</template>'.format(identifiant, lire_svg(schemas[cle][0])))
            entrees.append({"schema": identifiant, "legende": schemas[cle][1]})
    for cible, source in liste.get("reutilisations", {}).items():
        source, propre = (source[0], source[1]) if isinstance(source, list) else (source, None)
        if cible.split(".")[0] not in classes:
            continue
        entrees = demos.setdefault(cible, [])
        if source in plans:
            entrees.append({"video": source, "legende": propre if propre is not None else legende(source)})
        elif source in schemas:
            identifiant = "schema-" + re.sub(r"[^A-Za-z0-9_-]", "-", source)
            if not any(identifiant in m for m in modeles):
                modeles.append('<template id="{}">\n{}\n</template>'.format(identifiant, lire_svg(schemas[source][0])))
            entrees.append({"schema": identifiant, "legende": propre if propre is not None else schemas[source][1]})
        else:
            print("  ⚠ réutilisation de {} : « {} » n'est ni un plan ni un schéma".format(cible, source))
    for entrees in demos.values():
        entrees.sort(key=lambda e: "schema" in e)  # vidéos d'abord
    bloc = DEBUT + "\n" + "\n".join(modeles) + ("\n" if modeles else "") + "<script>\nwindow.GTK_DEMOS = " \
        + json.dumps(demos, ensure_ascii=False, indent=2).replace("</", "<\\/") + ";\n</script>\n" + FIN
    if DEBUT in html:
        html = html[:html.index(DEBUT)] + bloc + html[html.index(FIN) + len(FIN):]
    else:
        ancre = re.search(r'<script>\s*window\.GTK_USAGES|<script src="[^"]*modules\.js"', html)
        html = html[:ancre.start()] + bloc + "\n" + html[ancre.start():]

    with open(chemin, "w", encoding="utf-8", newline="\n") as f:
        f.write(html)
    print("  ✓ {} : {} démo(s) d'API, {} schéma(s)".format(os.path.relpath(chemin, DOCUMENTATION), len(demos), len(modeles)))


def commande_integrer(arguments):
    catalogue = charger_catalogue()
    for page in arguments.pages or [p for p, infos in catalogue.items() if "sequences" in infos]:
        integrer(page, catalogue[page], lister_plans(page))
    return 0


def filmer(page, plan, avec_son):
    destination = os.path.join(MEDIAS, page, plan + ".mp4")
    os.makedirs(os.path.dirname(destination), exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="capture_") as dossier:
        brut = os.path.join(dossier, "plan.avi")
        commande = [GODOT, "--path", PROJET, "--resolution", "960x540", "--write-movie", brut, "--fixed-fps", str(IPS),
                    "--quit-after", str(DUREE_MAX * IPS), CAPTURE, "--", "--page=" + page, "--plan=" + plan]
        resultat = subprocess.run(commande, capture_output=True, text=True, errors="replace")
        erreurs = [l for l in resultat.stdout.splitlines() + resultat.stderr.splitlines() if "ERROR" in l or "SCRIPT ERROR" in l]
        if resultat.returncode != 0 or not os.path.isfile(brut):
            print("  ✗ {} : échec (code {})\n    {}".format(plan, resultat.returncode, "\n    ".join(erreurs[-8:])))
            return False
        son = ["-c:a", "aac", "-b:a", "96k"] if avec_son else ["-an"]
        subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-i", brut, "-c:v", "libx264", "-preset", "slow", "-crf", "27",
                        "-pix_fmt", "yuv420p", "-movflags", "+faststart"] + son + [destination], check=True)
    print("  ✓ {} ({} Ko){}".format(os.path.relpath(destination, DOCUMENTATION), os.path.getsize(destination) // 1024,
                                    " — avertissements : " + " | ".join(erreurs[:3]) if erreurs else ""))
    return True


def photographier_apercu(page):
    """Affiche de l'aperçu web (medias/<page>/apercu.jpg) : image de l'aperçu filmé sur le bureau, 3 s après
    son ouverture et juste après son action principale (mode rapport du lanceur)."""
    destination = os.path.join(MEDIAS, page, "apercu.jpg")
    os.makedirs(os.path.dirname(destination), exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="apercu_") as dossier:
        subprocess.run([GODOT, "--path", PROJET, "--resolution", "1152x648", "--write-movie", os.path.join(dossier, "i.png"),
                        "--fixed-fps", str(IPS), "--quit-after", str(int(2.9 * IPS)), LANCEUR_WEB, "--", "--demo=" + page,
                        "--rapport=1"], capture_output=True)
        images = sorted(f for f in os.listdir(dossier) if f.endswith(".png"))
        if not images:
            print("  ✗ apercu.jpg : aucune image")
            return False
        subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-i", os.path.join(dossier, images[-1]), "-vf", "scale=960:-2",
                        "-q:v", "4", destination], check=True)
    print("  ✓ {} ({} Ko)".format(os.path.relpath(destination, DOCUMENTATION), os.path.getsize(destination) // 1024))
    return True


def commande_videos(arguments):
    catalogue = charger_catalogue()
    pages = arguments.pages or [p for p, infos in catalogue.items() if "sequences" in infos]
    echecs = 0
    for page in pages:
        if page not in catalogue:
            sys.exit("Page absente du catalogue : " + page)
        liste = lister_plans(page)
        plans = [arguments.plan] if arguments.plan else liste["plans"]
        print("🎥 {} : {} plan(s)".format(page, len(plans)))
        for plan in plans:
            son = catalogue[page].get("son", False)  # true : toutes les vidéos, ou liste de plans
            echecs += not filmer(page, plan, son is True or (isinstance(son, list) and plan in son))
        if "apercu" in catalogue[page] and not arguments.plan:
            echecs += not photographier_apercu(page)
        integrer(page, catalogue[page], liste)
    return 1 if echecs else 0


def preparer_preset_web():
    """Liste les scènes d'aperçu du catalogue dans le préréglage d'export (export_files).

    Les scripts qu'elles préchargent ne sont pas suivis par ce filtre d'export : ils viennent de
    include_filter (tout « 🎥Démos de documentation/ ») ; un aperçu ne doit donc rien précharger d'ailleurs.
    """
    chemin = os.path.join(PROJET, "export_presets.cfg")
    with open(chemin, encoding="utf-8") as f:
        texte = f.read()
    scenes = ["res://🎥Démos de documentation/socle/lanceur_web.tscn"]
    scenes += [infos["apercu"] for infos in charger_catalogue().values() if "apercu" in infos]
    ligne = "export_files=PackedStringArray({})".format(", ".join('"{}"'.format(s) for s in scenes))
    nouveau = re.sub(r"^export_files=.*$", lambda _: ligne, texte, count=1, flags=re.M)
    if nouveau != texte:
        with open(chemin, "w", encoding="utf-8", newline="\n") as f:
            f.write(nouveau)


def commande_web(arguments):
    os.makedirs(WEB, exist_ok=True)
    preparer_preset_web()
    cible = os.path.join(WEB, "index.html")
    # Met à jour le cache du système de fichiers de l'éditeur : l'export y lit les dépendances des scènes
    # (preload des scripts) ; un fichier qu'il ne connaît pas encore serait absent du .pck.
    subprocess.run([GODOT, "--headless", "--path", PROJET, "--import"], capture_output=True)
    commande = [GODOT, "--headless", "--path", PROJET, "--export-debug" if arguments.debug else "--export-release", PRESET_WEB, cible]
    print("🌐 Export web :", os.path.relpath(cible, RACINE))
    resultat = subprocess.run(commande, capture_output=True, text=True, errors="replace")
    sortie = resultat.stdout + resultat.stderr
    for ligne in sortie.splitlines():
        if "ERROR" in ligne or "WARNING" in ligne:
            print("  " + ligne)
    if resultat.returncode != 0 or not os.path.isfile(cible):
        print(sortie[-3000:])
        return 1
    taille = sum(os.path.getsize(os.path.join(WEB, f)) for f in os.listdir(WEB))
    print("  ✓ {} fichiers, {:.1f} Mo".format(len(os.listdir(WEB)), taille / 1e6))
    return 0


def main():
    if not os.path.isfile(GODOT):
        sys.exit("Godot introuvable : {} (variable d'environnement GODOT)".format(GODOT))
    if not shutil.which("ffmpeg"):
        sys.exit("ffmpeg introuvable.")
    analyseur = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commandes = analyseur.add_subparsers(dest="commande", required=True)
    videos = commandes.add_parser("videos", help="filme les plans des pages")
    videos.add_argument("pages", nargs="*", help="pages du catalogue (toutes par défaut)")
    videos.add_argument("--plan", help="un seul plan")
    integration = commandes.add_parser("integrer", help="insère figures, légendes et schémas dans les pages")
    integration.add_argument("pages", nargs="*", help="pages du catalogue (toutes par défaut)")
    web = commandes.add_parser("web", help="exporte les aperçus interactifs")
    web.add_argument("--debug", action="store_true", help="export debug (sinon release)")
    arguments = analyseur.parse_args()
    return {"videos": commande_videos, "integrer": commande_integrer, "web": commande_web}[arguments.commande](arguments)


if __name__ == "__main__":
    sys.exit(main())
