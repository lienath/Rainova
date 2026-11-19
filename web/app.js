(function () {
  'use strict';
  const $ = (id) => document.getElementById(id);
  const ESTADOS = {
    REPOSO: 'OK', BOMBEANDO: 'REGANDO', ESPERANDO: 'ESPERA', ALERTA: 'PAUSA'
  };
  let ultimaCal = false;
  let estadoCache = null;

  function fmtHora(epoch) {
    if (!epoch) return '--:--';
    const d = new Date(epoch * 1000);
    return d.toLocaleTimeString('es-CO', { hour: '2-digit', minute: '2-digit', hour12: false });
  }
  function fmtFechaHora(epoch) {
    if (!epoch) return 'Sin registros hoy';
    const d = new Date(epoch * 1000);
    return d.toLocaleString('es-CO', {
      day: '2-digit', month: 'short', hour: '2-digit', minute: '2-digit', hour12: false
    });
  }
  function fmtUptime(s) {
    const h = Math.floor(s / 3600), m = Math.floor((s % 3600) / 60);
    return h > 0 ? h + ' h ' + m + ' min' : m + ' min';
  }
  function setMsg(id, text, ok) {
    const el = $(id);
    if (!el) return;
    el.textContent = text || '';
    el.classList.toggle('ok', ok === true);
    el.classList.toggle('err', ok === false);
  }

  async function api(url, options) {
    const r = await fetch(url, Object.assign({ cache: 'no-store' }, options || {}));
    if (!r.ok) throw new Error('HTTP ' + r.status);
    const ct = r.headers.get('content-type') || '';
    return ct.indexOf('json') >= 0 ? r.json() : r.text();
  }
  function post(url, obj) {
    return api(url, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(obj)
    });
  }
  async function postResult(url, obj) {
    try {
      const r = await fetch(url, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(obj)
      });
      let data = {};
      try { data = await r.json(); } catch (e) { /* respuesta vacía */ }
      return { ok: r.ok, status: r.status, data: data || {} };
    } catch (e) {
      return { ok: false, status: 0, data: {} };
    }
  }

  function syncHora() {
    return post('/api/hora', { epoch: Math.floor(Date.now() / 1000) }).catch(() => {});
  }

  function renderBanner(e) {
    const b = $('banner');
    let msg = '';
    if (!e.agua) msg = '🚫 Tanque vacío: la bomba está detenida.';
    else if (e.alerta) msg = '⚠️ ' + e.alerta;
    else if (e.pausado) msg = '⏸ Riego automático en pausa.';
    b.textContent = msg;
    b.classList.toggle('hidden', !msg);
  }

  function render(e) {
    estadoCache = e;
    $('wifiDot').classList.toggle('off', !e.wifi);
    $('clock').textContent = e.hora ? fmtHora(e.hora) : fmtHora(Date.now() / 1000);

    const h = Math.max(0, Math.min(100, e.humedad));
    $('humedad').textContent = h;
    const arc = $('gaugeArc');
    arc.style.strokeDashoffset = String(327 * (1 - h / 100));
    arc.style.stroke = h < 35 ? 'var(--amber)' : h >= 60 ? 'var(--accent2)' : 'var(--green)';

    let tend = 'Estable';
    if (typeof e.tendencia === 'number') {
      if (e.tendencia < -0.05) tend = '↓ Bajando ' + Math.abs(e.tendencia).toFixed(2) + '%/min';
      else if (e.tendencia > 0.05) tend = '↑ Subiendo ' + e.tendencia.toFixed(2) + '%/min';
    }
    $('tendencia').textContent = tend;

    $('aguaTexto').textContent = e.agua ? 'HAY AGUA' : 'SIN AGUA';
    $('cardAgua').classList.toggle('warn', !e.agua);
    $('estadoTexto').textContent = ESTADOS[e.estado] || e.estado;
    $('cardEstado').classList.toggle('warn', e.estado === 'ALERTA' && e.agua);

    const ya = e.riegosHoy > 0;
    const manual = e.manualHoy || 0;
    $('yaRego').textContent = ya ? 'SÍ' : 'NO';
    $('yaRego').classList.toggle('no', !ya);
    $('ultimoRiego').textContent = e.ultimoRiego ? fmtFechaHora(e.ultimoRiego) : 'Sin registros hoy';
    $('mlHoy').textContent = Math.round(e.mlHoy) + ' mL usados hoy';
    $('riegosHoy').textContent = e.riegosHoy + (e.riegosHoy === 1 ? ' ciclo' : ' ciclos') +
      (manual ? ' · ' + manual + ' manual' + (manual === 1 ? '' : 'es') : '');

    if (e.proximo >= 0 && e.proximo < 600) {
      $('proximo').textContent = '~ ' + Math.max(1, Math.round(e.proximo)) + ' min';
    } else if (!e.agua) {
      $('proximo').textContent = 'Sin agua';
    } else {
      $('proximo').textContent = 'Estable';
    }

    if (e.mlUltimo > 0 && e.mlManual > 0) {
      $('ahorroPct').textContent = Math.round(e.ahorro);
      $('barSistema').style.width = Math.max(2, Math.min(100, (e.mlUltimo / e.mlManual) * 100)) + '%';
      $('mlSistema').textContent = Math.round(e.mlUltimo) + ' mL';
      $('mlManual').textContent = Math.round(e.mlManual) + ' mL';
    } else {
      $('ahorroPct').textContent = '--';
      $('mlSistema').textContent = '-- mL';
      $('mlManual').textContent = Math.round(e.mlManual || 0) + ' mL';
    }

    $('statHoy').textContent = Math.round(e.mlHoy) + ' mL';
    $('statTotal').textContent = Math.round(e.mlTotal) + ' mL';
    $('statCiclos').textContent = String(e.riegosHoy) + (manual ? ' (' + manual + ' man.)' : '');
    $('uptime').textContent = fmtUptime(e.uptime);
    $('crudo').textContent = String(e.crudo);

    if (!ultimaCal) {
      $('inSeco').value = e.seco;
      $('inMojado').value = e.mojado;
      $('inMl').value = e.mlSeg;
      ultimaCal = true;
    }
    renderBanner(e);
  }

  function renderHistorial(data) {
    const pts = (data.samples || []).slice(-80);
    if (pts.length > 1) {
      const coords = pts.map((s, i) => {
        const x = (i / (pts.length - 1)) * 320;
        const y = 150 - (Math.max(0, Math.min(100, s.h)) / 100) * 150;
        return x.toFixed(1) + ',' + y.toFixed(1);
      });
      $('chartLine').setAttribute('points', coords.join(' '));
    }
    const ev = data.eventos || [];
    const box = $('eventos');
    if (!ev.length) {
      box.innerHTML = '<p class="muted">Sin eventos todavía.</p>';
      return;
    }
    box.innerHTML = ev.slice(-20).reverse().map((x) =>
      '<div class="evento"><span class="t">' + fmtHora(x.t) + '</span>' +
      '<span class="d">' + x.d + '</span></div>'
    ).join('');
  }

  async function poll() {
    try { render(await api('/api/estado')); }
    catch (err) { $('wifiDot').classList.add('off'); }
  }
  async function pollHist() {
    try { renderHistorial(await api('/api/historial')); } catch (err) { }
  }

  async function accion(name, btn) {
    if (!btn) return;
    btn.classList.add('busy');
    const res = await postResult('/api/accion', { accion: name });
    if (res.ok && name === 'regar') {
      const m = res.data.manual;
      if (m === 'iniciado') setMsg('msgControl', 'Riego manual iniciado y registrado.', true);
      else if (m === 'pendiente') setMsg('msgControl', 'Riego manual programado al terminar el ciclo actual.', true);
      else if (m === 'pausa') setMsg('msgControl', 'No se pudo regar: riego automático en pausa.', false);
      else if (m === 'no_agua') setMsg('msgControl', 'No se pudo regar: tanque sin agua.', false);
    } else if (!res.ok) {
      setMsg('msgControl', 'No se pudo enviar la orden.', false);
    }
    await poll();
    await pollHist();
    btn.classList.remove('busy');
  }

  function init() {
    document.querySelectorAll('.nav-btn').forEach((b) => {
      b.addEventListener('click', () => {
        document.querySelectorAll('.nav-btn').forEach((x) => x.classList.remove('active'));
        document.querySelectorAll('.tab').forEach((x) => x.classList.remove('active'));
        b.classList.add('active');
        $('tab-' + b.dataset.tab).classList.add('active');
        if (b.dataset.tab === 'historico') pollHist();
      });
    });

    $('btnRegar').addEventListener('click', (ev) => accion('regar', ev.currentTarget));
    $('btnPausa').addEventListener('click', (ev) => accion('pausa', ev.currentTarget));
    $('btnReanudar').addEventListener('click', (ev) => accion('reanudar', ev.currentTarget));
    $('btnReset').addEventListener('click', (ev) => accion('reset', ev.currentTarget));
    $('btnCsv').addEventListener('click', () => { window.location = '/exportar.csv'; });

    $('btnUsarSeco').addEventListener('click', () => {
      if (estadoCache) $('inSeco').value = estadoCache.crudo;
    });
    $('btnUsarMojado').addEventListener('click', () => {
      if (estadoCache) $('inMojado').value = estadoCache.crudo;
    });
    $('btnGuardarCal').addEventListener('click', async (ev) => {
      const b = ev.currentTarget;
      const pin = ($('inPin').value || '').replace(/\D/g, '');
      const nuevoPin = ($('inNuevoPin').value || '').replace(/\D/g, '');
      if (pin.length !== 4) {
        setMsg('msgCal', 'Ingresa el PIN de 4 dígitos.', false);
        return;
      }
      if (nuevoPin && nuevoPin.length !== 4) {
        setMsg('msgCal', 'El nuevo PIN debe tener 4 dígitos.', false);
        return;
      }
      const payload = {
        seco: parseInt($('inSeco').value, 10),
        mojado: parseInt($('inMojado').value, 10),
        ml: parseFloat($('inMl').value),
        pin: parseInt(pin, 10)
      };
      if (nuevoPin) payload.nuevoPin = parseInt(nuevoPin, 10);
      b.classList.add('busy');
      const res = await postResult('/api/calibracion', payload);
      if (!res.ok) {
        const txt = res.status === 403 ? 'PIN incorrecto.' :
                    res.status === 429 ? 'Demasiados intentos. Espera 30 s.' :
                    'No se pudo guardar la calibración.';
        setMsg('msgCal', txt, false);
      } else {
        ultimaCal = false;
        $('inPin').value = '';
        $('inNuevoPin').value = '';
        setMsg('msgCal', nuevoPin ? 'Calibración y PIN actualizados.' : 'Calibración guardada.', true);
        await poll();
      }
      b.classList.remove('busy');
    });

    syncHora();
    poll();
    pollHist();
    setInterval(poll, 2000);
    setInterval(pollHist, 6000);
    setInterval(syncHora, 300000);
  }

  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', init);
  else init();
})();
