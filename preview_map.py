#!/usr/bin/env python3
"""
SatisTools - Stage 1 Interactive Map Tester & Preview Server
Zero-dependency test environment replicating the in-game C++ Map Subsystem.
Runs locally and automatically opens in your default web browser.
"""

import http.server
import socketserver
import webbrowser
import threading
import json
import os
import sys
import math

PORT = 7890

# Satisfactory World Coordinate Bounds (UE cm)
WORLD_MIN_X = -324698.0
WORLD_MAX_X = 425301.0
WORLD_MIN_Y = -375000.0
WORLD_MAX_Y = 375000.0

HTML_PAGE = """<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>SatisTools - Интерактивная карта (Этап 1)</title>
<style>
  * { box-sizing: border-box; margin: 0; padding: 0; user-select: none; }
  body {
    background-color: #0e1117;
    color: #e0e6ed;
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
    display: flex;
    height: 100vh;
    overflow: hidden;
  }
  #sidebar {
    width: 380px;
    background: #161b22;
    border-right: 1px solid #30363d;
    display: flex;
    flex-direction: column;
    z-index: 10;
    box-shadow: 2px 0 12px rgba(0,0,0,0.4);
  }
  #header {
    padding: 16px;
    background: #1f242c;
    border-bottom: 1px solid #30363d;
  }
  #header h1 {
    font-size: 18px;
    font-weight: 700;
    color: #fa5c2c;
    display: flex;
    align-items: center;
    gap: 8px;
  }
  #header p {
    font-size: 12px;
    color: #8b949e;
    margin-top: 4px;
  }
  .panel-section {
    padding: 14px 16px;
    border-bottom: 1px solid #21262d;
  }
  .panel-title {
    font-size: 13px;
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    color: #8b949e;
    margin-bottom: 10px;
  }
  .tool-btn-group {
    display: flex;
    gap: 8px;
  }
  .tool-btn {
    flex: 1;
    background: #21262d;
    border: 1px solid #30363d;
    color: #c9d1d9;
    padding: 8px;
    border-radius: 6px;
    font-size: 13px;
    cursor: pointer;
    font-weight: 500;
    transition: all 0.15s;
    text-align: center;
  }
  .tool-btn:hover { background: #30363d; }
  .tool-btn.active {
    background: #fa5c2c;
    border-color: #fa5c2c;
    color: #fff;
    font-weight: 600;
  }
  .layer-checkboxes {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 8px;
  }
  .layer-item {
    display: flex;
    align-items: center;
    gap: 8px;
    font-size: 13px;
    cursor: pointer;
  }
  .layer-item input { accent-color: #fa5c2c; cursor: pointer; }
  .analytics-container {
    flex: 1;
    overflow-y: auto;
    padding: 16px;
  }
  .empty-hint {
    color: #8b949e;
    font-size: 13px;
    text-align: center;
    padding: 24px 8px;
    line-height: 1.5;
  }
  .stat-card {
    background: #21262d;
    border-radius: 6px;
    padding: 12px;
    margin-bottom: 12px;
    border: 1px solid #30363d;
  }
  .stat-row {
    display: flex;
    justify-content: space-between;
    margin-bottom: 6px;
    font-size: 13px;
  }
  .stat-row:last-child { margin-bottom: 0; }
  .stat-val { font-weight: 600; color: #58a6ff; }
  .rate-list {
    margin-top: 8px;
    font-size: 12px;
  }
  .rate-item {
    display: flex;
    justify-content: space-between;
    padding: 3px 0;
    border-bottom: 1px solid #282e38;
  }
  .rate-pos { color: #3fb950; font-weight: 600; }
  .rate-neg { color: #f85149; font-weight: 600; }
  .zone-actions {
    margin-top: 12px;
    display: flex;
    gap: 8px;
  }
  .save-zone-btn {
    flex: 1;
    background: #238636;
    border: none;
    color: white;
    padding: 8px;
    border-radius: 6px;
    font-size: 13px;
    cursor: pointer;
    font-weight: 600;
  }
  .save-zone-btn:hover { background: #2ea043; }
  #map-container {
    flex: 1;
    position: relative;
    background: #090c10;
    overflow: hidden;
    cursor: crosshair;
  }
  #map-canvas {
    width: 100%;
    height: 100%;
    display: block;
  }
  #coords-overlay {
    position: absolute;
    bottom: 12px;
    right: 12px;
    background: rgba(22, 27, 34, 0.85);
    border: 1px solid #30363d;
    border-radius: 6px;
    padding: 6px 12px;
    font-family: monospace;
    font-size: 12px;
    color: #8b949e;
    backdrop-filter: blur(4px);
    pointer-events: none;
  }
  #hint-overlay {
    position: absolute;
    top: 12px;
    right: 12px;
    background: rgba(22, 27, 34, 0.85);
    border: 1px solid #30363d;
    border-radius: 6px;
    padding: 8px 14px;
    font-size: 12px;
    color: #c9d1d9;
    backdrop-filter: blur(4px);
    pointer-events: none;
  }
  .saved-zones-list {
    margin-top: 8px;
    max-height: 120px;
    overflow-y: auto;
  }
  .saved-zone-badge {
    display: flex;
    justify-content: space-between;
    align-items: center;
    background: #21262d;
    padding: 6px 10px;
    border-radius: 4px;
    font-size: 12px;
    margin-bottom: 4px;
    cursor: pointer;
    border-left: 3px solid #58a6ff;
  }
  .saved-zone-badge:hover { background: #30363d; }
</style>
</head>
<body>

<div id="sidebar">
  <div id="header">
    <h1><span>⚙️</span> SatisTools: Карта</h1>
    <p>Модуль интерактивной аналитики (Satisfactory 1.2)</p>
  </div>

  <div class="panel-section">
    <div class="panel-title">Режим работы</div>
    <div class="tool-btn-group">
      <button class="tool-btn active" id="btn-pan" onclick="setMode('pan')">✋ Обзор (Pan)</button>
      <button class="tool-btn" id="btn-box" onclick="setMode('box')">⛶ Рамка (Box)</button>
      <button class="tool-btn" id="btn-lasso" onclick="setMode('lasso')">✎ Лассо (Lasso)</button>
    </div>
  </div>

  <div class="panel-section">
    <div class="panel-title">Слои карты</div>
    <div class="layer-checkboxes">
      <label class="layer-item"><input type="checkbox" id="layer-nodes" checked onchange="redraw()"> Месторождения</label>
      <label class="layer-item"><input type="checkbox" id="layer-pods" checked onchange="redraw()"> Диски (Pods)</label>
      <label class="layer-item"><input type="checkbox" id="layer-factories" checked onchange="redraw()"> Заводы / Фабрики</label>
      <label class="layer-item"><input type="checkbox" id="layer-conveyors" checked onchange="redraw()"> Конвейеры (Mk.1-6)</label>
      <label class="layer-item"><input type="checkbox" id="layer-pipes" checked onchange="redraw()"> Трубы (Pipes)</label>
      <label class="layer-item"><input type="checkbox" id="layer-zones" checked onchange="redraw()"> Зоны игрока</label>
    </div>
  </div>

  <div class="panel-section">
    <div class="panel-title">Сохраненные зоны</div>
    <div class="saved-zones-list" id="saved-zones-container">
      <div style="font-size: 12px; color: #8b949e;">Нет сохраненных зон</div>
    </div>
  </div>

  <div class="analytics-container" id="analytics-panel">
    <div class="empty-hint">
      Выделите область рамкой или лассо на карте, чтобы рассчитать суммарный баланс ресурсов (I/O), энергопотребление и КПД фабрики.
    </div>
  </div>
</div>

<div id="map-container">
  <canvas id="map-canvas"></canvas>
  <div id="hint-overlay">ЛКМ / ПКМ: Перемещение • Колесо мыши: Зум • Инструмент: Рамка / Лассо</div>
  <div id="coords-overlay">X: 0 | Y: 0 (Unreal Units)</div>
</div>

<script>
const WORLD_MIN_X = -324698.0;
const WORLD_MAX_X = 425301.0;
const WORLD_MIN_Y = -375000.0;
const WORLD_MAX_Y = 375000.0;

let currentMode = 'pan';
let zoom = 1.0;
let panX = 0;
let panY = 0;
let isDragging = false;
let startX, startY;
let boxStart = null, boxEnd = null;
let lassoPoints = [];
let savedZones = [];
let currentAnalytics = null;

const canvas = document.getElementById('map-canvas');
const ctx = canvas.getContext('2d');
const container = document.getElementById('map-container');
const coordsOverlay = document.getElementById('coords-overlay');

// Игровые данные мира (симулированный снимок реального мира Satisfactory)
let worldData = {
  nodes: [],
  pods: [],
  buildings: [],
  conveyors: [],
  pipes: []
};

// Загрузка данных мира
fetch('/api/world-data')
  .then(res => res.json())
  .then(data => {
    worldData = data;
    resetView();
    redraw();
  });

function resizeCanvas() {
  canvas.width = container.clientWidth;
  canvas.height = container.clientHeight;
  redraw();
}
window.addEventListener('resize', resizeCanvas);

function worldToScreen(wx, wy) {
  const u = (wx - WORLD_MIN_X) / (WORLD_MAX_X - WORLD_MIN_X);
  const v = (wy - WORLD_MIN_Y) / (WORLD_MAX_Y - WORLD_MIN_Y);
  const size = Math.min(canvas.width, canvas.height) * 0.95;
  const sx = (canvas.width - size) / 2 + (u * size * zoom) + panX;
  const sy = (canvas.height - size) / 2 + (v * size * zoom) + panY;
  return [sx, sy];
}

function screenToWorld(sx, sy) {
  const size = Math.min(canvas.width, canvas.height) * 0.95;
  const u = (sx - (canvas.width - size) / 2 - panX) / (size * zoom);
  const v = (sy - (canvas.height - size) / 2 - panY) / (size * zoom);
  const wx = WORLD_MIN_X + (u * (WORLD_MAX_X - WORLD_MIN_X));
  const wy = WORLD_MIN_Y + (v * (WORLD_MAX_Y - WORLD_MIN_Y));
  return [wx, wy];
}

function resetView() {
  zoom = 1.2;
  panX = -canvas.width * 0.1;
  panY = -canvas.height * 0.1;
}

function setMode(mode) {
  currentMode = mode;
  document.querySelectorAll('.tool-btn').forEach(btn => btn.classList.remove('active'));
  document.getElementById('btn-' + mode).classList.add('active');
  boxStart = boxEnd = null;
  lassoPoints = [];
  redraw();
}

// Mouse events
canvas.addEventListener('mousedown', (e) => {
  const rect = canvas.getBoundingClientRect();
  const mx = e.clientX - rect.left;
  const my = e.clientY - rect.top;

  if (e.button === 1 || e.button === 2 || currentMode === 'pan') {
    isDragging = true;
    startX = mx - panX;
    startY = my - panY;
  } else if (e.button === 0) {
    if (currentMode === 'box') {
      isDragging = true;
      boxStart = [mx, my];
      boxEnd = [mx, my];
    } else if (currentMode === 'lasso') {
      isDragging = true;
      lassoPoints = [[mx, my]];
    }
  }
});

canvas.addEventListener('mousemove', (e) => {
  const rect = canvas.getBoundingClientRect();
  const mx = e.clientX - rect.left;
  const my = e.clientY - rect.top;

  const [wx, wy] = screenToWorld(mx, my);
  coordsOverlay.innerText = `X: ${Math.round(wx)} | Y: ${Math.round(wy)} UU`;

  if (isDragging) {
    if (e.buttons === 2 || e.buttons === 4 || currentMode === 'pan') {
      panX = mx - startX;
      panY = my - startY;
      redraw();
    } else if (currentMode === 'box') {
      boxEnd = [mx, my];
      redraw();
    } else if (currentMode === 'lasso') {
      const last = lassoPoints[lassoPoints.length - 1];
      if (!last || Math.hypot(mx - last[0], my - last[1]) > 5) {
        lassoPoints.push([mx, my]);
        redraw();
      }
    }
  }
});

window.addEventListener('mouseup', (e) => {
  if (!isDragging) return;
  isDragging = false;

  if (currentMode === 'box' && boxStart && boxEnd) {
    const [w1x, w1y] = screenToWorld(Math.min(boxStart[0], boxEnd[0]), Math.min(boxStart[1], boxEnd[1]));
    const [w2x, w2y] = screenToWorld(Math.max(boxStart[0], boxEnd[0]), Math.max(boxStart[1], boxEnd[1]));
    performAnalytics({ type: 'box', minX: w1x, maxX: w2x, minY: w1y, maxY: w2y });
  } else if (currentMode === 'lasso' && lassoPoints.length >= 3) {
    const worldPoly = lassoPoints.map(p => screenToWorld(p[0], p[1]));
    performAnalytics({ type: 'polygon', points: worldPoly });
  }
  redraw();
});

canvas.addEventListener('wheel', (e) => {
  e.preventDefault();
  const rect = canvas.getBoundingClientRect();
  const mx = e.clientX - rect.left;
  const my = e.clientY - rect.top;

  const oldZoom = zoom;
  const delta = e.deltaY < 0 ? 1.15 : 0.87;
  zoom = Math.max(0.3, Math.min(25.0, zoom * delta));

  panX = mx - (mx - panX) * (zoom / oldZoom);
  panY = my - (my - panY) * (zoom / oldZoom);
  redraw();
});

// Ray-casting Point-In-Polygon
function isInsidePoly(pt, poly) {
  let inside = false;
  for (let i = 0, j = poly.length - 1; i < poly.length; j = i++) {
    const vi = poly[i], vj = poly[j];
    if (((vi[1] > pt[1]) !== (vj[1] > pt[1])) &&
        (pt[0] < (vj[0] - vi[0]) * (pt[1] - vi[1]) / (vj[1] - vi[1]) + vi[0])) {
      inside = !inside;
    }
  }
  return inside;
}

function performAnalytics(query) {
  let totalBuildings = 0;
  let totalPower = 0.0;
  let prod = {};
  let cons = {};
  let producingCount = 0;

  worldData.buildings.forEach(b => {
    let inside = false;
    if (query.type === 'box') {
      inside = (b.x >= query.minX && b.x <= query.maxX && b.y >= query.minY && b.y <= query.maxY);
    } else {
      inside = isInsidePoly([b.x, b.y], query.points);
    }

    if (inside) {
      totalBuildings++;
      totalPower += b.power;
      if (b.producing) producingCount++;

      for (const [item, rate] of Object.entries(b.inputs || {})) {
        cons[item] = (cons[item] || 0) + rate;
      }
      for (const [item, rate] of Object.entries(b.outputs || {})) {
        prod[item] = (prod[item] || 0) + rate;
      }
    }
  });

  const eff = totalBuildings > 0 ? (producingCount / totalBuildings * 100) : 0;
  const net = {};
  const allItems = new Set([...Object.keys(prod), ...Object.keys(cons)]);
  allItems.forEach(item => {
    net[item] = (prod[item] || 0) - (cons[item] || 0);
  });

  currentAnalytics = {
    totalBuildings,
    totalPower,
    efficiency: eff,
    prod,
    cons,
    net,
    lastQuery: query
  };

  renderAnalyticsPanel();
}

function renderAnalyticsPanel() {
  const panel = document.getElementById('analytics-panel');
  if (!currentAnalytics || currentAnalytics.totalBuildings === 0) {
    panel.innerHTML = `<div class="empty-hint">В выбранной зоне нет построек игрока. Попробуйте выделить область с фабриками.</div>`;
    return;
  }

  let netRows = '';
  for (const [item, rate] of Object.entries(currentAnalytics.net)) {
    const isPos = rate >= 0;
    const sign = isPos ? '+' : '';
    const cls = isPos ? 'rate-pos' : 'rate-neg';
    netRows += `
      <div class="rate-item">
        <span>${item}</span>
        <span class="${cls}">${sign}${rate.toFixed(1)} /мин</span>
      </div>
    `;
  }

  panel.innerHTML = `
    <div class="stat-card">
      <div class="stat-row"><span>Зданий в зоне:</span><span class="stat-val">${currentAnalytics.totalBuildings}</span></div>
      <div class="stat-row"><span>Энергопотребление:</span><span class="stat-val">${currentAnalytics.totalPower.toFixed(1)} МВт</span></div>
      <div class="stat-row"><span>Эффективность фабрики:</span><span class="stat-val">${currentAnalytics.efficiency.toFixed(1)}%</span></div>
    </div>

    <div class="stat-card">
      <div class="panel-title" style="margin-bottom: 6px;">Баланс предметов (Net Rate)</div>
      <div class="rate-list">
        ${netRows || '<div style="color:#8b949e;">Нет активного производства</div>'}
      </div>
    </div>

    <div class="zone-actions">
      <button class="save-zone-btn" onclick="saveCurrentZone()">📌 Закрепить и назвать зону</button>
    </div>
  `;
}

function saveCurrentZone() {
  if (!currentAnalytics || !currentAnalytics.lastQuery) return;
  const name = prompt("Введите имя зоны:", "Фабрика #" + (savedZones.length + 1));
  if (!name) return;

  const colors = ["#58a6ff", "#3fb950", "#d29922", "#f85149", "#bc8cff"];
  const color = colors[savedZones.length % colors.length];

  savedZones.push({
    name,
    color,
    query: currentAnalytics.lastQuery
  });

  updateSavedZonesList();
  redraw();
}

function updateSavedZonesList() {
  const c = document.getElementById('saved-zones-container');
  if (savedZones.length === 0) {
    c.innerHTML = `<div style="font-size: 12px; color: #8b949e;">Нет сохраненных зон</div>`;
    return;
  }
  c.innerHTML = savedZones.map((z, idx) => `
    <div class="saved-zone-badge" style="border-left-color: ${z.color}" onclick="focusZone(${idx})">
      <span style="font-weight: 500;">${z.name}</span>
      <span style="font-size: 10px; color: #8b949e;">Перейти</span>
    </div>
  `).join('');
}

function focusZone(idx) {
  const z = savedZones[idx];
  if (!z) return;
  performAnalytics(z.query);
  redraw();
}

function redraw() {
  ctx.clearRect(0, 0, canvas.width, canvas.height);

  // Background map grid
  ctx.fillStyle = '#0d1117';
  ctx.fillRect(0, 0, canvas.width, canvas.height);

  // Draw Grid Lines
  ctx.strokeStyle = '#161b22';
  ctx.lineWidth = 1;
  const gridSize = 50 * zoom;
  for (let x = (panX % gridSize); x < canvas.width; x += gridSize) {
    ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, canvas.height); ctx.stroke();
  }
  for (let y = (panY % gridSize); y < canvas.height; y += gridSize) {
    ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(canvas.width, y); ctx.stroke();
  }

  // Draw World Boundaries outline
  const [bMinX, bMinY] = worldToScreen(WORLD_MIN_X, WORLD_MIN_Y);
  const [bMaxX, bMaxY] = worldToScreen(WORLD_MAX_X, WORLD_MAX_Y);
  ctx.strokeStyle = '#30363d';
  ctx.strokeRect(bMinX, bMinY, bMaxX - bMinX, bMaxY - bMinY);

  // 1. Conveyors
  if (document.getElementById('layer-conveyors').checked) {
    ctx.lineWidth = Math.max(1.5, 2 * (zoom / 2));
    worldData.conveyors.forEach(c => {
      ctx.strokeStyle = c.color || '#388bfd';
      ctx.beginPath();
      c.points.forEach((pt, i) => {
        const [sx, sy] = worldToScreen(pt[0], pt[1]);
        if (i === 0) ctx.moveTo(sx, sy);
        else ctx.lineTo(sx, sy);
      });
      ctx.stroke();
    });
  }

  // 2. Pipes
  if (document.getElementById('layer-pipes').checked) {
    ctx.lineWidth = Math.max(2.0, 2.5 * (zoom / 2));
    ctx.strokeStyle = '#f0883e';
    worldData.pipes.forEach(p => {
      ctx.beginPath();
      p.points.forEach((pt, i) => {
        const [sx, sy] = worldToScreen(pt[0], pt[1]);
        if (i === 0) ctx.moveTo(sx, sy);
        else ctx.lineTo(sx, sy);
      });
      ctx.stroke();
    });
  }

  // 3. Buildings / Factories
  if (document.getElementById('layer-factories').checked) {
    const bSize = Math.max(4, 7 * zoom);
    worldData.buildings.forEach(b => {
      const [sx, sy] = worldToScreen(b.x, b.y);
      ctx.fillStyle = b.producing ? '#3fb950' : (b.hasPower ? '#d29922' : '#f85149');
      ctx.fillRect(sx - bSize/2, sy - bSize/2, bSize, bSize);
    });
  }

  // 4. Resource Nodes
  if (document.getElementById('layer-nodes').checked) {
    const nRadius = Math.max(3, 5 * zoom);
    worldData.nodes.forEach(n => {
      const [sx, sy] = worldToScreen(n.x, n.y);
      ctx.fillStyle = n.purity === 'Pure' ? '#d2a8ff' : (n.purity === 'Normal' ? '#58a6ff' : '#8b949e');
      ctx.beginPath();
      ctx.arc(sx, sy, nRadius, 0, Math.PI * 2);
      ctx.fill();
    });
  }

  // 5. Drop Pods (Crash sites)
  if (document.getElementById('layer-pods').checked) {
    const pSize = Math.max(4, 6 * zoom);
    worldData.pods.forEach(p => {
      const [sx, sy] = worldToScreen(p.x, p.y);
      ctx.fillStyle = p.opened ? '#484f58' : '#e3b341';
      ctx.save();
      ctx.translate(sx, sy);
      ctx.rotate(Math.PI / 4);
      ctx.fillRect(-pSize/2, -pSize/2, pSize, pSize);
      ctx.restore();
    });
  }

  // 6. Saved Zones
  if (document.getElementById('layer-zones').checked) {
    savedZones.forEach(z => {
      ctx.strokeStyle = z.color;
      ctx.lineWidth = 2.5;
      if (z.query.type === 'box') {
        const [s1x, s1y] = worldToScreen(z.query.minX, z.query.minY);
        const [s2x, s2y] = worldToScreen(z.query.maxX, z.query.maxY);
        ctx.strokeRect(s1x, s1y, s2x - s1x, s2y - s1y);
        ctx.fillStyle = z.color;
        ctx.font = '12px sans-serif';
        ctx.fillText(z.name, s1x + 4, s1y - 6);
      } else if (z.query.type === 'polygon') {
        ctx.beginPath();
        z.query.points.forEach((pt, i) => {
          const [sx, sy] = worldToScreen(pt[0], pt[1]);
          if (i === 0) ctx.moveTo(sx, sy);
          else ctx.lineTo(sx, sy);
        });
        ctx.closePath();
        ctx.stroke();
        const first = worldToScreen(z.query.points[0][0], z.query.points[0][1]);
        ctx.fillStyle = z.color;
        ctx.font = '12px sans-serif';
        ctx.fillText(z.name, first[0] + 4, first[1] - 6);
      }
    });
  }

  // 7. Active Selection Preview
  if (isDragging) {
    if (currentMode === 'box' && boxStart && boxEnd) {
      const x = Math.min(boxStart[0], boxEnd[0]);
      const y = Math.min(boxStart[1], boxEnd[1]);
      const w = Math.abs(boxEnd[0] - boxStart[0]);
      const h = Math.abs(boxEnd[1] - boxStart[1]);

      ctx.fillStyle = 'rgba(56, 139, 253, 0.15)';
      ctx.fillRect(x, y, w, h);
      ctx.strokeStyle = '#58a6ff';
      ctx.lineWidth = 2;
      ctx.setLineDash([4, 4]);
      ctx.strokeRect(x, y, w, h);
      ctx.setLineDash([]);
    } else if (currentMode === 'lasso' && lassoPoints.length > 1) {
      ctx.strokeStyle = '#3fb950';
      ctx.lineWidth = 2;
      ctx.beginPath();
      lassoPoints.forEach((pt, i) => {
        if (i === 0) ctx.moveTo(pt[0], pt[1]);
        else ctx.lineTo(pt[0], pt[1]);
      });
      ctx.stroke();
    }
  }
}

// Initial resize and render
resizeCanvas();
</script>
</body>
</html>
"""

