// 🧭 Comportement commun des pages : navigation, sommaire, tableaux d'API générés depuis api-data.js,
// liens croisés, coloration GDScript, thème clair / sombre.
//
// Une page déclare dans <body> :
//   data-root="../../"                        chemin vers Documentation/
//   data-page="modules/effets/x.html"         sa propre entrée dans modules.js (navigation)
// et place ses tableaux d'API avec <div data-api="ClasseA ClasseB"></div>.
// La colonne « 🎓 Dans quel cas je peux l'utiliser » vient de window.GTK_USAGES = { "Classe.membre": "..." }.
(function () {
  "use strict";

  const API = (window.GTK_API && window.GTK_API.classes) || {};
  const MODULES = window.GTK_MODULES || [];
  const USAGES = window.GTK_USAGES || {};
  const body = document.body;
  const ROOT = body.dataset.root || "";
  const PAGE = body.dataset.page || "";
  const GODOT_DOC = "https://docs.godotengine.org/fr/4.x/classes/";

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
  const GODOT_ANCHOR = { member: "property", method: "method", signal: "signal", constant: "constant", enum: "enum" };
  function memberHref(kind, ref, context) {
    let cls = context, name = ref;
    const dot = ref.lastIndexOf(".");
    if (dot > 0) { cls = ref.slice(0, dot); name = ref.slice(dot + 1); }
    if (!cls) return null;
    if (API[cls]) {
      const base = classHref(cls);
      if (base === null) return null;
      return base.split("#")[0] + "#" + kind + "-" + cls + "-" + name;
    }
    const base = classHref(cls);
    return base ? base + "#class-" + cls.toLowerCase() + "-" + (GODOT_ANCHOR[kind] || kind) + "-" + name.replace(/_/g, "-") : null;
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
        + '<td class="name"><code>' + esc(m.name) + "</code></td>"
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
        + '<td class="name"><code>' + esc(m.name) + "(" + params + ")</code>" + quals + "</td>"
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
        + '<td class="name"><code>' + esc(s.name) + "</code></td>"
        + "<td>" + params + "</td>"
        + "<td>" + descOrMissing(s.description) + "</td>"
        + "<td>" + usage(owner, s.name) + "</td>"
        + "<td><em>" + owner + "</em></td>"
        + '<td><span class="access">public</span></td></tr>';
    }).join("");
  }

  function constantRows(cls, owner) {
    return API[owner].constants.map((c) => '<tr id="constant-' + cls + "-" + c.name + '" data-class="' + owner + '">'
      + '<td class="name"><code>' + esc(c.name) + "</code></td>"
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

    const filter = tools.querySelector("input[type=search]");
    const toggle = tools.querySelector(".show-inherited");
    function refresh() {
      const q = filter.value.trim().toLowerCase();
      const showInh = toggle ? toggle.checked : false;
      container.querySelectorAll("tbody tr").forEach((tr) => {
        const okInh = showInh || !tr.classList.contains("inherited");
        tr.hidden = !(okInh && (!q || tr.textContent.toLowerCase().includes(q)));
      });
    }
    filter.addEventListener("input", refresh);
    if (toggle) toggle.addEventListener("change", refresh);
    refresh();
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
