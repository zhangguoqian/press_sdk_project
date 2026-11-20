/* =========================================================
   自动热压机 · 上位机 UI — interaction layer
   ========================================================= */
(() => {
  'use strict';

  const $ = (s, r = document) => r.querySelector(s);
  const $$ = (s, r = document) => [...r.querySelectorAll(s)];
  const fmt = (v, n = 1) => Number(v).toFixed(n);
  const mmss = (sec) => {
    sec = Math.max(0, Math.round(sec));
    return String(Math.floor(sec / 60)).padStart(2, '0') + ':' + String(sec % 60).padStart(2, '0');
  };

  /* ---------------------------------------------------------
     state
     --------------------------------------------------------- */
  const S = {
    view: 'single',
    press: false,
    heat: false,
    coopRunning: false,
    estop: false,
    pStage: 0,          // 1-based current, 0 = idle
    tStage: 0,
    coopStage: 0,
    coopRemain: 0,
    liveP: 0,
    upT: -1.0,
    downT: 0.0,
    pRemain: 600,
    tRemain: 500,
    runtime: 0,
    output: 0,
    moldShape: 'circle',   // special | circle | rect
    moldDia: 100,
    moldLen: 100,
    moldWid: 100,
    rateMode: 'normal',    // normal | fast
    coolT: 15.0,
    warmT: 45.0,
    pCount: 30,
    tCount: 30,
    coopCount: 30,
    heatMode: 0,
    // 压力流程：30 段，压力 1–30 Ton（额定 30 Ton），补压设定比压力低 0.3 Ton
    pStages: Array.from({ length: 30 }, (_, i) => ({
      p: +(i + 1).toFixed(1),
      boost: +(i + 1 - 0.3).toFixed(1),
      hold: 600,
    })),
    // 温度流程：30 段，30–175 °C，每段升温 5 °C
    tStages: Array.from({ length: 30 }, (_, i) => ({
      t: +(30 + i * 5).toFixed(1),
      rate: 3.0,
      hold: 500,
    })),
    // 合作流程：独立可编辑数据
    coopRows: Array.from({ length: 30 }, (_, i) => ({
      press: true,
      p: +(i + 1).toFixed(1),
      boost: +(i + 1 - 0.3).toFixed(1),
      heat: true,
      heatDown: true,
      tUp: +(30 + i * 5).toFixed(1),
      tDown: +(30 + i * 5).toFixed(1),
      hold: 600,
    })),
  };

  const HEAT_MODES = ['速率', '时间'];
  const PI = Math.PI;

  /** 模具受力面积 mm²；异形模具无固定公式，返回 0 */
  const moldArea = () => {
    if (S.moldShape === 'circle') return PI * (Math.max(1, S.moldDia) / 2) ** 2;
    if (S.moldShape === 'rect') return Math.max(1, S.moldLen) * Math.max(1, S.moldWid);
    return 0;
  };

  /** 模具压强 P = F / A = (Ton × 1000 kgf × 9.80665 N/kgf) / 模具面积(mm²) → MPa */
  const moldPressure = (ton) => {
    const area = moldArea();
    return area ? (ton * 9806.65) / area : null;
  };

  /** 受力面积参考值（cm²）；异形模具无固定面积 */
  const moldAreaText = () => {
    const area = moldArea();
    return area ? fmt(area / 100, 1) : '—';
  };

  /* ---------------------------------------------------------
     toast / modal
     --------------------------------------------------------- */
  let toastTimer;
  const toast = (msg) => {
    $('#toastText').textContent = msg;
    $('#toast').classList.add('is-on');
    clearTimeout(toastTimer);
    toastTimer = setTimeout(() => $('#toast').classList.remove('is-on'), 2200);
  };

  let modalOk = null;
  const modal = (opts) => {
    const { title, text, ok = '确认', cancel = '返回', tone = 'info', icon = 'i-info', onOk } = opts;
    $('#modalTitle').textContent = title || '提示';
    $('#modalText').textContent = text || '';
    const box = $('#modalIcon');
    box.className = 'mi' + (tone === 'warn' ? ' warn' : tone === 'bad' ? ' bad' : '');
    box.innerHTML = `<svg class="ic"><use href="#${icon}"/></svg>`;
    $('#modalOk').textContent = ok;
    $('#modalCancel').textContent = cancel;
    $('#modalOk').className = 'btn ' + (tone === 'bad' ? 'btn-red' : tone === 'warn' ? 'btn-amber' : 'btn-brand');
    modalOk = onOk || null;
    $('#overlay').classList.add('is-open');
  };
  const closeModal = () => { $('#overlay').classList.remove('is-open'); modalOk = null; };

  $('#modalCancel').onclick = closeModal;
  $('#modalOk').onclick = () => { const f = modalOk; closeModal(); f && f(); };
  $('#overlay').onclick = (e) => { if (e.target === $('#overlay')) closeModal(); };
  document.addEventListener('keydown', (e) => { if (e.key === 'Escape' && $('#overlay').classList.contains('is-open')) closeModal(); });

  /* ---------------------------------------------------------
     run log（底栏上方，可展开/收起）
     --------------------------------------------------------- */
  const LOGS = [];
  const escapeHtml = (s) => s.replace(/[&<>]/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;' }[c]));

  const renderLogs = () => {
    const level = $('#logLevel').value;
    const kw = $('#logSearch').value.trim().toLowerCase();
    const list = LOGS.filter((l) => (level === 'all' || l.level === level)
      && (!kw || l.msg.toLowerCase().includes(kw)));
    $('#logBody').innerHTML = list.map((l) =>
      `<div class="log-line ${l.level}"><span class="t">${l.ts}</span><span class="m">${escapeHtml(l.msg)}</span></div>`).join('');
    $('#logCount').textContent = `${list.length} 条`;
    $('#logBody').scrollTop = $('#logBody').scrollHeight;
  };

  const log = (msg, level = 'info') => {
    const d = new Date();
    const ts = [d.getHours(), d.getMinutes(), d.getSeconds()]
      .map((n) => String(n).padStart(2, '0')).join(':');
    LOGS.push({ ts, msg, level });
    if (LOGS.length > 500) LOGS.shift();
    renderLogs();
  };

  const setLogOpen = (open) => {
    $('#logPanel').classList.toggle('is-open', open);
    $('#btnLog').classList.toggle('is-active', open);
  };

  $('#btnLog').onclick = () => setLogOpen(!$('#logPanel').classList.contains('is-open'));
  $('#logCollapse').onclick = () => setLogOpen(false);
  $('#logLevel').onchange = renderLogs;
  $('#logSearch').oninput = renderLogs;
  $('#logClear').onclick = () => { LOGS.length = 0; renderLogs(); toast('日志已清空'); };
  $('#logExport').onclick = () => {
    const text = LOGS.map((l) => `${l.ts} [${l.level}] ${l.msg}`).join('\n');
    const a = document.createElement('a');
    a.href = URL.createObjectURL(new Blob([text], { type: 'text/plain;charset=utf-8' }));
    a.download = `上位机日志_${new Date().toISOString().slice(0, 10)}.txt`;
    a.click();
    URL.revokeObjectURL(a.href);
    toast('日志已导出');
  };

  /* ---------------------------------------------------------
     设备连接（启动时先选择串口 / 网络 / 本地模拟）
     --------------------------------------------------------- */
  let connMode = 'serial';
  $$('#connSeg button').forEach((b) => {
    b.onclick = () => {
      connMode = b.dataset.conn;
      $$('#connSeg button').forEach((x) => x.classList.toggle('is-on', x === b));
      $$('.conn-pane').forEach((p) => p.classList.toggle('is-on', p.dataset.connPane === connMode));
    };
  });

  $('#connOk').onclick = () => {
    const label = connMode === 'serial'
      ? `串口 ${$('#connPort').value}`
      : connMode === 'net'
        ? `网络 ${$('#connHost').value}:${$('#connPortTcp').value}`
        : '本地模拟';
    $('#connectOverlay').classList.remove('is-open');
    const small = $('.rail-stat small');
    if (small) small.textContent = label;
    log(`设备连接成功：${label}`);
    toast(connMode === 'sim' ? '已进入本地模拟运行' : '设备已连接');
  };

  $('#connExit').onclick = () => {
    $('#connectOverlay').classList.remove('is-open');
    log('用户选择退出', 'warn');
    window.close();
    setTimeout(() => {
      if (!window.closed) toast('已退出（原型演示，可直接关闭页面）');
    }, 80);
  };

  /* ---------------------------------------------------------
     navigation
     --------------------------------------------------------- */
  const VIEWS = {
    single: { bar: 'run' },
    coop:   { bar: 'coop' },
    settings: { bar: 'set' },
  };

  const go = (name) => {
    if (!VIEWS[name]) return;
    // 运行中禁止切换模式 / 页面，须先停止
    if (name !== S.view && (S.press || S.heat || S.coopRunning)) {
      toast('设备运行中，请先停止后再切换');
      return;
    }
    S.view = name;
    $$('.rail-item').forEach((b) => b.classList.toggle('is-active', b.dataset.nav === name));
    $$('.view').forEach((v) => v.classList.toggle('is-active', v.dataset.view === name));
    $$('.bar-group').forEach((g) => g.classList.toggle('is-on', g.dataset.bar === VIEWS[name].bar));
    $('#content').scrollTop = 0;
    // 实时曲线卡片跟随当前模式（单独 / 合作）移动
    const slot = name === 'single' ? $('#singleChartSlot') : name === 'coop' ? $('#coopChartSlot') : null;
    const chartCard = $('#chartCard');
    if (slot && chartCard.parentElement !== slot) slot.appendChild(chartCard);
    if (name === 'single' || name === 'coop') requestAnimationFrame(resizeCharts);
  };

  $$('.rail-item').forEach((b) => { b.onclick = () => go(b.dataset.nav); });

  /* ---------------------------------------------------------
     stage tables
     --------------------------------------------------------- */
  // 当前阶段行超出可视区时自动滚动到视野内
  const revealNowRow = () => {
    $$('.tbl-wrap').forEach((wrap) => {
      const row = wrap.querySelector('tr.is-now');
      if (!row) return;
      const wt = wrap.getBoundingClientRect();
      const rt = row.getBoundingClientRect();
      if (rt.top < wt.top) wrap.scrollTop -= wt.top - rt.top;
      else if (rt.bottom > wt.bottom) wrap.scrollTop += rt.bottom - wt.bottom;
    });
  };

  // 运行中锁定其他模式入口的视觉状态
  const updateRailLock = () => {
    const running = S.press || S.heat || S.coopRunning;
    $$('.rail-item').forEach((b) => {
      const locked = running && b.dataset.nav !== S.view;
      b.classList.toggle('is-locked', locked);
      b.setAttribute('aria-disabled', locked ? 'true' : 'false');
    });
  };

  // 可编辑单元格：阶段列固定，数值列直接输入，计时列点击弹窗编辑
  const cellInput = (tbl, i, k, val, cls = '') => `
    <td class="edit ${cls}"><input class="cell-in" type="number" inputmode="decimal" step="0.1"
      data-t="${tbl}" data-i="${i}" data-k="${k}" value="${val}"></td>`;

  const cellTime = (tbl, i, sec) => `
    <td class="edit mono"><button type="button" class="cell-in time-btn"
      data-t="${tbl}" data-i="${i}" data-time="1">${mmss(sec)}</button></td>`;

  const renderStageTables = () => {
    const tbP = $('#tblP tbody');
    tbP.innerHTML = S.pStages.slice(0, S.pCount).map((s, i) => `
      <tr class="${S.press && S.pStage === i + 1 ? 'is-now' : ''}">
        <td><span class="row-no">${i + 1}</span></td>
        ${cellInput('p', i, 'p', fmt(s.p), 'num')}
        ${cellInput('p', i, 'boost', fmt(s.boost), 'num')}
        ${cellTime('p', i, s.hold)}
      </tr>`).join('');

    const tbT = $('#tblT tbody');
    tbT.innerHTML = S.tStages.slice(0, S.tCount).map((s, i) => `
      <tr class="${S.heat && S.tStage === i + 1 ? 'is-now' : ''}">
        <td><span class="row-no">${i + 1}</span></td>
        ${cellInput('t', i, 't', fmt(s.t), 'num')}
        ${cellInput('t', i, 'rate', fmt(s.rate), 'mono')}
        ${cellTime('t', i, s.hold)}
      </tr>`).join('');

    revealNowRow();
  };

  const renderCoopTable = () => {
    $('#tblCoop tbody').innerHTML = S.coopRows.slice(0, S.coopCount).map((r, i) => {
      const now = S.coopRunning && S.coopStage === i + 1 ? 'is-now' : '';
      return `
        <tr class="${now}">
          <td><span class="row-no">${i + 1}</span></td>
          <td><button type="button" class="flag ${r.press ? 'yes' : 'off'}" data-t="coop" data-i="${i}" data-k="press" data-flag="1">${r.press ? 'Y' : '—'}</button></td>
          ${cellInput('coop', i, 'p', fmt(r.p), 'num')}
          ${cellInput('coop', i, 'boost', fmt(r.boost), 'num')}
          <td><button type="button" class="flag ${r.heat ? 'yes' : 'off'}" data-t="coop" data-i="${i}" data-k="heat" data-flag="1">${r.heat ? 'Y' : '—'}</button></td>
          ${cellInput('coop', i, 'tUp', fmt(r.tUp), 'num')}
          <td><button type="button" class="flag ${r.heatDown ? 'yes' : 'off'}" data-t="coop" data-i="${i}" data-k="heatDown" data-flag="1">${r.heatDown ? 'Y' : '—'}</button></td>
          ${cellInput('coop', i, 'tDown', fmt(r.tDown), 'num')}
          ${cellTime('coop', i, r.hold)}
        </tr>`;
    }).join('');

    revealNowRow();
  };

  /* ---------- 表格内联编辑 ---------- */
  const CELL_TABLES = { p: 'pStages', t: 'tStages', coop: 'coopRows' };
  const CELL_RANGE = {
    p: { p: [0.1, 30], boost: [0, 30] },
    t: { t: [0, 310], rate: [0.1, 50] },
    coop: { p: [0.1, 30], boost: [0, 30], tUp: [0, 310], tDown: [0, 310] },
  };
  const cellRow = (el) => {
    const arr = S[CELL_TABLES[el.dataset.t]];
    return arr ? arr[+el.dataset.i] : null;
  };

  document.addEventListener('input', (e) => {
    const el = e.target.closest('input.cell-in');
    if (!el) return;
    const row = cellRow(el);
    const v = Number(el.value);
    if (row && el.value !== '' && isFinite(v)) { row[el.dataset.k] = v; paintValues(); }
  });

  document.addEventListener('change', (e) => {
    const el = e.target.closest('input.cell-in');
    if (!el) return;
    const row = cellRow(el);
    if (!row) return;
    const k = el.dataset.k;
    const num = Number(el.value);
    if (el.value === '' || !isFinite(num)) { el.value = fmt(row[k]); return; }
    const [lo, hi] = CELL_RANGE[el.dataset.t][k];
    row[k] = Math.min(hi, Math.max(lo, Math.round(num * 10) / 10));
    el.value = fmt(row[k]);
    paintValues();
  });

  // 是否加压 / 是否加热：点击切换
  document.addEventListener('click', (e) => {
    const btn = e.target.closest('[data-flag]');
    if (!btn) return;
    const row = cellRow(btn);
    if (!row) return;
    const k = btn.dataset.k;
    row[k] = !row[k];
    btn.classList.toggle('yes', row[k]);
    btn.classList.toggle('off', !row[k]);
    btn.textContent = row[k] ? 'Y' : '—';
  });

  /* ---------- 计时弹窗（图形化编辑） ---------- */
  const tpOverlay = $('#timeOverlay');
  const tpMin = $('#tpMin');
  const tpSec = $('#tpSec');
  const tpPreview = $('#timePreview');
  let tpTarget = null;

  const syncTp = () => {
    const m = Math.min(599, Math.max(0, Math.round(Number(tpMin.value) || 0)));
    const s = Math.min(59, Math.max(0, Math.round(Number(tpSec.value) || 0)));
    tpMin.value = m;
    tpSec.value = s;
    tpPreview.textContent = mmss(m * 60 + s);
  };

  const closeTp = () => {
    tpOverlay.classList.remove('is-open');
    tpTarget = null;
  };

  const openTp = (el) => {
    const row = cellRow(el);
    if (!row) return;
    tpTarget = el;
    tpMin.value = Math.floor(row.hold / 60);
    tpSec.value = row.hold % 60;
    syncTp();
    tpOverlay.classList.add('is-open');
    tpMin.focus();
    tpMin.select();
  };

  tpMin.oninput = syncTp;
  tpSec.oninput = syncTp;

  $$('.tp-step').forEach((b) => {
    b.onclick = () => {
      const input = b.dataset.tp === 'min' ? tpMin : tpSec;
      const max = b.dataset.tp === 'min' ? 599 : 59;
      input.value = Math.min(max, Math.max(0, (Number(input.value) || 0) + Number(b.dataset.step)));
      syncTp();
    };
  });

  $$('.tp-presets button').forEach((b) => {
    b.onclick = () => {
      const sec = Number(b.dataset.preset);
      tpMin.value = Math.floor(sec / 60);
      tpSec.value = sec % 60;
      syncTp();
    };
  });

  $('#tpCancel').onclick = closeTp;
  tpOverlay.onclick = (e) => { if (e.target === tpOverlay) closeTp(); };

  $('#tpOk').onclick = () => {
    syncTp();
    const row = tpTarget ? cellRow(tpTarget) : null;
    if (row) {
      row.hold = Math.min(35999, Number(tpMin.value) * 60 + Number(tpSec.value));
      if (tpTarget.isConnected) tpTarget.textContent = mmss(row.hold);
      paintValues();
    }
    closeTp();
  };

  document.addEventListener('click', (e) => {
    const btn = e.target.closest('[data-time]');
    if (btn) openTp(btn);
  });

  document.addEventListener('keydown', (e) => {
    if (e.key === 'Escape' && tpOverlay.classList.contains('is-open')) closeTp();
  });

  /* 阶段数切换（1–30，spinbox） */
  const STAGE_SPINS = [
    ['#pCountSpin', '#pCountInput', 'pCount'],
    ['#tCountSpin', '#tCountInput', 'tCount'],
    ['#coopCountSpin', '#coopCountInput', 'coopCount'],
  ];

  const applyStageCounts = () => {
    if (S.press) { S.pStage = Math.min(S.pStage, S.pCount); S.pRemain = S.pStages[S.pStage - 1].hold; }
    if (S.heat) { S.tStage = Math.min(S.tStage, S.tCount); S.tRemain = S.tStages[S.tStage - 1].hold; }
    if (S.coopRunning) { S.coopStage = Math.min(S.coopStage, S.coopCount); S.coopRemain = S.coopRows[S.coopStage - 1].hold; }
    renderStageTables(); renderCoopTable(); paintValues();
  };

  const initCountSpins = () => {
    STAGE_SPINS.forEach(([wrapSel, inputSel, key]) => {
      const wrap = $(wrapSel);
      const input = $(inputSel);
      const apply = (v) => {
        S[key] = Math.min(30, Math.max(1, Math.round(Number(v) || 1)));
        input.value = S[key];
        applyStageCounts();
      };
      $$('button', wrap).forEach((b) => { b.onclick = () => apply(S[key] + Number(b.dataset.step)); });
      input.onchange = () => apply(input.value);
    });
  };

  /* ---------------------------------------------------------
     live values
     --------------------------------------------------------- */
  const paintValues = () => {
    const ps = S.pStages[Math.min(S.pStage || 1, S.pCount) - 1] || S.pStages[0];
    $('#vSetP').textContent = fmt(ps.p);
    $('#vBoost').textContent = fmt(ps.boost);
    $('#vLiveP').textContent = fmt(S.liveP);
    const mp = moldPressure(S.liveP);
    $('#vMold').textContent = mp === null ? '—' : fmt(mp, 2);
    $('#vHoldP').textContent = mmss(ps.hold);

    const ts = S.tStages[Math.min(S.tStage || 1, S.tCount) - 1] || S.tStages[0];
    $('#vSetT').textContent = fmt(ts.t);
    $('#vUpT').textContent = fmt(S.upT);
    $('#vDownT').textContent = fmt(S.downT);
    $('#vHoldSet').textContent = mmss(ts.hold);

    $('#btnPressTxt').textContent = S.press ? '停止加压' : '启动加压';
    $('#btnHeatTxt').textContent = S.heat ? '停止加热' : '启动加热';

    const running = S.press || S.heat || S.coopRunning;
    document.body.classList.toggle('is-running', running);
    $('#btnPress').classList.toggle('is-stop', S.press);
    $('#btnHeat').classList.toggle('is-stop', S.heat);
    $('#btnCoopStart').classList.toggle('is-stop', S.coopRunning);

    $('#cnP').textContent = fmt(S.liveP);
    $('#cnUp').textContent = fmt(S.upT);
    $('#cnDown').textContent = fmt(S.downT);

    $('#pStageNow').textContent = S.pStage || 0;
    $('#tStageNow').textContent = S.tStage || 0;
    $('#pStageTotal').textContent = S.pCount;
    $('#tStageTotal').textContent = S.tCount;
    $('#coopStageTotal').textContent = S.coopCount;

    const stateTxt = S.estop ? '急停' : S.press && S.heat ? '加压 + 加热中' : S.press ? '加压中' : S.heat ? '加热中' : S.coopRunning ? '合作运行中' : '待机';
    const ci = Math.min(S.coopStage || 1, S.coopCount);
    const cr = S.coopRows[ci - 1];
    const cmp = moldPressure(S.liveP);
    $('#cSetP').textContent = fmt(cr.p);
    $('#cMold').textContent = cmp === null ? '—' : fmt(cmp, 2);
    $('#cHold').textContent = mmss(cr.hold);
    $('#cSetTUp').textContent = fmt(cr.tUp);
    $('#cSetTDown').textContent = fmt(cr.tDown);
    $('#cLiveP').textContent = fmt(S.liveP);
    $('#cUpT').textContent = fmt(S.upT);
    $('#cDownT').textContent = fmt(S.downT);
    $('#coopStageNow').textContent = S.coopRunning ? S.coopStage : 0;
    $('#btnCoopTxt').textContent = S.coopRunning ? '停止' : '启动';

    $('#abNote').innerHTML = S.estop
      ? '<b style="color:#A50D25">急停生效</b> · 排除故障并解除急停后方可重新启动'
      : S.press || S.heat || S.coopRunning
        ? `<b>${stateTxt}</b>`
        : '<b>待机</b> · 启动加热前请确认上 / 下板温度与模具已就位';

    updateRailLock();
  };

  /* ---------------------------------------------------------
     simulation
     --------------------------------------------------------- */
  setInterval(() => {
    if (S.estop) return;
    let dirty = false;

    if (S.press) {
      const target = S.pStages[S.pStage - 1].p;
      const next = S.liveP + (target - S.liveP) * 0.18 + (Math.random() - 0.5) * 0.03;
      S.liveP = Math.min(target * 1.02, Math.max(0, next));
      S.pRemain -= 30;
      if (S.pRemain <= 0) {
        if (S.pStage < S.pCount) { S.pStage++; S.pRemain = S.pStages[S.pStage - 1].hold; }
        else { S.pStage = 0; S.press = false; S.liveP = 0; S.output++; S.pRemain = 0; }
        renderStageTables();
      }
      dirty = true;
    } else if (S.liveP > 0) {
      S.liveP = Math.max(0, S.liveP - S.liveP * 0.3);
      if (S.liveP < 0.02) S.liveP = 0;
      dirty = true;
    }

    if (S.heat) {
      const ts = S.tStages[S.tStage - 1];
      const d = ts.t - S.upT;
      S.upT += Math.sign(d) * Math.min(Math.abs(d), Math.max(0.08, Math.abs(d) * 0.08)) + (Math.random() - 0.5) * 0.05;
      S.downT += ((ts.t - 0.6) - S.downT) * 0.1;
      S.tRemain -= 12;
      if (S.tRemain <= 0) {
        if (S.tStage < S.tCount) { S.tStage++; S.tRemain = S.tStages[S.tStage - 1].hold; }
        else { S.heat = false; S.tStage = 0; }
        renderStageTables();
      }
      dirty = true;
    }

    if (S.coopRunning) {
      S.coopRemain -= 30;
      if (S.coopRemain <= 0) {
        if (S.coopStage < S.coopCount) {
          S.coopStage++;
          S.coopRemain = S.coopRows[S.coopStage - 1].hold;
        } else {
          S.coopRunning = false; S.coopStage = 0; S.coopRemain = 0;
        }
        renderCoopTable();
      }
      dirty = true;
    }

    if (S.press || S.heat || S.coopRunning) { S.runtime += 0.5; dirty = true; }
    if (dirty) paintValues();
  }, 500);

  /* ---------------------------------------------------------
     actions
     --------------------------------------------------------- */
  $('#btnPress').onclick = () => {
    if (S.press) {
      S.press = false; S.pStage = 0; S.pRemain = 0;
      renderStageTables(); paintValues(); toast('加压已停止');
      log('加压已停止');
      return;
    }
    modal({
      title: '启动加压', text: `将按压力流程执行 ${S.pCount} 段加压，首段 ${fmt(S.pStages[0].p)} Ton / 保压 ${mmss(S.pStages[0].hold)}。请确认人员已离开压机工作区。`,
      icon: 'i-power', ok: '确认启动', onOk: () => {
        S.press = true; S.pStage = 1; S.pRemain = S.pStages[0].hold; S.runtime = S.runtime || 0;
        renderStageTables(); paintValues(); toast('加压已启动');
        log(`启动加压：${S.pCount} 段，首段 ${fmt(S.pStages[0].p)} Ton`);
      },
    });
  };

  $('#btnHeat').onclick = () => {
    if (S.heat) {
      S.heat = false; S.tStage = 0;
      renderStageTables(); paintValues(); toast('加热已停止');
      log('加热已停止');
      return;
    }
    modal({
      title: '启动加热', text: `将按温度流程执行 ${S.tCount} 段升温，末段目标 ${fmt(S.tStages[S.tCount - 1].t)} °C，升温速率 ${fmt(S.tStages[0].rate)} °C/min。`,
      icon: 'i-flame', tone: 'warn', ok: '确认启动', onOk: () => {
        S.heat = true; S.tStage = 1; S.tRemain = S.tStages[0].hold;
        if (S.upT <= 0) S.upT = 24.5;
        if (S.downT <= 0) S.downT = 23.8;
        renderStageTables(); paintValues(); toast('加热已启动');
        log(`启动加热：${S.tCount} 段，末段 ${fmt(S.tStages[S.tCount - 1].t)} °C`);
      },
    });
  };

  $('#btnVent').onclick = () => {
    modal({
      title: '排气', text: '将打开排气阀释放管路压力，请确认模具与人员安全。',
      icon: 'i-air', tone: 'warn', ok: '确认排气', onOk: () => {
        S.liveP = 0;
        paintValues(); toast('排气完成，压力已释放');
        log('排气指令已下发，压力已释放');
      },
    });
  };

  /* ---------- 合作模式启停 ---------- */
  $('#btnCoopStart').onclick = () => {
    S.coopRunning = !S.coopRunning;
    S.coopStage = S.coopRunning ? 1 : 0;
    S.coopRemain = S.coopRunning ? S.coopRows[0].hold : 0;
    renderCoopTable();
    paintValues();
    toast(S.coopRunning ? '合作模式已启动' : '合作模式已停止');
    log(S.coopRunning ? '合作模式已启动' : '合作模式已停止');
  };

  /* ---------- 模具类型（单独 / 合作各一份控件，共享同一套设定） ---------- */
  const MOLD_INPUTS = [
    ['#moldDia', 'moldDia'], ['#moldLen', 'moldLen'], ['#moldWid', 'moldWid'],
    ['#cMoldDia', 'moldDia'], ['#cMoldLen', 'moldLen'], ['#cMoldWid', 'moldWid'],
  ];

  const syncMoldUI = (except) => {
    $$('.mold-seg button[data-mold]').forEach((b) => b.classList.toggle('is-on', b.dataset.mold === S.moldShape));
    $$('.mold-pane').forEach((p) => p.classList.toggle('is-on', p.dataset.moldPane === S.moldShape));
    MOLD_INPUTS.forEach(([sel, key]) => {
      const el = $(sel);
      if (el && el !== except) el.value = S[key].toFixed(1);
    });
    // 受力面积：圆形 / 矩形时显示，异形隐藏
    $$('[data-mold-area]').forEach((el) => {
      el.classList.toggle('is-off', S.moldShape === 'special');
      const b = $('b', el);
      if (b) b.textContent = moldAreaText();
    });
  };

  const setMoldShape = (shape) => {
    S.moldShape = shape;
    syncMoldUI();
    paintValues();
  };

  $$('.mold-seg button[data-mold]').forEach((b) => { b.onclick = () => setMoldShape(b.dataset.mold); });

  MOLD_INPUTS.forEach(([sel, key]) => {
    const el = $(sel);
    el.oninput = () => {
      // 模具尺寸保留 1 位小数
      S[key] = Math.max(1, Math.round((Number(el.value) || 1) * 10) / 10);
      syncMoldUI(el);
      paintValues();
    };
    el.onchange = () => { el.value = S[key].toFixed(1); };
  });

  /* ---------- 速率设置 ---------- */
  const RATE_TIPS = {
    normal: '普通模式：按温度段设定直接升温，不做速率限制。',
    fast: '速率模式：按各段速率（°C/min）线性升温，自动校核升温时长。',
  };

  const applyRateMode = () => {
    const fast = S.rateMode === 'fast';
    const tbl = $('#tblT');
    tbl.classList.toggle('no-rate', !fast);
    $('colgroup', tbl).innerHTML = fast
      ? '<col style="width:64px"><col><col><col>'
      : '<col style="width:64px"><col><col>';
  };

  $$('#rateSeg button').forEach((b) => {
    b.onclick = () => {
      S.rateMode = b.dataset.rate;
      $$('#rateSeg button').forEach((x) => x.classList.toggle('is-on', x === b));
      $('#rateTip').textContent = RATE_TIPS[S.rateMode];
      applyRateMode();
    };
  });

  /* ---------- 水冷设置 ---------- */
  const bindTemp1 = (sel, key) => {
    const el = $(sel);
    el.oninput = () => { S[key] = Math.max(0, Math.round((Number(el.value) || 0) * 10) / 10); };
    el.onchange = () => { el.value = S[key].toFixed(1); };
  };
  bindTemp1('#coolT', 'coolT');
  bindTemp1('#warmT', 'warmT');

  /* ---------------------------------------------------------
     chart
     --------------------------------------------------------- */
  const WINDOW = 360;                        // 采样点：2 Hz × 180 s
  const hist = { p: [], t: [], t2: [] };
  const charts = [];

  function setupCanvas(cv) {
    const dpr = Math.min(window.devicePixelRatio || 1, 2);
    const r = cv.getBoundingClientRect();
    const w = Math.floor(r.width);
    const h = Math.floor(r.height);
    // 视图隐藏或尚未完成布局时不绘制，等 ResizeObserver 再次触发
    if (w < 20 || h < 20) return null;
    cv.width = Math.round(w * dpr);
    cv.height = Math.round(h * dpr);
    const ctx = cv.getContext('2d');
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    return { ctx, w, h };
  }

  function drawChart(cv, ctx, w, h, data, series, opts = {}) {
    ctx.clearRect(0, 0, w, h);
    // 填充白底：避免 Edge 合成层在滚动时透出页面背景形成残影
    ctx.fillStyle = '#FFFFFF';
    ctx.fillRect(0, 0, w, h);
    const pad = { l: 34, r: series === 'both' ? 40 : 14, t: 12, b: 22 };
    const cw = w - pad.l - pad.r;
    const ch = h - pad.t - pad.b;
    if (cw <= 10 || ch <= 10) return;

    const pMax = opts.pMax || 30, tMin = 0, tMax = opts.tMax || 80;

    // grid
    ctx.strokeStyle = '#EDF2F7';
    ctx.lineWidth = 1;
    ctx.font = '10px Bahnschrift, Segoe UI, sans-serif';
    ctx.textBaseline = 'middle';
    const rows = 5;
    for (let i = 0; i <= rows; i++) {
      const y = pad.t + (ch / rows) * i;
      ctx.beginPath();
      ctx.moveTo(pad.l, y + .5);
      ctx.lineTo(pad.l + cw, y + .5);
      ctx.stroke();
      const pv = pMax - (pMax / rows) * i;
      ctx.fillStyle = '#8CA2B3';
      ctx.textAlign = 'right';
      ctx.fillText(pv.toFixed(0), pad.l - 7, y);
      if (series === 'both') {
        const tv = tMax - ((tMax - tMin) / rows) * i;
        ctx.textAlign = 'left';
        ctx.fillStyle = '#C08A5A';
        ctx.fillText(tv.toFixed(0), pad.l + cw + 7, y);
      }
    }

    // vertical grid
    ctx.strokeStyle = '#F4F8FB';
    for (let i = 1; i < 6; i++) {
      const x = pad.l + (cw / 6) * i;
      ctx.beginPath(); ctx.moveTo(x + .5, pad.t); ctx.lineTo(x + .5, pad.t + ch); ctx.stroke();
    }

    const n = data.p.length;
    const xAt = (i) => pad.l + (n <= 1 ? 0 : (cw * i) / (WINDOW - 1));

    const line = (arr, color, min, max, fill, glow = 'rgba(0,161,233,.16)') => {
      if (!arr.length) return;
      const yAt = (v) => pad.t + ch - ((v - min) / (max - min)) * ch;
      if (fill) {
        const g = ctx.createLinearGradient(0, pad.t, 0, pad.t + ch);
        g.addColorStop(0, fill[0]); g.addColorStop(1, fill[1]);
        ctx.beginPath();
        ctx.moveTo(xAt(0), pad.t + ch);
        arr.forEach((v, i) => ctx.lineTo(xAt(i), yAt(v)));
        ctx.lineTo(xAt(arr.length - 1), pad.t + ch);
        ctx.closePath();
        ctx.fillStyle = g; ctx.fill();
      }
      ctx.beginPath();
      arr.forEach((v, i) => { const x = xAt(i), y = yAt(v); i ? ctx.lineTo(x, y) : ctx.moveTo(x, y); });
      ctx.strokeStyle = color;
      ctx.lineWidth = 2;
      ctx.lineJoin = 'round';
      ctx.lineCap = 'round';
      ctx.stroke();

      // head dot
      const hx = xAt(arr.length - 1), hy = yAt(arr[arr.length - 1]);
      ctx.beginPath(); ctx.arc(hx, hy, 3.4, 0, Math.PI * 2);
      ctx.fillStyle = color; ctx.fill();
      ctx.beginPath(); ctx.arc(hx, hy, 6.5, 0, Math.PI * 2);
      ctx.fillStyle = glow;
      ctx.fill();
    };

    if (series !== 't') line(data.p, '#00A1E9', 0, pMax, ['rgba(0,161,233,.18)', 'rgba(0,161,233,0)']);
    if (series !== 'p') {
      line(data.t, '#C8102E', tMin, tMax, null, 'rgba(200,16,46,.16)');    // 上板温度（右轴）
      if (data.t2) line(data.t2, '#E8730C', tMin, tMax, null, 'rgba(232,115,12,.16)');  // 下板温度（右轴）
    }

    // x labels
    const lx = '-180 s', rx = '现在';
    ctx.fillStyle = '#A9BCCB';
    ctx.textAlign = 'left';
    ctx.fillText(lx, pad.l, pad.t + ch + 12);
    ctx.textAlign = 'right';
    ctx.fillText(rx, pad.l + cw, pad.t + ch + 12);
  }

  const c1 = $('#chart'), c1ctx = c1.getContext('2d');

  // 右轴温度上限：随当前温度自适应，最低 80 °C
  const tempAxisMax = () => {
    const m = Math.max(hist.t[hist.t.length - 1] || 0, hist.t2[hist.t2.length - 1] || 0);
    return Math.max(80, Math.ceil((m * 1.2) / 20) * 20);
  };

  function resizeCharts() {
    const a = setupCanvas(c1);
    if (!a) return;
    charts[0] = a;
    drawChart(c1, a.ctx, a.w, a.h, hist, 'both', { tMax: tempAxisMax() });
  }

  // 观察画布容器本身：卡片尺寸 / 视图切换 / 断点变化都能触发重新适配
  new ResizeObserver(() => resizeCharts()).observe($('.chart-wrap'));
  window.addEventListener('resize', resizeCharts);
  if (document.fonts && document.fonts.ready) document.fonts.ready.then(resizeCharts);

  // Edge 滚动时画布可能出现局部残影，滚动结束后重绘一次
  let scrollTick = 0;
  $('#content').addEventListener('scroll', () => {
    if (scrollTick) return;
    scrollTick = requestAnimationFrame(() => {
      scrollTick = 0;
      const c = charts[0];
      if (c) drawChart(c1, c.ctx, c.w, c.h, hist, 'both', { tMax: tempAxisMax() });
    });
  }, { passive: true });

  // 采样：每秒 2 点
  setInterval(() => {
    hist.p.push(S.press ? S.liveP : 0);
    hist.t.push(Math.max(0, S.upT));
    hist.t2.push(Math.max(0, S.downT));
    if (hist.p.length > WINDOW) { hist.p.shift(); hist.t.shift(); hist.t2.shift(); }
    const c = charts[0];
    if (c) drawChart(c1, c.ctx, c.w, c.h, hist, 'both', { tMax: tempAxisMax() });
  }, 500);

  /* ---------------------------------------------------------
     boot
     --------------------------------------------------------- */
  // 阶段卡片较窄时隐藏“阶段数”文字标签（替代容器查询，规避 Edge 绘制异常）
  const narrowObs = new ResizeObserver((entries) => {
    entries.forEach((e) => e.target.classList.toggle('is-narrow', e.contentRect.width <= 350));
  });
  $$('.stage-card').forEach((el) => narrowObs.observe(el));

  renderStageTables();
  initCountSpins();
  renderCoopTable();
  syncMoldUI();
  applyRateMode();
  paintValues();
  requestAnimationFrame(resizeCharts);

  log('上位机已启动，等待设备连接');

  // 初始曲线：填充最近 180 s 的上一轮完整循环（加压 3 段 + 升温 5 段 + 泄压回落）
  const PV = [1.0, 2.0, 3.0];
  for (let i = 0; i < WINDOW; i++) {
    const k = i / WINDOW;
    const seg = k * 3;
    const st = Math.min(2, Math.floor(seg));
    const prev = st === 0 ? 0 : PV[st - 1];
    let pv = prev + (PV[st] - prev) * Math.min(1, (seg - st) * 8);
    if (k > 0.9) pv *= Math.max(0, 1 - (k - 0.9) / 0.1);
    const tv = k < 0.78 ? 24.5 + (k / 0.78) * 25.5 : 50 - ((k - 0.78) / 0.22) * 25.5;
    hist.p.push(+pv.toFixed(2));
    hist.t.push(+tv.toFixed(1));
    hist.t2.push(+(tv - 2.5).toFixed(1));
  }
  S.liveP = 0;
  setTimeout(resizeCharts, 60);

  /* --- 深链接：#/coop、#/settings ... --- */
  const applyHash = () => {
    const parts = location.hash.replace(/^#\/?/, '').split('/').filter(Boolean);
    if (!parts.length) return;
    if (VIEWS[parts[0]]) go(parts[0]);
  };
  window.addEventListener('hashchange', applyHash);
  applyHash();
})();