def generate_sample_world_data():
    """Generates rich world data representing Satisfactory 1.2 nodes and factory hubs"""
    import random
    random.seed(42)

    # 1. Natural Resource Nodes
    nodes = []
    res_types = ["Железо", "Медь", "Уголь", "Известняк", "Нефть", "Катерий", "Кварц", "Сера", "Уран", "Азот", "Боксит"]
    purities = ["Impure", "Normal", "Normal", "Pure"]

    # Clustered natural nodes across the 8x8km world
    cluster_centers = [
        (-150000, -80000), (0, 0), (120000, -100000), (-50000, 150000), (180000, 120000), (-180000, 80000)
    ]
    for cx, cy in cluster_centers:
        for _ in range(12):
            nodes.append({
                "x": cx + random.uniform(-40000, 40000),
                "y": cy + random.uniform(-40000, 40000),
                "type": random.choice(res_types),
                "purity": random.choice(purities)
            })

    # 2. Hard Drives (Crash sites / Drop pods)
    pods = []
    for _ in range(45):
        pods.append({
            "x": random.uniform(WORLD_MIN_X * 0.8, WORLD_MAX_X * 0.8),
            "y": random.uniform(WORLD_MIN_Y * 0.8, WORLD_MAX_Y * 0.8),
            "opened": random.random() < 0.3
        })

    # 3. Player Factories & Buildables (Hubs like in user screenshot)
    buildings = []
    conveyors = []
    pipes = []

    # Hub 1: Central Modular Frame & Iron Factory
    h1_x, h1_y = -30000, -20000
    for r in range(8):
        for c in range(6):
            bx = h1_x + (c * 2500)
            by = h1_y + (r * 3200)
            is_assembler = (r % 2 == 0)
            buildings.append({
                "x": bx, "y": by,
                "power": 15.0 if is_assembler else 4.0,
                "producing": random.random() > 0.15,
                "hasPower": True,
                "inputs": {"Железный слиток": 30.0} if not is_assembler else {"Железная пластина": 20.0, "Винты": 100.0},
                "outputs": {"Железная пластина": 20.0} if not is_assembler else {"Усиленная пластина": 5.0}
            })

    # Hub 2: Steel & Motor Complex
    h2_x, h2_y = 60000, -40000
    for r in range(10):
        for c in range(8):
            bx = h2_x + (c * 2800)
            by = h2_y + (r * 3000)
            buildings.append({
                "x": bx, "y": by,
                "power": 16.0,
                "producing": True,
                "hasPower": True,
                "inputs": {"Стальной слиток": 45.0, "Проволока": 60.0},
                "outputs": {"Стальная балка": 15.0} if c < 4 else {"Стальная труба": 20.0}
            })

    # Conveyor Networks connecting hubs
    # Conveyor trunk line between Hub 1 and Hub 2
    trunk1 = []
    for step in range(15):
        t = step / 14.0
        x = h1_x + t * (h2_x - h1_x)
        y = h1_y + t * (h2_y - h1_y) + (math.sin(t * math.pi) * 8000)
        trunk1.append([x, y])
    conveyors.append({"points": trunk1, "color": "#388bfd"})

    # Local conveyor bus lines inside Hub 1
    for r in range(8):
        line = [[h1_x - 3000, h1_y + (r * 3200)], [h1_x + 18000, h1_y + (r * 3200)]]
        conveyors.append({"points": line, "color": "#58a6ff"})

    # Local conveyor bus lines inside Hub 2
    for r in range(10):
        line = [[h2_x - 3000, h2_y + (r * 3000)], [h2_x + 22000, h2_y + (r * 3000)]]
        conveyors.append({"points": line, "color": "#3fb950"})

    # Pipeline network (Water / Oil from lakes)
    p_line = [
        [h1_x + 20000, h1_y + 10000],
        [h1_x + 15000, h1_y + 25000],
        [h1_x + 5000, h1_y + 35000],
        [-10000, 50000]
    ]
    pipes.append({"points": p_line})

    return {
        "nodes": nodes,
        "pods": pods,
        "buildings": buildings,
        "conveyors": conveyors,
        "pipes": pipes
    }

