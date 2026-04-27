#pragma once
#include <pgmspace.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>openUC2 Microscope Controller</title>
<style>
  :root { --blue:#023773; --green:#85b918; --teal:#1f9c7c; --grey:#999; --white:#fff; --radius:8px; }
  * { box-sizing:border-box; margin:0; padding:0; }
  body { background:var(--blue); color:var(--white); font-family:"Segoe UI",Arial,sans-serif; min-height:100vh; }
  header { background:var(--blue); border-bottom:4px solid var(--green); padding:16px 24px; display:flex; align-items:center; gap:16px; }
  header img { height:38px; }
  .htext h1 { font-size:1rem; font-weight:700; letter-spacing:2px; text-transform:uppercase; }
  .htext p  { font-size:.7rem; color:var(--grey); letter-spacing:1px; }
  .sbar { background:#012a5a; display:flex; flex-wrap:wrap; gap:10px; padding:10px 24px; border-bottom:2px solid var(--teal); }
  .badge { background:rgba(255,255,255,.07); border:1px solid rgba(255,255,255,.15); border-radius:4px; padding:5px 12px; font-size:.7rem; letter-spacing:1px; }
  .badge span { color:var(--green); font-weight:700; }
  .grid { display:grid; grid-template-columns:repeat(auto-fit,minmax(260px,1fr)); gap:16px; padding:20px 24px; }
  .card { background:#012a5a; border:1px solid rgba(255,255,255,.12); border-top:3px solid var(--green); border-radius:var(--radius); padding:18px; }
  .card.t { border-top-color:var(--teal); }
  .card h2 { font-size:.76rem; letter-spacing:2px; text-transform:uppercase; color:var(--green); margin-bottom:14px; padding-bottom:8px; border-bottom:1px solid rgba(255,255,255,.1); }
  .card.t h2 { color:var(--teal); }
  label { font-size:.7rem; color:var(--grey); display:block; margin-bottom:4px; margin-top:10px; }
  input[type=range] { width:100%; accent-color:var(--green); height:4px; }
  input[type=color] { width:100%; height:38px; border:1px solid rgba(255,255,255,.2); border-radius:4px; background:none; cursor:pointer; }
  .val { float:right; color:var(--green); font-size:.7rem; font-weight:700; }
  button { margin-top:12px; width:100%; padding:10px; background:transparent; border:1px solid var(--green); color:var(--green); border-radius:var(--radius); cursor:pointer; font-family:inherit; font-size:.76rem; letter-spacing:2px; text-transform:uppercase; transition:all .2s; }
  button:hover { background:var(--green); color:var(--blue); }
  button.s { border-color:#e74c3c; color:#e74c3c; } button.s:hover { background:#e74c3c; color:#fff; }
  button.t { border-color:var(--teal); color:var(--teal); } button.t:hover { background:var(--teal); color:#fff; }
  #log { color:var(--grey); font-size:.67rem; margin:0 24px 20px; height:52px; overflow-y:auto; background:rgba(0,0,0,.2); border-radius:4px; padding:8px 10px; font-family:monospace; }
  footer { background:#011d40; border-top:4px solid var(--green); text-align:center; padding:12px; font-size:.67rem; color:var(--grey); letter-spacing:1px; }
  footer a { color:var(--teal); text-decoration:none; }
</style>
</head>
<body>
<header>
  <img src="/logo" alt="openUC2" onerror="this.style.display='none'">
  <div class="htext"><h1>Microscope Controller</h1><p>XIAO ESP32S3 // openUC2 Platform</p></div>
</header>
<div class="sbar">
  <div class="badge">TOUCH 1: <span id="t1">idle</span></div>
  <div class="badge">TOUCH 2: <span id="t2">idle</span></div>
  <div class="badge">LED: <span id="ls">off</span></div>
  <div class="badge">IP: <span id="ip">-</span></div>
</div>
<div class="grid">
  <div class="card">
    <h2>&#9632; Illumination</h2>
    <label>Colour</label>
    <input type="color" id="npColor" value="#003200">
    <label>Brightness <span class="val" id="bv">50</span></label>
    <input type="range" id="brightness" min="0" max="255" value="50" oninput="document.getElementById('bv').textContent=this.value">
    <button onclick="sendNP()">Apply</button>
    <button class="s" onclick="npOff()">Off</button>
  </div>
  <div class="card t">
    <h2>&#9632; Motor 1 - Focus</h2>
    <label>Speed / Direction <span class="val" id="m1v">0</span></label>
    <input type="range" id="motor1" min="-255" max="255" value="0" oninput="document.getElementById('m1v').textContent=this.value">
    <button class="t" onclick="sendMotor(1)">Set</button>
    <button class="s" onclick="stopMotor(1)">Stop</button>
  </div>
  <div class="card t">
    <h2>&#9632; Motor 2 - Stage</h2>
    <label>Speed / Direction <span class="val" id="m2v">0</span></label>
    <input type="range" id="motor2" min="-255" max="255" value="0" oninput="document.getElementById('m2v').textContent=this.value">
    <button class="t" onclick="sendMotor(2)">Set</button>
    <button class="s" onclick="stopMotor(2)">Stop</button>
  </div>
  <div class="card">
    <h2>&#9632; Servo 1</h2>
    <label>Angle <span class="val" id="s1v">90</span>&deg;</label>
    <input type="range" id="servo1" min="0" max="180" value="90" oninput="document.getElementById('s1v').textContent=this.value">
    <button onclick="sendServo(1)">Set</button>
  </div>
  <div class="card">
    <h2>&#9632; Servo 2</h2>
    <label>Angle <span class="val" id="s2v">90</span>&deg;</label>
    <input type="range" id="servo2" min="0" max="180" value="90" oninput="document.getElementById('s2v').textContent=this.value">
    <button onclick="sendServo(2)">Set</button>
  </div>
  <div class="card">
    <h2>&#9632; I2C Scanner</h2>
    <div id="i2cr" style="font-size:.73rem;color:var(--grey);min-height:40px;padding:4px 0">Press Scan...</div>
    <button class="t" onclick="scanI2C()">Scan</button>
  </div>
</div>
<div id="log"></div>
<footer>openUC2 Open Science Hardware &mdash; <a href="https://openuc2.com" target="_blank">openuc2.com</a></footer>
<script>
function log(m){ const e=document.getElementById('log'); e.innerHTML=new Date().toLocaleTimeString()+' &gt; '+m+'<br>'+e.innerHTML; }
function rgb(h){ return{r:parseInt(h.slice(1,3),16),g:parseInt(h.slice(3,5),16),b:parseInt(h.slice(5,7),16)}; }
function sendNP(){ const c=rgb(document.getElementById('npColor').value),br=document.getElementById('brightness').value;
  fetch('/neopixel?r='+c.r+'&g='+c.g+'&b='+c.b+'&brightness='+br).then(r=>r.text()).then(t=>log('NeoPixel: '+t)); }
function npOff(){ fetch('/neopixel?r=0&g=0&b=0&brightness=0').then(()=>log('NeoPixel off')); }
function sendMotor(n){ const s=document.getElementById('motor'+n).value;
  fetch('/motor?n='+n+'&speed='+s).then(r=>r.text()).then(t=>log('Motor '+n+': '+t)); }
function stopMotor(n){ document.getElementById('motor'+n).value=0; document.getElementById('m'+n+'v').textContent='0';
  fetch('/motor?n='+n+'&speed=0').then(()=>log('Motor '+n+' stopped')); }
function sendServo(n){ const a=document.getElementById('servo'+n).value;
  fetch('/servo?n='+n+'&angle='+a).then(r=>r.text()).then(t=>log('Servo '+n+': '+t)); }
function scanI2C(){ fetch('/i2c').then(r=>r.json()).then(d=>{
  const e=document.getElementById('i2cr');
  e.innerHTML=d.devices.length ? d.devices.map(a=>'<span style="color:var(--teal);font-weight:700">0x'+a.toString(16).padStart(2,'0').toUpperCase()+'</span>').join('  ')
    : '<span style="color:#e74c3c">No devices found</span>';
  log('I2C: '+d.devices.length+' device(s)'); }); }
function poll(){ fetch('/status').then(r=>r.json()).then(d=>{
  ['t1','t2'].forEach((id,i)=>{ const el=document.getElementById(id),val=d['touch'+(i+1)];
    el.textContent=val?'ACTIVE':'idle'; el.style.color=val?'var(--green)':'var(--grey)'; });
  const ls=document.getElementById('ls'); ls.textContent=d.ledOn?'ON':'off'; ls.style.color=d.ledOn?'var(--green)':'var(--grey)';
  document.getElementById('ip').textContent=d.ip; }).catch(()=>{}); }
setInterval(poll,1000); poll();
</script>
</body>
</html>
)rawliteral";
