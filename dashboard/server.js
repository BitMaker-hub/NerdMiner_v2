const dgram = require('dgram');
const express = require('express');
const http = require('http');
const { Server } = require("socket.io");
const path = require('path');

const app = express();
const server = http.createServer(app);
const io = new Server(server);
const udpSocket = dgram.createSocket('udp4');

const UDP_PORT = 33333;
const HTTP_PORT = 3000;

app.use(express.static(path.join(__dirname, 'public')));
app.use(express.json());

const miners = {};

// UDP Listener
udpSocket.on('error', (err) => {
  console.log(`UDP error:\n${err.stack}`);
  udpSocket.close();
});

udpSocket.on('message', (msg, rinfo) => {
  console.log(`UDP received: ${msg} from ${rinfo.address}:${rinfo.port}`);
  try {
    const data = JSON.parse(msg.toString());
    const id = data.id || rinfo.address;
    const ip = data.ip || rinfo.address; // Prefer reported IP to bypass Docker NAT

    // Initialize or update miner
    if (!miners[id]) {
      miners[id] = { ...data, lastSeen: Date.now(), ip: ip };
      // New miner discovered: Fetch full config to get BTC address
      fetchMinerConfig(ip, id);
    } else {
      // Preserve existing address if already fetched
      const existingAddr = miners[id].address;
      miners[id] = { ...data, lastSeen: Date.now(), ip: ip };
      if (existingAddr) miners[id].address = existingAddr;
      else fetchMinerConfig(ip, id); // Try fetching again if we missed it
    }

    io.emit('miner_update', miners[id]);
  } catch (e) {
    console.error('Invalid JSON from', rinfo.address, msg.toString());
  }
});

// Helper to fetch config and update miner state
async function fetchMinerConfig(ip, id) {
  try {
    // console.log(`Fetching config for ${id} (${ip})...`);
    const response = await fetch(`http://${ip}/api/config`, { signal: AbortSignal.timeout(3000) });
    if (response.ok) {
      const config = await response.json();
      if (miners[id]) {
        miners[id].address = config.address; // Store address
        io.emit('miner_update', miners[id]); // Push update
      }
    }
  } catch (e) {
    // console.error(`Failed to fetch config for ${ip}:`, e.message);
  }
}

udpSocket.bind(UDP_PORT, () => {
  console.log(`UDP socket listening on port ${UDP_PORT}`);
});

// Clean up old miners
setInterval(() => {
  const now = Date.now();
  for (const id in miners) {
    if (now - miners[id].lastSeen > 30000) { // 30 seconds timeout
      console.log(`Miner ${id} timed out`);
      delete miners[id];
      io.emit('miner_remove', id);
    }
  }
}, 5000);

// Bitcoin Stats
let bitcoinStats = {};

async function fetchBitcoinStats() {
  try {
    const [priceRes, heightRes, diffRes, feesRes, miningRes] = await Promise.all([
      fetch('https://mempool.space/api/v1/prices'),
      fetch('https://mempool.space/api/blocks/tip/height'),
      fetch('https://mempool.space/api/v1/difficulty-adjustment'),
      fetch('https://mempool.space/api/v1/fees/recommended'),
      fetch('https://mempool.space/api/v1/mining/hashrate/3d')
    ]);

    const prices = await priceRes.json();
    const height = parseInt(await heightRes.text());
    const diffData = await diffRes.json();
    const fees = await feesRes.json();
    const miningData = await miningRes.json();

    // Calculate Halving Progress
    const blocksPerHalving = 210000;
    const currentHalvingCycle = Math.floor(height / blocksPerHalving);
    const nextHalvingBlock = (currentHalvingCycle + 1) * blocksPerHalving;
    const blocksUntilHalving = nextHalvingBlock - height;
    const halvingProgress = ((blocksPerHalving - blocksUntilHalving) / blocksPerHalving) * 100;

    bitcoinStats = {
      price: prices.USD,
      height: height,
      difficulty: miningData.currentDifficulty, // Corrected source
      networkHashrate: miningData.currentHashrate, // Direct from API
      blocksUntilHalving: blocksUntilHalving,
      halvingProgress: halvingProgress.toFixed(2),
      fees: fees
    };

    io.emit('bitcoin_stats', bitcoinStats);
    // console.log('Updated Bitcoin Stats:', bitcoinStats);
  } catch (e) {
    console.error('Error fetching Bitcoin stats:', e.message);
  }
}

// Fetch stats every 60 seconds
setInterval(fetchBitcoinStats, 60000);
// Miner Config Proxy
app.get('/miners/:ip/config', async (req, res) => {
  try {
    const { ip } = req.params;
    console.log(`Proxying GET config to http://${ip}/api/config`);
    const response = await fetch(`http://${ip}/api/config`, { signal: AbortSignal.timeout(5000) }); // 5s timeout
    if (!response.ok) throw new Error(`Miner returned ${response.status}`);
    const data = await response.json();
    res.json(data);
  } catch (e) {
    console.error(`Proxy Error (GET ${req.params.ip}):`, e.cause || e.message);
    res.status(502).json({ error: e.message });
  }
});

app.post('/miners/:ip/config', async (req, res) => {
  try {
    const { ip } = req.params;
    console.log(`Proxying POST config to http://${ip}/api/config`);
    const response = await fetch(`http://${ip}/api/config`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(req.body),
      signal: AbortSignal.timeout(5000)
    });
    if (!response.ok) throw new Error(`Miner returned ${response.status}`);
    const data = await response.json();
    res.json(data);
  } catch (e) {
    console.error(`Proxy Error (POST ${req.params.ip}):`, e.cause || e.message);
    res.status(502).json({ error: e.message });
  }
});

fetchBitcoinStats(); // Initial fetch

const hashrateHistory = [];
const MAX_HISTORY = 1440; // 24 hours (1440 minutes)

// Update History every 60 seconds
setInterval(() => {
  let totalHashrate = 0;
  for (const id in miners) {
    totalHashrate += parseFloat(miners[id].hashrate) || 0;
  }

  const point = {
    timestamp: Date.now(),
    hashrate: totalHashrate
  };

  hashrateHistory.push(point);
  if (hashrateHistory.length > MAX_HISTORY) {
    hashrateHistory.shift();
  }

  io.emit('history_update', point);
}, 60000);

io.on('connection', (socket) => {
  socket.emit('init_miners', miners);
  socket.emit('init_history', hashrateHistory);
  if (bitcoinStats.price) {
    socket.emit('bitcoin_stats', bitcoinStats);
  }
});

server.listen(HTTP_PORT, () => {
  console.log(`Dashboard running at http://localhost:${HTTP_PORT}`);
});