class SatisToolsServer(http.server.SimpleHTTPRequestHandler):
    world_cache = None

    def do_GET(self):
        if self.path == '/' or self.path.startswith('/index'):
            self.send_response(200)
            self.send_header('Content-Type', 'text/html; charset=utf-8')
            self.end_headers()
            self.wfile.write(HTML_PAGE.encode('utf-8'))
        elif self.path == '/api/world-data':
            if SatisToolsServer.world_cache is None:
                import math
                SatisToolsServer.world_cache = generate_sample_world_data()
            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.end_headers()
            self.wfile.write(json.dumps(SatisToolsServer.world_cache).encode('utf-8'))
        else:
            self.send_error(404, "Not Found")

    def log_message(self, format, *args):
        # Quiet logger
        pass

def start_server():
    import math
    handler = SatisToolsServer
    with socketserver.TCPServer(("127.0.0.1", PORT), handler) as httpd:
        print(f"\n" + "="*60)
        print(f"  SatisTools - Интерактивный тестовый сервер запущен!")
        print(f"  URL: http://127.0.0.1:{PORT}")
        print(f"="*60 + "\n")
        httpd.serve_forever()

if __name__ == '__main__':
    # Start server in thread
    t = threading.Thread(target=start_server, daemon=True)
    t.start()

    # Open default browser
    url = f"http://127.0.0.1:{PORT}"
    print(f"Открываю браузер: {url} ...")
    webbrowser.open(url)

    print("Сервер работает. Нажмите Ctrl+C в консоли для остановки.\n")
    try:
        t.join()
    except KeyboardInterrupt:
        print("\nСервер остановлен.")
