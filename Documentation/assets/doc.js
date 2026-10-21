// 🧭 Comportement commun des pages : navigation, sommaire, tableaux d'API générés depuis api-data.js,
// liens croisés, coloration GDScript, thème clair / sombre.
//
// Une page déclare dans <body> :
//   data-root="../../"                        chemin vers Documentation/
//   data-page="modules/effets/x.html"         sa propre entrée dans modules.js (navigation)
// et place ses tableaux d'API avec <div data-api="ClasseA ClasseB"></div>.
// La colonne « 🎓 Dans quel cas je peux l'utiliser » vient de window.GTK_USAGES = { "Classe.membre": "..." }.
//
// 🎥 Démos (produites par Documentation/generer_medias.py, dans medias/<page sans modules/ ni .html>/) :
//   <figure class="media" data-video="mode-emploi"><figcaption>...</figcaption></figure>   vidéo <nom>.mp4
//   <figure class="media media-web" data-web="effets/secousse"><figcaption>...</figcaption></figure>
//                                     aperçu interactif (demos/web/index.html?demo=...), affiche apercu.jpg
//   window.GTK_DEMOS = { "Classe": ..., "Classe.membre": ... } : démos des cartes de classe et des lignes d'API.
//     Valeur : "légende" (vidéo classe-<Classe>.mp4 ou <Classe.membre>.mp4), { video: "nom", legende },
//     { schema: "id d'un <template> de la page", legende } (membres sans effet visible), ou un tableau de ces valeurs.
(function () {
  "use strict";

  const API = (window.GTK_API && window.GTK_API.classes) || {};
  const MODULES = window.GTK_MODULES || [];
  const USAGES = window.GTK_USAGES || {};
  const DEMOS = window.GTK_DEMOS || {};
  const body = document.body;
  const ROOT = body.dataset.root || "";
  const PAGE = body.dataset.page || "";
  const GODOT_DOC = "https://docs.godotengine.org/fr/4.x/classes/";
  const MEDIA_BASE = ROOT + "medias/" + PAGE.replace(/^modules\//, "").replace(/\.html$/, "") + "/";

  // 🎨 Thème (préférence locale facultative : le site fonctionne sans stockage)
  function storedTheme() {
    try { return localStorage.getItem("gtk-doc-theme"); } catch (e) { return null; }
  }
  function applyTheme(theme) {
    if (theme) document.documentElement.dataset.theme = theme;
    else delete document.documentElement.dataset.theme;
  }
  applyTheme(storedTheme());
  function toggleTheme() {
    const dark = document.documentElement.dataset.theme
      ? document.documentElement.dataset.theme === "dark"
      : window.matchMedia("(prefers-color-scheme: dark)").matches;
    const next = dark ? "light" : "dark";
    applyTheme(next);
    try { localStorage.setItem("gtk-doc-theme", next); } catch (e) { /* stockage indisponible */ }
  }

  // 🗂️ Modules et classes
  const classToModule = {};
  MODULES.forEach((cat) => cat.modules.forEach((mod) => {
    mod.category = cat;
    mod.classes.forEach((c) => { classToModule[c] = mod; });
  }));

  function coverage(mod) {
    let done = 0, total = 0;
    mod.classes.forEach((c) => {
      const cls = API[c];
      if (cls) { done += cls.coverage[0]; total += cls.coverage[1]; }
    });
    return total ? done / total : 0;
  }

  function el(tag, attrs, html) {
    const node = document.createElement(tag);
    if (attrs) Object.entries(attrs).forEach(([k, v]) => { if (v !== undefined && v !== null) node.setAttribute(k, v); });
    if (html !== undefined) node.innerHTML = html;
    return node;
  }
  const esc = (s) => String(s).replace(/[&<>"]/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }[c]));

  // 🔗 Résolution des liens : classe du toolkit -> page du module, classe du moteur -> doc Godot
  function classHref(name) {
    if (API[name]) {
      const mod = classToModule[name];
      if (!mod || !mod.page) return null;
      return (mod.page === PAGE ? "" : ROOT + mod.page) + "#class-" + name;
    }
    if (/^[A-Z]/.test(name)) return GODOT_DOC + "class_" + name.toLowerCase() + ".html";
    return null;
  }
  const GODOT_ANCHOR = { member: "property", method: "method", signal: "signal", constant: "constant" };
  const GLOBAL = "@GlobalScope";
  // L'élément existe-t-il dans la classe du toolkit (ou ses parents du toolkit) ?
  function apiHas(cls, kind, name) {
    for (let c = cls; c && API[c]; c = API[c].inherits) {
      const k = API[c];
      const found = kind === "enum" ? k.constants.some((x) => x.enum === name)
        : (k[{ member: "members", method: "methods", signal: "signals", constant: "constants" }[kind]] || []).some((x) => x.name === name);
      if (found) return c;
    }
    return null;
  }
  function godotMemberHref(cls, kind, name) {
    const page = GODOT_DOC + "class_" + cls.toLowerCase() + ".html";
    const id = cls.replace("@", "").toLowerCase();
    return page + (kind === "enum" ? "#enum-" + id + "-" + name.toLowerCase()
      : "#class-" + id + "-" + (GODOT_ANCHOR[kind] || kind) + "-" + name.toLowerCase().replace(/_/g, "-"));
  }
  function memberHref(kind, ref, context) {
    let cls = context, name = ref;
    const dot = ref.lastIndexOf(".");
    if (dot > 0) { cls = ref.slice(0, dot); name = ref.slice(dot + 1); }
    if (cls === GLOBAL) return godotMemberHref(GLOBAL, kind, name);
    if (!cls) return null;
    if (API[cls]) {
      const owner = apiHas(cls, kind, name);
      // Inconnu dans la classe sans préfixe explicite : constante ou enum global de Godot (OK, Key...)
      if (!owner) return dot > 0 || (kind !== "constant" && kind !== "enum") ? null : godotMemberHref(GLOBAL, kind, name);
      const base = classHref(owner);
      if (base === null) return null;
      // Les lignes des tableaux portent l'id de la classe affichée (owner si elle est sur la page)
      return base.split("#")[0] + "#" + kind + "-" + owner + "-" + name;
    }
    const base = classHref(cls);
    return base ? godotMemberHref(cls, kind, name) : null;
  }
  function resolveRefs(root, context) {
    root.querySelectorAll("a.xref:not([href])").forEach((a) => {
      const ctx = a.closest("[data-class]");
      const href = a.dataset.kind === "class"
        ? classHref(a.dataset.ref)
        : memberHref(a.dataset.kind, a.dataset.ref, (ctx && ctx.dataset.class) || context);
      if (href) {
        a.href = href;
        if (href.startsWith("http")) { a.target = "_blank"; a.rel = "noopener"; }
      } else {
        a.classList.add("unresolved");
      }
    });
  }

  function typeHtml(type) {
    // « Effect[] » -> lien sur Effect ; « Tween.EaseType » -> lien sur Tween
    const base = type.replace(/\[\]$/, "").split(".")[0];
    const href = /^[A-Z]/.test(base) ? classHref(base) : null;
    const code = "<code>" + esc(type) + "</code>";
    return href ? '<a href="' + href + '"' + (href.startsWith("http") ? ' target="_blank" rel="noopener"' : "") + ">" + code + "</a>" : code;
  }
  const descOrMissing = (d) => d || '<span class="missing">Non documenté.</span>';
  const demoBtn = (owner, name) => DEMOS[owner + "." + name]
    ? ' <button type="button" class="api-demo-btn" data-demo="' + owner + "." + name + '" aria-expanded="false" title="Voir la démo">🎬</button>' : "";
  const usage = (cls, name) => USAGES[cls + "." + name] || '<span class="missing">—</span>';

  // 🧾 Tableaux d'API
  function ancestors(name) {
    const out = [];
    let cur = API[name] && API[name].inherits;
    while (cur && API[cur]) { out.push(cur); cur = API[cur].inherits; }
    return out;
  }

  function memberRows(cls, owner, inherited) {
    return API[owner].members.map((m) => {
      const access = m.setter ? "public" : "public · lecture seule";
      return '<tr id="member-' + cls + "-" + m.name + '" data-class="' + owner + '"' + (inherited ? ' class="inherited"' : "") + ">"
        + '<td class="name"><code>' + esc(m.name) + "</code>" + demoBtn(owner, m.name) + "</td>"
        + '<td class="type">' + typeHtml(m.type) + (m.default ? '<span class="default">= ' + esc(m.default) + "</span>" : "") + "</td>"
        + "<td>" + descOrMissing(m.description) + "</td>"
        + "<td>" + usage(owner, m.name) + "</td>"
        + "<td><em>" + owner + "</em></td>"
        + '<td><span class="access">' + access + "</span></td></tr>";
    }).join("");
  }

  function methodRows(cls, owner, inherited) {
    return API[owner].methods.map((m) => {
      const params = m.params.map((p) => esc(p.name) + ": " + esc(p.type) + (p.default ? " = " + esc(p.default) : "")).join(", ");
      const virtual = m.qualifiers.includes("virtual");
      const access = virtual ? '<span class="access protected">protected · virtuelle</span>' : '<span class="access">public</span>';
      const quals = m.qualifiers && !virtual ? ' <span class="default">' + esc(m.qualifiers) + "</span>" : "";
      return '<tr id="method-' + cls + "-" + m.name + '" data-class="' + owner + '"' + (inherited ? ' class="inherited"' : "") + ">"
        + '<td class="name"><code>' + esc(m.name) + "(" + params + ")</code>" + quals + demoBtn(owner, m.name) + "</td>"
        + '<td class="type">' + typeHtml(m.return) + "</td>"
        + "<td>" + descOrMissing(m.description) + "</td>"
        + "<td>" + usage(owner, m.name) + "</td>"
        + "<td><em>" + owner + "</em></td>"
        + "<td>" + access + "</td></tr>";
    }).join("");
  }

  function signalRows(cls, owner, inherited) {
    return API[owner].signals.map((s) => {
      const params = s.params.length ? s.params.map((p) => "<code>" + esc(p.name) + ": " + esc(p.type) + "</code>").join("<br>") : '<span class="missing">aucun</span>';
      return '<tr id="signal-' + cls + "-" + s.name + '" data-class="' + owner + '"' + (inherited ? ' class="inherited"' : "") + ">"
        + '<td class="name"><code>' + esc(s.name) + "</code>" + demoBtn(owner, s.name) + "</td>"
        + "<td>" + params + "</td>"
        + "<td>" + descOrMissing(s.description) + "</td>"
        + "<td>" + usage(owner, s.name) + "</td>"
        + "<td><em>" + owner + "</em></td>"
        + '<td><span class="access">public</span></td></tr>';
    }).join("");
  }

  function constantRows(cls, owner) {
    const seen = new Set();
    return API[owner].constants.map((c) => '<tr id="constant-' + cls + "-" + c.name + '" data-class="' + owner + '">'
      // Première valeur d'une énumération : cible des liens [enum Nom]
      + '<td class="name">' + (c.enum && !seen.has(c.enum) && seen.add(c.enum) ? '<span id="enum-' + cls + "-" + c.enum + '"></span>' : "")
      + "<code>" + esc(c.name) + "</code></td>"
      + "<td><code>" + esc(c.enum ? owner + "." + c.enum : "int") + "</code><span class=\"default\">= " + esc(c.value) + "</span></td>"
      + "<td>" + descOrMissing(c.description) + "</td>"
      + "<td>" + usage(owner, c.name) + "</td>"
      + "<td><em>" + owner + "</em></td>"
      + '<td><span class="access">public</span></td></tr>').join("");
  }

  const TABLES = [
    { key: "members", title: "⚛️ Détail des Propriétés", cols: ["Nom de la propriété", "Type", "📋 Description", "🎓 Dans quel cas je peux l'utiliser", "Classe possessive", "Accessibilité"], rows: memberRows },
    { key: "methods", title: "🔹 Méthodes principales", cols: ["Nom de la méthode", "Type retour", "📋 Description", "🎓 Dans quel cas je peux l'utiliser", "Classe possessive", "Accessibilité"], rows: methodRows },
    { key: "signals", title: "🔔 Détail des signaux", cols: ["Nom de l'évènement", "Paramètre", "📋 Description", "🎓 Dans quel cas je peux l'utiliser", "Classe possessive", "Accessibilité"], rows: signalRows },
    { key: "constants", title: "🔢 Énumérations et constantes", cols: ["Nom de la constante", "Énumération", "📋 Description", "🎓 Dans quel cas je peux l'utiliser", "Classe possessive", "Accessibilité"], rows: constantRows, noInherit: true },
  ];

  function renderApi(container) {
    const names = container.dataset.api.split(/\s+/).filter(Boolean);
    const missing = names.filter((n) => !API[n]);
    if (missing.length) {
      container.appendChild(el("div", { class: "callout danger" }, "Classes absentes de <code>api-data.js</code> : " + missing.join(", ")
        + ". Relancer <code>python Documentation/generer_api.py</code>."));
    }
    const present = names.filter((n) => API[n]);

    // Présentation de chaque classe
    present.forEach((name) => {
      const cls = API[name];
      // Parent direct, puis ses propres parents tant qu'ils appartiennent au toolkit
      const chain = cls.inherits ? [cls.inherits].concat(ancestors(cls.inherits)) : [];
      if (chain.length && API[chain[chain.length - 1]]) chain.push(API[chain[chain.length - 1]].inherits);
      const card = el("div", { class: "card class-card", id: "class-" + name, "data-class": name });
      card.innerHTML = "<h4><code>" + name + "</code></h4>"
        + '<div class="inherits">Hérite de : ' + (chain.filter(Boolean).map((c) => '<a class="xref" data-kind="class" data-ref="' + c + '"><code>' + c + "</code></a>").join(" ‹ ") || "—") + "</div>"
        + (cls.brief || "") + (cls.description ? "<details><summary>Description complète</summary>" + cls.description + "</details>" : "");
      if (DEMOS[name]) {
        const figures = mediaFigures(DEMOS[name], "classe-" + name);
        const holder = figures.length > 1 ? card.appendChild(el("div", { class: "media-grid" })) : card;
        figures.forEach((f) => holder.appendChild(f));
      }
      container.appendChild(card);
    });

    // Outils : filtre et membres hérités
    const inheritable = present.some((n) => ancestors(n).length);
    const tools = el("div", { class: "api-tools" });
    tools.innerHTML = '<input type="search" placeholder="🔎 Filtrer l\'API (nom, description...)">'
      + (inheritable ? '<label><input type="checkbox" class="show-inherited"> Afficher les membres hérités des classes du toolkit</label>' : "");
    container.appendChild(tools);

    TABLES.forEach((t) => {
      let own = "", inherited = "";
      present.forEach((name) => {
        own += t.rows(name, name, false);
        if (!t.noInherit) ancestors(name).forEach((a) => { inherited += t.rows(name, a, true); });
      });
      if (!own && !inherited) return;
      container.appendChild(el("h3", null, t.title));
      const wrap = el("div", { class: "table-wrap" });
      wrap.innerHTML = "<table><thead><tr>" + t.cols.map((c) => "<th>" + c + "</th>").join("") + "</tr></thead><tbody>"
        + own + inherited + "</tbody></table>";
      container.appendChild(wrap);
    });

    container.querySelectorAll(".api-demo-btn").forEach((btn) => btn.addEventListener("click", () => {
      const tr = btn.closest("tr");
      const open = btn.getAttribute("aria-expanded") === "true";
      btn.setAttribute("aria-expanded", open ? "false" : "true");
      let row = tr.nextElementSibling;
      if (!row || !row.classList.contains("media-row")) {
        row = el("tr", { class: "media-row" });
        const td = el("td", { colspan: tr.children.length });
        mediaFigures(DEMOS[btn.dataset.demo], btn.dataset.demo).forEach((f) => td.appendChild(f));
        row.appendChild(td);
        tr.after(row);
        watchVideos(row);
      }
      row.hidden = open;
    }));

    const filter = tools.querySelector("input[type=search]");
    const toggle = tools.querySelector(".show-inherited");
    function refresh() {
      const q = filter.value.trim().toLowerCase();
      const showInh = toggle ? toggle.checked : false;
      container.querySelectorAll("tbody tr").forEach((tr) => {
        const okInh = showInh || !tr.classList.contains("inherited");
        if (tr.classList.contains("media-row")) return;
        tr.hidden = !(okInh && (!q || tr.textContent.toLowerCase().includes(q)));
        const next = tr.nextElementSibling;
        if (next && next.classList.contains("media-row")) {
          next.hidden = tr.hidden || tr.querySelector(".api-demo-btn").getAttribute("aria-expanded") !== "true";
        }
      });
    }
    filter.addEventListener("input", refresh);
    if (toggle) toggle.addEventListener("change", refresh);
    refresh();
  }

  // 🎥 Médias de démonstration
  function videoFigure(name, legende) {
    const fig = el("figure", { class: "media", "data-video": name });
    if (legende) fig.appendChild(el("figcaption", null, legende));
    buildVideo(fig);
    return fig;
  }

  function schemaFigure(id, legende) {
    const fig = el("figure", { class: "media schema" });
    const box = fig.appendChild(el("div", { class: "schema-svg" }));
    const tpl = document.getElementById(id);
    if (tpl) box.appendChild(tpl.content.cloneNode(true));
    else box.innerHTML = '<span class="missing">Schéma introuvable : ' + esc(id) + "</span>";
    if (legende) fig.appendChild(el("figcaption", null, legende));
    return fig;
  }

  // Valeur de GTK_DEMOS -> figures ; `defaut` : nom de la vidéo quand la valeur n'est qu'une légende.
  function mediaFigures(value, defaut) {
    return [].concat(value).map((v) => {
      if (typeof v === "string") return videoFigure(defaut, v);
      if (v.schema) return schemaFigure(v.schema, v.legende);
      return videoFigure(v.video || defaut, v.legende);
    });
  }

  function buildVideo(fig) {
    if (fig.querySelector("video")) return;
    const video = el("video", { muted: "", loop: "", playsinline: "", preload: "none", title: "Cliquer : pause / lecture",
      "data-src": MEDIA_BASE + fig.dataset.video + ".mp4" });
    video.muted = true;
    video.addEventListener("click", () => { if (video.paused) video.play(); else video.pause(); video.dataset.user = "1"; });
    fig.insertBefore(video, fig.firstChild);
  }

  // Lecture seulement à l'écran : chargement au premier passage, pause hors de la vue.
  const videoSpy = "IntersectionObserver" in window ? new IntersectionObserver((entries) => {
    entries.forEach((e) => {
      const video = e.target;
      if (e.isIntersecting) {
        if (!video.src) video.src = video.dataset.src;
        if (!video.dataset.user) video.play().catch(() => {});
      } else if (!video.paused) {
        video.pause();
      }
    });
  }, { threshold: 0.35 }) : null;

  function watchVideos(root) {
    root.querySelectorAll("video[data-src]").forEach((video) => {
      if (videoSpy) videoSpy.observe(video);
      else { video.src = video.dataset.src; video.controls = true; }
    });
  }

  function buildWeb(fig) {
    const demo = fig.dataset.web;
    const url = ROOT + "demos/web/index.html?demo=" + encodeURIComponent(demo);
    const poster = el("img", { class: "media-poster", src: MEDIA_BASE + "apercu.jpg", alt: "Aperçu de la démo interactive" });
    const launch = el("div", { class: "media-launch" });
    launch.innerHTML = '<button type="button">▶ Lancer la démo interactive</button><small>Godot en WebAssembly · ≈ 75 Mo au premier lancement</small>';
    fig.insertBefore(launch, fig.firstChild);
    fig.insertBefore(poster, fig.firstChild);
    const caption = fig.querySelector("figcaption") || fig.appendChild(el("figcaption"));
    caption.insertAdjacentHTML("beforeend", '<a href="' + url + '" target="_blank" rel="noopener">Ouvrir en plein écran ↗</a>');
    launch.querySelector("button").addEventListener("click", () => {
      if (location.protocol === "file:") {
        if (!fig.querySelector(".media-file")) {
          fig.appendChild(el("div", { class: "media-file" }, "Les démos web ne fonctionnent pas en ouvrant le fichier directement (<code>file://</code>). "
            + "Servir la documentation en HTTP : <code>python -m http.server -d Documentation</code>, puis ouvrir "
            + "<code>http://localhost:8000/" + esc(PAGE) + "</code>."));
        }
        return;
      }
      const frame = el("iframe", { src: url, title: "Démo interactive", allow: "fullscreen; gamepad; autoplay", allowfullscreen: "" });
      poster.replaceWith(frame);
      launch.remove();
      frame.focus();
    });
  }

  function renderMedia(root) {
    root.querySelectorAll("figure.media[data-video]").forEach(buildVideo);
    root.querySelectorAll("figure.media-web[data-web]").forEach(buildWeb);
    watchVideos(root);
  }

  // 🔤 Coloration GDScript (légère, sans dépendance)
  const KW = new Set(("func var const if elif else for while match return await signal extends class_name static pass break "
    + "continue in and or not true false null self super enum is as").split(" "));
  const span = (cls, t) => '<span class="' + cls + '">' + esc(t) + "</span>";
  function highlight(code) {
    const src = code.textContent;
    // commentaire | chaîne | annotation | nombre | Type | appel( | mot
    const re = /(#[^\n]*)|("(?:[^"\\\n]|\\.)*"|'(?:[^'\\\n]|\\.)*')|(@\w+)|\b(\d+(?:\.\d+)?)\b|\b([A-Z][A-Za-z0-9_]*)\b|\b([a-z_]\w*)(?=\()|\b([a-z_]\w*)\b/g;
    let out = "", last = 0, m;
    while ((m = re.exec(src))) {
      out += esc(src.slice(last, m.index));
      const t = m[0];
      if (m[1]) out += span("tok-com", t);
      else if (m[2]) out += span("tok-str", t);
      else if (m[3]) out += span("tok-kw", t);
      else if (m[4]) out += span("tok-num", t);
      else if (m[5]) out += span("tok-type", t);
      else if (m[6]) out += span(KW.has(t) ? "tok-kw" : "tok-fn", t);
      else if (KW.has(t)) out += span("tok-kw", t);
      else out += esc(t);
      last = m.index + t.length;
    }
    code.innerHTML = out + esc(src.slice(last));
  }

  function enhanceCode(root) {
    root.querySelectorAll("pre > code").forEach((code) => {
      if (code.dataset.done) return;
      code.dataset.done = "1";
      if (!code.className || code.classList.contains("gdscript")) highlight(code);
      const btn = el("button", { class: "copy-btn", type: "button" }, "Copier");
      btn.addEventListener("click", () => {
        const done = () => { btn.textContent = "Copié ✓"; setTimeout(() => { btn.textContent = "Copier"; }, 1500); };
        if (navigator.clipboard) navigator.clipboard.writeText(code.textContent).then(done, () => { btn.textContent = "Échec"; });
      });
      code.parentElement.appendChild(btn);
    });
  }

  // 🧱 Ossature : barre du haut + navigation latérale + sommaire
  function buildChrome() {
    const main = document.querySelector("main");
    const top = el("header", { class: "topbar" });
    top.innerHTML = '<button class="icon-btn" id="menu-btn" type="button" aria-label="Menu">☰</button>'
      + '<a class="brand" href="' + ROOT + 'index.html">🧰 GameplayToolkit · Documentation</a><span class="spacer"></span>'
      + '<button class="icon-btn" id="theme-btn" type="button" aria-label="Changer de thème" title="Thème clair / sombre">🌓</button>';
    body.insertBefore(top, body.firstChild);

    const layout = el("div", { class: "layout" });
    const nav = el("nav", { class: "sidebar", "aria-label": "Modules" });
    let navHtml = '<input type="search" placeholder="🔎 Chercher un module ou une classe">';
    MODULES.forEach((cat) => {
      navHtml += '<div class="nav-cat">' + cat.emoji + " " + esc(cat.name) + "</div>";
      cat.modules.forEach((mod) => {
        const cov = coverage(mod);
        const dot = '<span class="dot ' + (cov >= 0.999 && mod.page ? "ok" : cov > 0 || mod.page ? "partial" : "") + '" title="Documentation : ' + Math.round(cov * 100) + ' %"></span>';
        const label = "<span>" + mod.emoji + "</span><span>" + esc(mod.name) + "</span>" + dot;
        const search = esc((mod.name + " " + mod.domain + " " + mod.classes.join(" ")).toLowerCase());
        navHtml += mod.page
          ? '<a class="nav-link' + (mod.page === PAGE ? " current" : "") + '" href="' + ROOT + mod.page + '" data-search="' + search + '">' + label + "</a>"
          : '<span class="nav-link disabled" title="Page à rédiger" data-search="' + search + '">' + label + "</span>";
      });
    });
    nav.innerHTML = navHtml;
    layout.appendChild(nav);
    body.insertBefore(layout, main);
    layout.appendChild(main);

    const tocTargets = main.querySelectorAll(".content h2[id], .content h3[id]");
    if (tocTargets.length && !body.dataset.noToc) {
      const toc = el("aside", { class: "toc" }, '<div class="toc-title">Sur cette page</div>');
      tocTargets.forEach((h) => toc.appendChild(el("a", { href: "#" + h.id, class: h.tagName === "H3" ? "sub" : "" }, h.textContent)));
      layout.appendChild(toc);
      const links = [...toc.querySelectorAll("a")];
      const spy = new IntersectionObserver((entries) => {
        entries.forEach((e) => {
          if (!e.isIntersecting) return;
          links.forEach((a) => a.classList.toggle("active", a.getAttribute("href") === "#" + e.target.id));
        });
      }, { rootMargin: "-60px 0px -70% 0px" });
      tocTargets.forEach((h) => spy.observe(h));
    }

    document.getElementById("theme-btn").addEventListener("click", toggleTheme);
    document.getElementById("menu-btn").addEventListener("click", () => body.classList.toggle("nav-open"));
    nav.querySelector("input").addEventListener("input", (e) => {
      const q = e.target.value.trim().toLowerCase();
      nav.querySelectorAll(".nav-link").forEach((a) => { a.hidden = q && !a.dataset.search.includes(q); });
    });
  }

  // 🏠 Accueil : tuiles des modules avec leur couverture
  function renderIndex(container) {
    MODULES.forEach((cat) => {
      container.appendChild(el("h2", { id: "cat-" + cat.id }, cat.emoji + " " + esc(cat.name) + " " + (cat.trailing || "")));
      const grid = el("div", { class: "grid" });
      cat.modules.forEach((mod) => {
        const cov = coverage(mod);
        const tile = el(mod.page ? "a" : "div", { class: "module-tile" + (mod.page ? "" : " todo"), href: mod.page ? ROOT + mod.page : null });
        tile.innerHTML = '<div class="t">' + mod.emoji + " " + esc(mod.name) + "</div>"
          + '<div class="d">' + esc(mod.domain) + " · " + mod.classes.length + " classe" + (mod.classes.length > 1 ? "s" : "") + " · "
          + (mod.page ? "page rédigée" : "page à rédiger") + "</div>"
          + '<div class="bar" title="Référence XML documentée à ' + Math.round(cov * 100) + ' %"><span style="width:' + Math.round(cov * 100) + '%"></span></div>';
        grid.appendChild(tile);
      });
      container.appendChild(grid);
    });
  }

  function init() {
    document.querySelectorAll("[data-api]").forEach(renderApi);
    document.querySelectorAll("[data-module-index]").forEach(renderIndex);
    renderMedia(document);
    buildChrome();
    resolveRefs(document, "");
    enhanceCode(document);
    if (location.hash) {
      const target = document.getElementById(decodeURIComponent(location.hash.slice(1)));
      if (target) {
        if (target.tagName === "TR") target.classList.add("flash");
        target.scrollIntoView();
      }
    }
  }

  if (document.readyState === "loading") document.addEventListener("DOMContentLoaded", init);
  else init();
})();
