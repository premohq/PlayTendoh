// The setup guide's firmware picker (Get started, site/guide/get-started.md): a firmware and whether
// the PS5 has a disc drive in, that console's way in, HEN and steps out, from guide/firmware.json as
// site/guide.py puts it in the page (#firmware-data). Without this script the page's table says the same.
(function () {
  'use strict';

  var $ = function (id) { return document.getElementById(id); };

  // "11.60" as a number to compare (1160); NaN for anything not written as the console shows it
  function number(text) {
    var m = /^\s*(\d{1,2})\.(\d{2})\s*$/.exec(text || '');
    return m ? Number(m[1]) * 100 + Number(m[2]) : NaN;
  }

  function within(fw, item) {
    return fw >= number(item.from || '1.00') && fw <= number(item.to);
  }

  function el(tag, className, text) {
    var node = document.createElement(tag);
    if (className) node.className = className;
    if (text) node.textContent = text;
    return node;
  }

  // text with `code` in it, as the guide writes it
  function rich(node, text) {
    String(text).split(/(`[^`]+`)/).forEach(function (part) {
      if (/^`[^`]+`$/.test(part)) node.append(el('code', '', part.slice(1, -1)));
      else if (part) node.append(part);
    });
    return node;
  }

  function link(href, text) {
    var a = el('a', '', text);
    a.href = href;
    return a;
  }

  function capital(text) { return text.charAt(0).toUpperCase() + text.slice(1); }

  function range(item) { return !item.from || item.from === '1.00' ? 'up to ' + item.to : item.from + ' to ' + item.to; }

  // the routes that fit: the browser first, then a disc (with a drive), then the YouTube app, which can erase the console
  function routes(data, fw, disc) {
    var order = data.ways.map(function (w) { return w.page; });
    var fits = data.exploits.filter(function (e) { return within(fw, e); });
    fits.sort(function (a, b) { return order.indexOf(a.way) - order.indexOf(b.way); });
    var usable = fits.filter(function (e) { return disc || !way(data, e.way).disc; });
    var needsDrive = fits.filter(function (e) { return !disc && way(data, e.way).disc; });
    return { usable: usable, needsDrive: needsDrive };
  }

  function way(data, page) {
    return data.ways.filter(function (w) { return w.page === page; })[0];
  }

  function step(list, title, body, href, more) {
    var li = el('li');
    li.append(el('strong', '', title));
    if (body) { li.append(' '); rich(li, body); }
    if (href) { li.append(' '); li.append(link(href, more)); }
    list.append(li);
    return li;
  }

  function render(data, text, disc) {
    var out = $('pick-result');
    var next = document.querySelector('[data-continue]');
    out.textContent = '';
    out.className = 'picker-result';
    if (!text.trim()) { out.hidden = true; return; }
    out.hidden = false;
    var fw = number(text);
    if (isNaN(fw) || fw < 100) {
      out.classList.add('ask');
      out.append(el('p', 'picker-verdict', 'Write your firmware as the console shows it: a number, a dot and two digits, such as 11.60.'));
      return;
    }
    var shown = Math.floor(fw / 100) + '.' + ('0' + fw % 100).slice(-2);
    var found = routes(data, fw, disc);
    var hen = data.hens.filter(function (h) { return fw <= number(h.to); })[0];
    if (!found.usable.length || !hen) {
      out.classList.add('none');
      if (found.needsDrive.length) {
        var e = found.needsDrive[0];
        out.append(el('p', 'picker-verdict', shown + ' has one way in, and it needs a disc drive.'));
        out.append(rich(el('p'), capital(way(data, e.way).name) + ', with ' + e.name + ' (' + range(e) + '). A Slim or a Pro with its drive attached counts. Without a drive there is nothing yet.'));
        return;
      }
      out.append(el('p', 'picker-verdict', fw >= 1400 ?
        'Nothing public runs homebrew on ' + shown + ' yet.' :
        'This guide knows of no way in for ' + shown + '. The newest firmware with one is ' + data.newest + '.'));
      out.append(el('p', '', 'Playtendoh runs on a jailbroken PS5, so it can\'t run on this one for now. Keep its updates off: a way in, when one comes, covers the firmwares it was found on.'));
      return;
    }

    var first = found.usable[0];
    var road = way(data, first.way);
    var target = first.way + '.html#' + first.id;
    out.append(el('p', 'picker-verdict', shown + ' can run Playtendoh. This is everything it needs, in order.'));
    // what to know before the first step: the YouTube app's erasing the console, a firmware's own
    var notes = data.notes.filter(function (n) { return within(fw, n); }).map(function (n) { return n.text; });
    if (road.caution) notes.unshift(road.caution);
    notes.forEach(function (n) { out.append(rich(el('p', 'picker-note'), n)); });
    var list = el('ol', 'picker-steps');
    var li = step(list, 'Your way in: ' + road.name + ', with ' + first.name + '.', road.needs + (first.detail ? ' ' + first.detail : ''),
      target, capital(road.name) + ', step by step');
    var others = found.usable.slice(1).map(function (e) {
      return way(data, e.way).name + ' with ' + e.name + ' (' + range(e) + ')';
    });
    if (found.needsDrive.length) {
      others.push(way(data, found.needsDrive[0].way).name + ' with ' + found.needsDrive[0].name + ', on a PS5 with a disc drive');
    }
    if (others.length) li.append(el('span', 'picker-also', 'Also works on ' + shown + ': ' + others.join('; ') + '.'));
    step(list, 'Your HEN: ' + hen.name + '.', hen.why, 'jailbreak.html#hen', 'Load a HEN');
    step(list, 'Let Playtendoh in.', hen.letIn, 'jailbreak.html#' + hen.id, hen.name + '\'s steps');
    step(list, 'Install Playtendoh.', 'Copy the `PPSA99360` folder to `/data/homebrew`. It updates itself from inside the app after that.',
      'install.html', 'Install Playtendoh');
    step(list, 'After every restart.', capital(road.again) + ', load ' + hen.name + ', then start Playtendoh. Rest mode keeps the jailbreak.');
    out.append(list);
    var go = link(target, 'Continue to ' + road.name + ' ');
    go.className = 'btn primary';
    var arrow = el('span', '', '→');
    arrow.setAttribute('aria-hidden', 'true');
    go.append(arrow);
    out.append(go);
    out.append(el('p', 'picker-checked', 'The ways in as of ' + data.checked + '. The scene moves fast: check the project\'s own page before you start.'));
    if (next) {
      next.href = target;
      next.firstChild.textContent = 'Continue to ' + road.name + ' ';
    }
  }

  function start() {
    var form = $('picker');
    var source = $('firmware-data');
    if (!form || !source) return;
    var data;
    try { data = JSON.parse(source.textContent); } catch (e) { return; }
    var input = $('pick-fw');
    var disc = function () { return form.elements.disc.value === 'yes'; };
    var update = function (remember) {
      render(data, input.value, disc());
      if (!remember || !window.history.replaceState) return;
      var query = number(input.value) ? '?fw=' + encodeURIComponent(input.value.trim()) + (disc() ? '&disc=yes' : '') : '';
      window.history.replaceState(null, '', window.location.pathname + query + '#pick');
    };
    // a link with ?fw=11.60&disc=yes opens on that console's steps
    var params = new URLSearchParams(window.location.search);
    if (params.get('fw')) input.value = params.get('fw').slice(0, 5);
    if (params.get('disc') === 'yes') form.elements.disc.value = 'yes';
    form.hidden = false;
    form.addEventListener('submit', function (e) { e.preventDefault(); update(true); });
    input.addEventListener('input', function () { update(false); });
    Array.prototype.forEach.call(form.elements.disc, function (r) { r.addEventListener('change', function () { update(true); }); });
    if (input.value) update(false);
  }

  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', start);
  else start();
})();
