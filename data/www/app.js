(() => {
  const $ = (id) => document.getElementById(id);

  const els = {
    heroReading: $("heroReading"),
    heroSub: $("heroSub"),
    statusPill: $("statusPill"),
    ipPill: $("ipPill"),
    timePill: $("timePill"),
    vPm1: $("vPm1"),
    vPm25: $("vPm25"),
    vPm10: $("vPm10"),
    qPm1: $("qPm1"),
    qPm25: $("qPm25"),
    qPm10: $("qPm10"),
    qDose: $("qDose"),
    vAqi: $("vAqi"),
    qAqi: $("qAqi"),
    aqiNote: $("aqiNote"),
    vRadIndex: $("vRadIndex"),
    qRad: $("qRad"),
    radNote: $("radNote"),
    vWind: $("vWind"),
    vDose: $("vDose"),
    vPressure: $("vPressure"),
    vTemp: $("vTemp"),
    vHum: $("vHum"),
    uWind: $("uWind"),
    uPressure: $("uPressure"),
    uTemp: $("uTemp"),
    fwLabel: $("fwLabel"),
    uptimeLabel: $("uptimeLabel"),
    chartSub: $("chartSub"),
    setupPanel: $("setupPanel"),
    btnSetup: $("btnSetup"),
    formMsg: $("formMsg"),
    wifiSsidSelect: $("wifiSsidSelect"),
    wifiSsid: $("wifiSsid"),
    wifiPass: $("wifiPass"),
    mqttEnabled: $("mqttEnabled"),
    mqttHost: $("mqttHost"),
    mqttPort: $("mqttPort"),
    mqttUser: $("mqttUser"),
    mqttPass: $("mqttPass"),
    mqttPrefix: $("mqttPrefix"),
    windFactor: $("windFactor"),
    geigerFactor: $("geigerFactor"),
    timezone: $("timezone"),
    timezoneCustom: $("timezoneCustom"),
    windUnit: $("windUnit"),
    tempUnit: $("tempUnit"),
    pressureUnit: $("pressureUnit"),
    mdnsHint: $("mdnsHint"),
    meshEnabled: $("meshEnabled"),
    meshHost: $("meshHost"),
    meshAdminPass: $("meshAdminPass"),
    meshHookToken: $("meshHookToken"),
    meshKeyword: $("meshKeyword"),
    meshHookUrl: $("meshHookUrl"),
    meshStatus: $("meshStatus"),
    meshLog: $("meshLog"),
    btnMeshLog: $("btnMeshLog"),
    btnMeshLogClear: $("btnMeshLogClear"),
    seriesRow: $("seriesRow"),
    canvas: $("chartMain"),
  };

  let history = [];
  let seriesKey = "pm2_5";
  const SERIES = [
    { key: "pm2_5", label: "PM2.5" },
    { key: "usv_h", label: "Dose" },
    { key: "wind_mps", label: "Wind" },
    { key: "temperature", label: "Temp" },
    { key: "humidity", label: "Humidity" },
    { key: "pressure", label: "Pressure" },
  ];
  let units = { wind: "mps", temp: "C", pressure: "hPa" };

  function fmt(v, digits = 1) {
    if (v === null || v === undefined || Number.isNaN(v)) return "—";
    return Number(v).toFixed(digits);
  }

  function windFromMps(mps) {
    if (mps == null || Number.isNaN(mps)) return null;
    if (units.wind === "kmh") return mps * 3.6;
    if (units.wind === "mph") return mps * 2.23694;
    return mps;
  }

  function windLabel() {
    return units.wind === "kmh" ? "km/h" : units.wind === "mph" ? "mph" : "m/s";
  }

  function tempFromC(c) {
    if (c == null || Number.isNaN(c)) return null;
    return units.temp === "F" ? c * 9 / 5 + 32 : c;
  }

  function tempLabel() {
    return units.temp === "F" ? "°F" : "°C";
  }

  function pressureFromHpa(hpa) {
    if (hpa == null || Number.isNaN(hpa)) return null;
    return units.pressure === "inHg" ? hpa * 0.0295299830714 : hpa;
  }

  function pressureLabel() {
    return units.pressure === "inHg" ? "inHg" : "hPa";
  }

  function convertSample(s) {
    return {
      ...s,
      wind_disp: windFromMps(s.wind_mps),
      temperature_disp: tempFromC(s.temperature),
      pressure_disp: pressureFromHpa(s.pressure),
    };
  }

  function applyUnitsFromStatus(d) {
    if (d.wind_unit) units.wind = d.wind_unit;
    if (d.temp_unit) units.temp = d.temp_unit;
    if (d.pressure_unit) units.pressure = d.pressure_unit;
    els.uWind.textContent = windLabel();
    els.uTemp.textContent = tempLabel();
    els.uPressure.textContent = pressureLabel();
  }

  // US EPA AQI breakpoints: [Clow, Chigh, Ilow, Ihigh].
  // PM2.5 uses the 2024 revision; PM10 is unchanged.
  const AQI_PM25 = [
    [0.0, 9.0, 0, 50], [9.1, 35.4, 51, 100], [35.5, 55.4, 101, 150],
    [55.5, 125.4, 151, 200], [125.5, 225.4, 201, 300], [225.5, 325.4, 301, 500],
  ];
  const AQI_PM10 = [
    [0, 54, 0, 50], [55, 154, 51, 100], [155, 254, 101, 150],
    [255, 354, 151, 200], [355, 424, 201, 300], [425, 604, 301, 500],
  ];

  const AQI_BANDS = [
    [50, "good", "Good"],
    [100, "moderate", "Moderate"],
    [150, "sensitive", "Unhealthy for sensitive groups"],
    [200, "unhealthy", "Unhealthy"],
    [300, "very", "Very unhealthy"],
    [Infinity, "hazardous", "Hazardous"],
  ];

  // Typical outdoor gamma background. The radiation index is a multiple of this.
  const BACKGROUND_USV_H = 0.10;
  const RAD_BANDS = [
    [1.5, "good", "Normal background"],
    [3, "moderate", "Slightly elevated"],
    [10, "sensitive", "Elevated"],
    [100, "unhealthy", "High"],
    [Infinity, "hazardous", "Very high"],
  ];

  const AQI_NOTE =
    "US EPA AQI. Official breakpoints are defined on 24-hour averages; " +
    "this is computed from a live reading, so it swings more than a real AQI would.";
  const RAD_NOTE =
    `Multiple of typical outdoor background (${BACKGROUND_USV_H} µSv/h). ` +
    "Short counting windows are statistically noisy — trust the trend, not one sample.";

  function subIndex(c, table) {
    if (c == null || Number.isNaN(c) || c < 0) return null;
    for (const [cLo, cHi, iLo, iHi] of table) {
      if (c <= cHi) return Math.round(((iHi - iLo) / (cHi - cLo)) * (c - cLo) + iLo);
    }
    return 500;
  }

  function band(value, bands) {
    if (value == null || Number.isNaN(value)) return null;
    for (const [max, level, label] of bands) {
      if (value <= max) return { level, label };
    }
    return null;
  }

  function setBadge(el, info, note) {
    if (!info) {
      el.textContent = "—";
      el.dataset.level = "none";
      el.title = "No reading yet";
      return;
    }
    el.textContent = info.label;
    el.dataset.level = info.level;
    el.title = note;
  }

  function applyIndices(d) {
    const pmOk = !!d.pm_valid;
    const i25 = pmOk ? subIndex(d.pm2_5, AQI_PM25) : null;
    const i10 = pmOk ? subIndex(d.pm10, AQI_PM10) : null;

    setBadge(els.qPm25, band(i25, AQI_BANDS), `PM2.5 sub-index ${i25 ?? "—"}. ${AQI_NOTE}`);
    setBadge(els.qPm10, band(i10, AQI_BANDS), `PM10 sub-index ${i10 ?? "—"}. ${AQI_NOTE}`);
    els.qPm1.title = "PM1.0 has no EPA AQI breakpoints; shown as a raw concentration.";

    const aqi = [i25, i10].filter((v) => v != null).length
      ? Math.max(...[i25, i10].filter((v) => v != null))
      : null;
    els.vAqi.textContent = aqi == null ? "—" : String(aqi);
    setBadge(els.qAqi, band(aqi, AQI_BANDS), AQI_NOTE);
    const driver = aqi == null ? null : (i25 != null && i25 >= (i10 ?? -1) ? "PM2.5" : "PM10");
    els.aqiNote.textContent = driver
      ? `US EPA AQI · driven by ${driver}`
      : "US EPA AQI · waiting for PM data";

    const dose = d.geiger_valid ? d.usv_h : null;
    const ratio = dose == null ? null : dose / BACKGROUND_USV_H;
    els.vRadIndex.textContent = ratio == null ? "—" : `${fmt(ratio, 1)}×`;
    setBadge(els.qRad, band(ratio, RAD_BANDS), RAD_NOTE);
    els.radNote.textContent = dose == null
      ? "Waiting for first 10 s counting window"
      : `${fmt(dose, 3)} µSv/h · ≈ ${fmt(dose * 8.766, 2)} mSv/year`;
  }

  function applyStatus(d) {
    applyUnitsFromStatus(d);
    const pm = d.pm_valid ? d.pm2_5 : null;
    els.heroReading.textContent = pm != null ? `${fmt(pm, 0)} µg/m³ PM2.5` : "Live sensors";

    const w = windFromMps(d.wind_mps);
    const t = tempFromC(d.temperature);
    const bits = [];
    if (t != null) bits.push(`${fmt(t, 1)} ${tempLabel()}`);
    if (w != null) bits.push(`${fmt(w, 1)} ${windLabel()} wind`);
    if (d.usv_h != null) bits.push(`${fmt(d.usv_h, 3)} µSv/h`);
    els.heroSub.textContent = bits.length ? bits.join(" · ") : (d.wifi_reason || "Waiting for sensors");

    els.statusPill.textContent = d.mode || "—";
    const mdns = d.mdns || "http://weather-station.local";
    els.ipPill.textContent = d.ip ? `${d.ip}` : (d.portal ? "192.168.4.1" : "—");
    els.ipPill.title = mdns;
    els.timePill.textContent = d.ntp && d.local_time ? d.local_time : (d.ntp ? "NTP ok" : "NTP —");

    els.vPm1.textContent = fmt(d.pm1_0, 0);
    els.vPm25.textContent = fmt(d.pm2_5, 0);
    els.vPm10.textContent = fmt(d.pm10, 0);
    els.vWind.textContent = fmt(w, units.wind === "mps" ? 2 : 1);
    els.vDose.textContent = fmt(d.usv_h, 3);
    setBadge(els.qDose, band(d.geiger_valid ? d.usv_h / BACKGROUND_USV_H : null, RAD_BANDS), RAD_NOTE);
    applyIndices(d);
    els.vPressure.textContent = fmt(pressureFromHpa(d.pressure), units.pressure === "inHg" ? 2 : 1);
    els.vTemp.textContent = fmt(t, 1);
    els.vHum.textContent = fmt(d.humidity, 0);
    els.fwLabel.textContent = `FW ${d.fw || "—"}`;
    const up = d.uptime || 0;
    const h = Math.floor(up / 3600);
    const m = Math.floor((up % 3600) / 60);
    const s = up % 60;
    els.uptimeLabel.textContent = `Uptime ${h}h ${m}m ${s}s · ${mdns.replace("http://", "")}`;

    if (d.portal) {
      els.setupPanel.hidden = false;
    }
  }

  function formatAxisTime(ts, isUnix) {
    if (!isUnix || !ts) return "";
    const d = new Date(ts * 1000);
    return d.toLocaleTimeString([], { hour: "2-digit", minute: "2-digit" });
  }

  function drawChart() {
    const canvas = els.canvas;
    if (!canvas) return;
    const ctx = canvas.getContext("2d");
    const dpr = window.devicePixelRatio || 1;
    const rect = canvas.parentElement.getBoundingClientRect();
    canvas.width = Math.floor(rect.width * dpr);
    canvas.height = Math.floor(rect.height * dpr);
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    const w = rect.width;
    const h = rect.height;
    ctx.clearRect(0, 0, w, h);

    const disp = history.map(convertSample);
    const keys = [
      { key: "pm2_5", color: "#2ec4b6", label: "PM2.5" },
      { key: "wind_disp", color: "#7eb8da", label: "wind" },
      { key: "usv_h", color: "#e0a458", label: "dose" },
      { key: "pressure_disp", color: "#9ad0c2", label: "pressure" },
      { key: "temperature_disp", color: "#f2d0a4", label: "temp" },
      { key: "humidity", color: "#8aa0aa", label: "humidity" },
    ];
    const activeKey = seriesKey === "wind_mps" ? "wind_disp"
      : seriesKey === "pressure" ? "pressure_disp"
      : seriesKey === "temperature" ? "temperature_disp"
      : seriesKey;

    ctx.fillStyle = "rgba(232,241,244,0.55)";
    ctx.font = "12px 'Segoe UI', sans-serif";
    const active = SERIES.find((s) => s.key === seriesKey);
    ctx.fillText(active ? active.label : "", 12, 18);

    const pad = { l: 36, r: 12, t: 28, b: 28 };
    const plotW = w - pad.l - pad.r;
    const plotH = h - pad.t - pad.b;
    const hasUnix = disp.length && disp.some((s) => s.unix);

    keys.forEach((series, ki) => {
      const pts = disp
        .map((s, i) => ({ i, v: s[series.key], t: s.t, unix: s.unix }))
        .filter((p) => p.v != null && !Number.isNaN(p.v));
      if (pts.length < 2) return;
      const vals = pts.map((p) => p.v);
      let min = Math.min(...vals);
      let max = Math.max(...vals);
      if (min === max) {
        min -= 1;
        max += 1;
      }
      ctx.beginPath();
      ctx.strokeStyle = series.color;
      ctx.globalAlpha = series.key === activeKey ? 1 : 0.35;
      ctx.lineWidth = series.key === activeKey ? 2.2 : 1.2;
      pts.forEach((p, idx) => {
        const x = pad.l + (p.i / Math.max(1, disp.length - 1)) * plotW;
        const y = pad.t + (1 - (p.v - min) / (max - min)) * plotH;
        if (idx === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
      });
      ctx.stroke();
      ctx.globalAlpha = 1;
      if (ki === 0) {
        ctx.fillStyle = "rgba(232,241,244,0.45)";
        ctx.fillText(String(max.toFixed(1)), 4, pad.t + 8);
        ctx.fillText(String(min.toFixed(1)), 4, pad.t + plotH);
      }
    });

    if (hasUnix && disp.length > 1) {
      ctx.fillStyle = "rgba(232,241,244,0.45)";
      ctx.fillText(formatAxisTime(disp[0].t, disp[0].unix), pad.l, h - 8);
      ctx.fillText(formatAxisTime(disp[disp.length - 1].t, disp[disp.length - 1].unix), w - 64, h - 8);
    }
  }

  async function loadHistory() {
    try {
      const res = await fetch("/api/history");
      const data = await res.json();
      history = data.samples || [];
      const unix = data.unix || (history[0] && history[0].unix);
      els.chartSub.textContent = unix
        ? "On-device history with NTP timestamps (15 s samples)"
        : "On-device history (relative time until NTP syncs)";
      drawChart();
    } catch (e) {
      console.warn(e);
    }
  }

  function selectedTimezone() {
    const custom = els.timezoneCustom.value.trim();
    return custom || els.timezone.value || "<-03>3";
  }

  function logTime(ev) {
    if (ev.ts) {
      return new Date(ev.ts * 1000).toLocaleTimeString();
    }
    // No NTP yet — fall back to uptime, which is still enough to order events.
    const h = Math.floor(ev.up / 3600);
    const m = Math.floor((ev.up % 3600) / 60);
    const s = ev.up % 60;
    return `+${h}:${String(m).padStart(2, "0")}:${String(s).padStart(2, "0")}`;
  }

  async function loadMeshLog() {
    const body = els.meshLog.tBodies[0];
    body.replaceChildren();
    let data;
    try {
      const res = await fetch("/api/mesh-log");
      data = await res.json();
    } catch (e) {
      const tr = body.insertRow();
      tr.insertCell().outerHTML = `<td class="empty" colspan="4">Could not read log: ${e}</td>`;
      return;
    }

    const events = data.events || [];
    if (!events.length) {
      const tr = body.insertRow();
      tr.insertCell().outerHTML = '<td class="empty" colspan="4">No exchanges recorded yet</td>';
      return;
    }

    for (const ev of events) {
      const tr = body.insertRow();
      const t = tr.insertCell();
      t.textContent = logTime(ev);

      const d = tr.insertCell();
      d.textContent = ev.dir === "<" ? "in" : "out";
      d.className = ev.dir === "<" ? "dir-in" : "dir-out";

      const w = tr.insertCell();
      w.textContent = ev.code ? `${ev.what} ${ev.code}` : ev.what;
      if (ev.code && (ev.code < 200 || ev.code >= 300)) {
        w.className = "code-bad";
      }

      const det = tr.insertCell();
      det.className = "detail";
      det.textContent = ev.detail || "";
    }

    if (data.dropped) {
      const tr = body.insertRow();
      tr.insertCell().outerHTML =
        `<td class="empty" colspan="4">${data.dropped} older entries dropped</td>`;
    }
  }

  async function loadSettings() {
    const res = await fetch("/api/settings");
    const s = await res.json();
    els.wifiSsid.value = s.wifi_ssid || "";
    els.mqttEnabled.checked = !!s.mqtt_enabled;
    els.mqttHost.value = s.mqtt_host || "";
    els.mqttPort.value = s.mqtt_port || 1883;
    els.mqttUser.value = s.mqtt_user || "";
    els.mqttPrefix.value = s.mqtt_prefix || "weather_station";
    els.windFactor.value = s.wind_factor ?? 0.0875;
    els.geigerFactor.value = s.geiger_factor ?? 0.0057;
    els.windUnit.value = s.wind_unit || "mps";
    els.tempUnit.value = s.temp_unit || "C";
    els.pressureUnit.value = s.pressure_unit || "hPa";
    const tz = s.timezone || "<-03>3";
    const opt = Array.from(els.timezone.options).find((o) => o.value === tz);
    if (opt) {
      els.timezone.value = tz;
      els.timezoneCustom.value = "";
    } else {
      els.timezoneCustom.value = tz;
    }
    els.meshEnabled.checked = !!s.mesh_enabled;
    els.meshHost.value = s.mesh_host || "";
    els.meshKeyword.value = s.mesh_keyword || "weather";
    // Secrets are never sent back; blank means "leave stored value alone".
    els.meshAdminPass.value = "";
    els.meshHookToken.value = "";
    els.meshAdminPass.placeholder = s.has_mesh_admin_pass
      ? "stored — leave blank to keep"
      : "required";
    els.meshHookToken.placeholder = s.has_mesh_hook_token
      ? `stored, ${s.mesh_token_len ?? "?"} chars — leave blank to keep`
      : "required";
    // Use the origin the browser actually reached us on, not the mDNS name:
    // the gateway delivers with HTTPClient, which cannot resolve .local.
    els.meshHookUrl.textContent = `${location.origin}/api/mesh-hook`;

    const meshBits = [`${s.mesh_hooks_ok ?? 0} received`];
    if (s.mesh_hooks_rejected) meshBits.push(`${s.mesh_hooks_rejected} rejected`);
    meshBits.push(`${s.mesh_sent ?? 0} replied`);
    if (s.mesh_failed) meshBits.push(`${s.mesh_failed} failed`);
    if (s.mesh_last_error) meshBits.push(s.mesh_last_error);
    els.meshStatus.textContent = meshBits.join(" · ");
    loadMeshLog();

    if (s.mdns) els.mdnsHint.textContent = `Dashboard: ${s.mdns}`;
    units = {
      wind: s.wind_unit || "mps",
      temp: s.temp_unit || "C",
      pressure: s.pressure_unit || "hPa",
    };
    els.uWind.textContent = windLabel();
    els.uTemp.textContent = tempLabel();
    els.uPressure.textContent = pressureLabel();
  }

  async function scanWifi() {
    els.formMsg.textContent = "Scanning…";
    const res = await fetch("/api/scan");
    const data = await res.json();
    const nets = data.networks || [];
    els.wifiSsidSelect.innerHTML = nets
      .map((n) => `<option value="${n.ssid.replace(/"/g, "&quot;")}">${n.ssid} (${n.rssi} dBm)</option>`)
      .join("");
    if (nets[0]) els.wifiSsid.value = nets[0].ssid;
    els.formMsg.textContent = `${nets.length} networks found`;
  }

  async function saveSettings(applyWifi) {
    const body = {
      wifi_ssid: els.wifiSsid.value.trim(),
      wifi_pass: els.wifiPass.value,
      mqtt_enabled: els.mqttEnabled.checked,
      mqtt_host: els.mqttHost.value.trim(),
      mqtt_port: Number(els.mqttPort.value) || 1883,
      mqtt_user: els.mqttUser.value.trim(),
      mqtt_pass: els.mqttPass.value,
      mqtt_prefix: els.mqttPrefix.value.trim() || "weather_station",
      wind_factor: Number(els.windFactor.value),
      geiger_factor: Number(els.geigerFactor.value),
      timezone: selectedTimezone(),
      wind_unit: els.windUnit.value,
      temp_unit: els.tempUnit.value,
      pressure_unit: els.pressureUnit.value,
      mesh_enabled: els.meshEnabled.checked,
      mesh_host: els.meshHost.value.trim(),
      mesh_admin_pass: els.meshAdminPass.value,
      mesh_hook_token: els.meshHookToken.value,
      mesh_keyword: els.meshKeyword.value.trim() || "weather",
      apply_wifi: !!applyWifi,
    };
    const res = await fetch("/api/settings", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(body),
    });
    const data = await res.json();
    units = {
      wind: body.wind_unit,
      temp: body.temp_unit,
      pressure: body.pressure_unit,
    };
    els.uWind.textContent = windLabel();
    els.uTemp.textContent = tempLabel();
    els.uPressure.textContent = pressureLabel();
    drawChart();
    els.formMsg.textContent = data.ok
      ? applyWifi
        ? "Saved — connecting to WiFi…"
        : "Saved"
      : data.error || "Save failed";
  }

  function connectWs() {
    const proto = location.protocol === "https:" ? "wss" : "ws";
    const ws = new WebSocket(`${proto}://${location.host}/ws`);
    ws.onmessage = (ev) => {
      try {
        applyStatus(JSON.parse(ev.data));
      } catch (e) {}
    };
    ws.onclose = () => setTimeout(connectWs, 2000);
  }

  els.btnSetup.addEventListener("click", () => {
    const opening = els.setupPanel.hidden;
    els.setupPanel.hidden = !opening;
    els.btnSetup.setAttribute("aria-expanded", String(opening));
    els.btnSetup.textContent = opening ? "Close setup" : "Setup";
    if (opening) {
      loadSettings();
      // On a phone the panel opens well below the fold — take the user there.
      els.setupPanel.scrollIntoView({ behavior: "smooth", block: "start" });
    } else {
      window.scrollTo({ top: 0, behavior: "smooth" });
    }
  });
  els.btnMeshLog.addEventListener("click", () => loadMeshLog());
  els.btnMeshLogClear.addEventListener("click", async () => {
    await fetch("/api/mesh-log", { method: "DELETE" }).catch(() => {});
    loadMeshLog();
  });

  els.wifiSsidSelect.addEventListener("change", () => {
    els.wifiSsid.value = els.wifiSsidSelect.value;
  });
  $("btnScan").addEventListener("click", () => scanWifi().catch((e) => {
    els.formMsg.textContent = String(e);
  }));
  $("setupForm").addEventListener("submit", (e) => {
    e.preventDefault();
    saveSettings(false).catch((err) => {
      els.formMsg.textContent = String(err);
    });
  });
  $("btnSaveWifi").addEventListener("click", () => {
    saveSettings(true).catch((err) => {
      els.formMsg.textContent = String(err);
    });
  });
  $("btnForget").addEventListener("click", async () => {
    if (!confirm("Forget WiFi and reboot into setup mode?")) return;
    await fetch("/api/forget-wifi", { method: "POST" });
    els.formMsg.textContent = "Rebooting…";
  });

  $("btnOta").addEventListener("click", () => {
    const fileInput = $("otaFile");
    const msg = $("otaMsg");
    const bar = $("otaProgress");
    const file = fileInput.files && fileInput.files[0];
    if (!file) {
      msg.textContent = "Choose a .bin file first";
      return;
    }
    if (!confirm(`Flash ${file.name}? The station will reboot.`)) return;

    const mode = $("otaMode").value === "fs" ? "fs" : "firmware";
    const xhr = new XMLHttpRequest();
    xhr.open("POST", `/api/ota?mode=${mode === "fs" ? "fs" : "fw"}`);
    bar.hidden = false;
    bar.value = 0;
    msg.textContent = "Uploading…";
    xhr.upload.onprogress = (ev) => {
      if (ev.lengthComputable) {
        bar.value = Math.round((ev.loaded / ev.total) * 100);
      }
    };
    xhr.onload = () => {
      try {
        const data = JSON.parse(xhr.responseText || "{}");
        msg.textContent = data.ok ? "Update OK — rebooting…" : data.error || "Update failed";
      } catch (e) {
        msg.textContent = xhr.status === 200 ? "Update finished — rebooting…" : "Update failed";
      }
    };
    xhr.onerror = () => {
      msg.textContent = "Upload error (device may still reboot if flash succeeded)";
    };
    const form = new FormData();
    form.append("firmware", file, file.name);
    xhr.send(form);
  });

  function syncSeriesChips() {
    for (const btn of els.seriesRow.children) {
      btn.setAttribute("aria-pressed", String(btn.dataset.key === seriesKey));
    }
  }

  function buildSeriesChips() {
    els.seriesRow.replaceChildren();
    for (const s of SERIES) {
      const btn = document.createElement("button");
      btn.type = "button";
      btn.className = "chip";
      btn.dataset.key = s.key;
      btn.textContent = s.label;
      btn.setAttribute("aria-pressed", String(s.key === seriesKey));
      btn.addEventListener("click", () => {
        seriesKey = s.key;
        syncSeriesChips();
        drawChart();
      });
      els.seriesRow.appendChild(btn);
    }
  }

  buildSeriesChips();

  // Tapping the chart still cycles, for anyone used to the old behaviour.
  els.canvas.addEventListener("click", () => {
    const i = SERIES.findIndex((s) => s.key === seriesKey);
    seriesKey = SERIES[(i + 1) % SERIES.length].key;
    syncSeriesChips();
    drawChart();
  });
  window.addEventListener("resize", drawChart);

  fetch("/api/status")
    .then((r) => r.json())
    .then(applyStatus)
    .catch(() => {});
  loadHistory();
  loadSettings().catch(() => {});
  connectWs();
  setInterval(loadHistory, 30000);
})();
