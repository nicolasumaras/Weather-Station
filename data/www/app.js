(() => {
  const $ = (id) => document.getElementById(id);

  const els = {
    heroReading: $("heroReading"),
    heroSub: $("heroSub"),
    statusPill: $("statusPill"),
    ipPill: $("ipPill"),
    timePill: $("timePill"),
    vPm25: $("vPm25"),
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
    canvas: $("chartMain"),
  };

  let history = [];
  let seriesKey = "pm2_5";
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

    els.vPm25.textContent = fmt(d.pm2_5, 0);
    els.vWind.textContent = fmt(w, units.wind === "mps" ? 2 : 1);
    els.vDose.textContent = fmt(d.usv_h, 3);
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
    ctx.fillText("PM2.5 · wind · dose · pressure · temp · humidity (tap to focus)", 12, 18);

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
    els.setupPanel.hidden = !els.setupPanel.hidden;
    if (!els.setupPanel.hidden) loadSettings();
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

  els.canvas.addEventListener("click", () => {
    const keys = ["pm2_5", "wind_mps", "usv_h", "pressure", "temperature", "humidity"];
    seriesKey = keys[(keys.indexOf(seriesKey) + 1) % keys.length];
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
