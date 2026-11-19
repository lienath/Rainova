const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<meta name="theme-color" content="#2C4018">
<title>Rainova · Monitor de planta</title>
<style>
:root{
  /* Paleta tomada de la imagen de la planta: crema, verde oliva y violeta floral */
  --bg:#E9E4D9; --card:#FBF9F3; --ink:#241D14; --muted:#7C7263;
  --green:#3B561E; --accent:#7C9A3F; --accent2:#587A28; --flower:#B050C8;
  --amber:#D79A2B; --red:#C24A32; --line:#E6DDCB;
  --shadow:0 8px 24px rgba(36,29,20,.10);
}
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent}
html,body{margin:0;height:100%}
body{
  font-family:system-ui,-apple-system,"Segoe UI",Roboto,Helvetica,Arial,sans-serif;
  background:radial-gradient(circle at 20% 0%,#F3EFE5,var(--bg) 60%);
  color:var(--ink);display:flex;align-items:center;justify-content:center;overflow:hidden;
}
/* UN SOLO scroll: el del .phone. Header y nav quedan fijos dentro de él con sticky. */
.phone{
  position:relative;width:390px;height:844px;max-height:100vh;background:var(--bg);
  border-radius:42px;box-shadow:0 24px 70px rgba(36,29,20,.28);
  border:8px solid #241D14;display:block;
  overflow-y:auto;overflow-x:hidden;scrollbar-width:none;-webkit-overflow-scrolling:touch;
}
.phone::-webkit-scrollbar{display:none}
.notch{position:absolute;top:0;left:50%;transform:translateX(-50%);width:120px;height:22px;background:#241D14;border-radius:0 0 16px 16px;z-index:12}
header{
  position:sticky;top:0;z-index:10;
  padding:34px 20px 14px;display:flex;align-items:center;justify-content:space-between;
  background:linear-gradient(135deg,#2C4018,#6E8A38);color:#fff;
  box-shadow:inset 0 -3px 0 rgba(176,80,200,.55);
}
.brand{display:flex;align-items:center;gap:10px}
.brand .leaf{font-size:26px;filter:drop-shadow(0 2px 4px rgba(0,0,0,.3))}
.brand b{display:block;font-size:19px;letter-spacing:.3px}
.brand small{display:block;font-size:11px;opacity:.8}
.head-right{display:flex;align-items:center;gap:8px;font-size:13px;font-variant-numeric:tabular-nums}
.dot{width:8px;height:8px;border-radius:50%;background:var(--accent);box-shadow:0 0 0 4px rgba(124,154,63,.22)}
.dot.off{background:var(--red);box-shadow:0 0 0 4px rgba(194,74,50,.22)}
.banner{margin:10px 14px 0;padding:10px 14px;border-radius:14px;background:#FBEDE8;color:#9C3A24;font-size:13px;font-weight:600;border:1px solid #F2D3C8}
.hidden{display:none}
main{padding:14px 14px 18px}
.tab{display:none;animation:fade .25s ease}
.tab.active{display:block}
@keyframes fade{from{opacity:0;transform:translateY(6px)}to{opacity:1;transform:none}}
.card{background:var(--card);border-radius:20px;padding:16px;box-shadow:var(--shadow);margin-bottom:12px;border:1px solid rgba(36,29,20,.05)}
.card-title{font-size:13px;font-weight:700;color:var(--green);margin-bottom:12px;display:flex;justify-content:space-between;align-items:baseline}
.card-title small{font-size:11px;color:var(--muted);font-weight:500}
.hero{text-align:center;padding:18px 16px 14px}
.gauge-wrap{position:relative;width:170px;height:170px;margin:0 auto}
.gauge{width:100%;height:100%;transform:rotate(-90deg)}
.gauge circle{fill:none;stroke-width:11;stroke-linecap:round}
.g-bg{stroke:#EAE4D5}
.g-val{stroke:var(--accent2);stroke-dasharray:327;stroke-dashoffset:327;transition:stroke-dashoffset .7s ease,stroke .3s}
.gauge-label{position:absolute;inset:0;display:flex;flex-direction:column;align-items:center;justify-content:center}
.gauge-label div{font-size:46px;font-weight:800;line-height:1;color:var(--green);font-variant-numeric:tabular-nums}
.gauge-label small{font-size:18px;font-weight:600;color:var(--muted);margin-left:2px}
.gauge-label p{margin:6px 0 0;font-size:12px;color:var(--muted)}
.trend{margin-top:6px;font-size:12px;color:var(--muted);font-weight:600}
.grid2{display:grid;grid-template-columns:1fr 1fr;gap:12px}
.mini{display:flex;align-items:center;gap:10px;padding:14px 12px;margin-bottom:0}
.mini .ico{font-size:22px}
.mini small{display:block;font-size:10px;color:var(--muted);margin-bottom:3px}
.mini b{font-size:14px;color:var(--green)}
.mini.warn b{color:var(--red)}
.riego-main{display:flex;align-items:center;gap:16px}
.yeah{font-size:34px;font-weight:800;color:var(--accent2);min-width:76px}
.yeah.no{color:var(--amber)}
.riego-det{display:flex;flex-direction:column;gap:3px;font-size:12px;color:var(--muted)}
.riego-det span:first-child{color:var(--ink);font-weight:600}
.divider{height:1px;background:var(--line);margin:14px 0 10px}
.next-row{display:flex;justify-content:space-between;align-items:center;font-size:12px;color:var(--muted);padding:5px 0}
.next-row b{color:var(--ink);font-size:13px}
.ahorro-card{text-align:center}
.ahorro-val{font-size:40px;font-weight:800;color:var(--green);line-height:1}
.ahorro-val small{font-size:18px;color:var(--muted)}
.bar{height:10px;border-radius:10px;background:#E2DBCA;overflow:hidden;margin:14px 0 6px}
.bar-sis{height:100%;width:0;background:linear-gradient(90deg,var(--accent),var(--flower));border-radius:10px;transition:width .6s ease}
.bar-label{display:flex;justify-content:space-between;font-size:11px;color:var(--muted)}
.bar-label b{color:var(--ink)}
.chart{width:100%;height:150px;background:linear-gradient(#FBF9F3,#F7F4EA);border-radius:12px}
.chart .gl{stroke:var(--line);stroke-width:1}
.chart .cl{fill:none;stroke:var(--accent2);stroke-width:2.5;stroke-linejoin:round;stroke-linecap:round}
.chart-legend{display:flex;justify-content:space-between;font-size:10px;color:var(--muted);margin-top:4px}
.eventos{font-size:12px}
.evento{display:flex;gap:10px;padding:8px 0;border-bottom:1px solid var(--line)}
.evento:last-child{border:0}
.evento .t{color:var(--green);font-weight:700;font-variant-numeric:tabular-nums;min-width:46px}
.evento .d{color:var(--ink)}
.muted{color:var(--muted);font-size:12px}
.stats3{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;text-align:center;margin-bottom:14px}
.stats3 small{display:block;font-size:10px;color:var(--muted)}
.stats3 b{font-size:14px;color:var(--green)}
.btn-row{display:flex;flex-wrap:wrap;gap:8px}
.btn{flex:1;min-width:120px;border:1px solid var(--line);background:#fff;color:var(--green);border-radius:12px;padding:12px 10px;font-size:13px;font-weight:700;cursor:pointer;transition:.15s}
.btn:active{transform:scale(.97)}
.btn.primary{background:var(--green);color:#fff;border-color:var(--green)}
.btn.ghost{background:#F1EDE3;color:var(--muted)}
.btn.busy{opacity:.5;pointer-events:none}
.field{margin-bottom:10px}
.field label{display:block;font-size:11px;color:var(--muted);margin-bottom:4px}
.field input{width:100%;border:1px solid var(--line);border-radius:10px;padding:10px 12px;font-size:14px;color:var(--ink);background:#FBF9F3;outline:none}
.field input:focus{border-color:var(--accent2)}
.field input[type=password]{letter-spacing:6px;font-size:16px}
.hint{font-size:11px;color:var(--muted);line-height:1.5;margin:0 0 12px}
.msg{min-height:16px;margin:8px 0 0;font-size:11px;font-weight:600}
.msg.ok{color:var(--accent2)}
.msg.err{color:var(--red)}
nav{
  position:sticky;bottom:0;z-index:10;display:flex;background:#FBF9F3;
  border-top:1px solid var(--line);padding:8px 6px 14px;
}
.nav-btn{flex:1;border:0;background:transparent;color:var(--muted);font-size:10px;font-weight:600;display:flex;flex-direction:column;align-items:center;gap:2px;cursor:pointer;padding:6px 0;border-radius:12px}
.nav-btn span{font-size:19px;line-height:1}
.nav-btn.active{color:var(--flower);background:#F6E9F9}
@media (max-width:430px){
  body{display:block;overflow:hidden}
  .phone{width:100%;height:100vh;height:100dvh;max-height:none;border:0;border-radius:0;box-shadow:none}
  header{padding-top:38px}
}

</style>
</head>
<body>
<div class="phone">
  <header>
    <div class="notch"></div>
    <div class="brand">
      <div class="leaf">🌿</div>
      <div><b>Rainova</b><small>Regadera inteligente</small></div>
    </div>
    <div class="head-right">
      <span id="wifiDot" class="dot" title="ESP conectado"></span>
      <span id="clock">--:--</span>
    </div>
  </header>

  <div id="banner" class="banner hidden"></div>

  <main>
    <!-- INICIO -->
    <section id="tab-inicio" class="tab active">
      <div class="card hero">
        <div class="gauge-wrap">
          <svg viewBox="0 0 120 120" class="gauge">
            <circle cx="60" cy="60" r="52" class="g-bg"></circle>
            <circle cx="60" cy="60" r="52" class="g-val" id="gaugeArc"></circle>
          </svg>
          <div class="gauge-label">
            <div><span id="humedad">--</span><small>%</small></div>
            <p>Humedad del suelo</p>
          </div>
        </div>
        <div class="trend" id="tendencia">Midiendo…</div>
      </div>

      <div class="grid2">
        <div class="card mini" id="cardAgua">
          <div class="ico">💧</div>
          <div><small>Agua en tanque</small><b id="aguaTexto">--</b></div>
        </div>
        <div class="card mini" id="cardEstado">
          <div class="ico">⚙️</div>
          <div><small>Estado</small><b id="estadoTexto">--</b></div>
        </div>
      </div>

      <div class="card riego-card">
        <div class="card-title">¿Ya regó hoy?</div>
        <div class="riego-main">
          <div id="yaRego" class="yeah">--</div>
          <div class="riego-det">
            <span id="ultimoRiego">Sin registros hoy</span>
            <span id="mlHoy">0 mL usados hoy</span>
            <span id="riegosHoy">0 ciclos</span>
          </div>
        </div>
        <div class="divider"></div>
        <div class="next-row">
          <span>Próximo riego estimado</span>
          <b id="proximo">--</b>
        </div>
      </div>

      <div class="card ahorro-card">
        <div class="card-title">Ahorro de agua <small>vs. riego a ojo</small></div>
        <div class="ahorro-val"><span id="ahorroPct">--</span><small>%</small></div>
        <div class="bar"><div id="barSistema" class="bar-sis"></div></div>
        <div class="bar-label"><span>Sistema <b id="mlSistema">-- mL</b></span><span>Riego manual <b id="mlManual">-- mL</b></span></div>
      </div>
    </section>

    <!-- HISTÓRICO -->
    <section id="tab-historico" class="tab">
      <div class="card">
        <div class="card-title">Humedad <small>últimos 10 minutos</small></div>
        <svg id="chart" class="chart" viewBox="0 0 320 150" preserveAspectRatio="none">
          <line x1="0" y1="60" x2="320" y2="60" class="gl" stroke-dasharray="4 4"></line>
          <line x1="0" y1="98" x2="320" y2="98" class="gl"></line>
          <polyline id="chartLine" points="" class="cl"></polyline>
        </svg>
        <div class="chart-legend"><span>60% óptimo</span><span>35% seco</span></div>
      </div>
      <div class="card">
        <div class="card-title">Eventos recientes <small>hora del día</small></div>
        <div id="eventos" class="eventos"><p class="muted">Sin eventos todavía.</p></div>
      </div>
      <div class="card">
        <div class="card-title">Consumo</div>
        <div class="stats3">
          <div><small>Hoy</small><b id="statHoy">0 mL</b></div>
          <div><small>Total</small><b id="statTotal">0 mL</b></div>
          <div><small>Ciclos hoy</small><b id="statCiclos">0</b></div>
        </div>
        <button class="btn ghost" id="btnCsv">Descargar CSV</button>
      </div>
    </section>

    <!-- CONTROL -->
    <section id="tab-control" class="tab">
      <div class="card">
        <div class="card-title">Control manual</div>
        <div class="btn-row">
          <button class="btn primary" id="btnRegar">💧 Regar ahora</button>
          <button class="btn" id="btnPausa">⏸ Pausa</button>
          <button class="btn" id="btnReanudar">▶ Reanudar</button>
        </div>
        <p class="msg" id="msgControl"></p>
        <p class="hint">El riego automático sigue activo aunque el celular se desconecte.</p>
      </div>

      <div class="card">
        <div class="card-title">Calibración del sensor <small>🔒 con PIN</small></div>
        <p class="hint">Lectura cruda actual: <b id="crudo">--</b>. Anota el valor al aire y en agua hasta la línea blanca.</p>
        <div class="field"><label>Seco (aire)</label><input id="inSeco" type="number" inputmode="numeric"></div>
        <div class="field"><label>Mojado (agua)</label><input id="inMojado" type="number" inputmode="numeric"></div>
        <div class="field"><label>mL por segundo</label><input id="inMl" type="number" step="0.1" inputmode="decimal"></div>
        <div class="field"><label>PIN de acceso (4 dígitos · inicial 1234)</label><input id="inPin" type="password" inputmode="numeric" maxlength="4" placeholder="••••"></div>
        <div class="field"><label>Nuevo PIN (opcional, 4 dígitos)</label><input id="inNuevoPin" type="password" inputmode="numeric" maxlength="4" placeholder="Sin cambio"></div>
        <div class="btn-row">
          <button class="btn primary" id="btnGuardarCal">Guardar calibración</button>
          <button class="btn ghost" id="btnUsarSeco">Usar lectura actual como seco</button>
          <button class="btn ghost" id="btnUsarMojado">Usar lectura actual como mojado</button>
        </div>
        <p class="msg" id="msgCal"></p>
      </div>

      <div class="card">
        <div class="card-title">Sistema</div>
        <div class="next-row"><span>Red WiFi</span><b>Rainova-ESP</b></div>
        <div class="next-row"><span>Dirección</span><b>192.168.4.1</b></div>
        <div class="next-row"><span>Encendido</span><b id="uptime">--</b></div>
        <button class="btn ghost" id="btnReset">Reiniciar contadores del día</button>
      </div>
    </section>
  </main>

  <nav>
    <button class="nav-btn active" data-tab="inicio"><span>🏠</span>Inicio</button>
    <button class="nav-btn" data-tab="historico"><span>📈</span>Histórico</button>
    <button class="nav-btn" data-tab="control"><span>🎛</span>Control</button>
  </nav>
</div>
<script>
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

</script>
</body>
</html>

)rawliteral";
