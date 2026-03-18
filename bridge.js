// bridge.js — WebSocket + UDP + Static File Server (all in one)
// npm install ws
// node bridge.js
// Buka: http://localhost:8080

const { WebSocketServer } = require('ws');
const dgram = require('dgram');
const http  = require('http');
const fs    = require('fs');
const path  = require('path');

const PORT     = 8080;
// const UDP_HOST = '192.168.43.197';
// const UDP_PORT = 1234;
const UDP_HOST = '127.0.0.1';
const UDP_PORT = 8080;

const udp = dgram.createSocket('udp4');

// ─── HTTP server (serve index.html) ──────────────────────────────
const httpServer = http.createServer((req, res) => {
  const file = path.join(__dirname, 'index.html');
  fs.readFile(file, (err, data) => {
    if (err) { res.writeHead(404); return res.end('index.html not found'); }
    res.writeHead(200, { 'Content-Type': 'text/html' });
    res.end(data);
  });
});

// ─── WebSocket server (attach ke HTTP server yang sama) ──────────
const wss = new WebSocketServer({ server: httpServer });

wss.on('connection', (ws) => {
  console.log('[WS] Browser connected');
  ws.on('message', (data) => {
    udp.send(data, UDP_PORT, UDP_HOST);
  });
  ws.on('close', () => console.log('[WS] Browser disconnected'));
});

// ─── Start ────────────────────────────────────────────────────────
httpServer.listen(PORT, () => {
  console.log(`✅ Buka di browser: http://localhost:${PORT}`);
  console.log(`📡 UDP → ${UDP_HOST}:${UDP_PORT}`);
});