const socket = io();
const grid = document.getElementById('miners-grid');
const totalHashEl = document.getElementById('total-hashrate');

const miners = {};

function formatTime(seconds) {
    if (seconds >= 86400) {
        const d = Math.floor(seconds / 86400);
        const h = Math.floor((seconds % 86400) / 3600);
        const m = Math.floor((seconds % 3600) / 60);
        return `${d}d ${h}h ${m}m`;
    }
    const h = Math.floor(seconds / 3600);
    const m = Math.floor((seconds % 3600) / 60);
    const s = Math.floor(seconds % 60);
    return `${h}h ${m}m ${s}s`;
}

function formatCount(val) {
    if (val >= 1e12) return (val / 1e12).toFixed(2) + ' T';
    if (val >= 1e9) return (val / 1e9).toFixed(2) + ' B';
    if (val >= 1e6) return (val / 1e6).toFixed(2) + ' M';
    if (val >= 1e3) return (val / 1e3).toFixed(2) + ' k';
    return Math.floor(val).toLocaleString();
}

function createCard(key) {
    const div = document.createElement('div');
    div.className = 'miner-card';
    div.id = `card-${key}`;
    return div;
}

function formatHashrate(strValue) {
    const val = parseFloat(strValue);
    if (isNaN(val)) return '0 <span style="font-size: 0.8rem">H/s</span>';

    if (val >= 1e18) return `${(val / 1e18).toFixed(3)} <span style="font-size: 0.8rem">ZH/s</span>`;
    if (val >= 1e15) return `${(val / 1e15).toFixed(2)} <span style="font-size: 0.8rem">EH/s</span>`;
    if (val >= 1e12) return `${(val / 1e12).toFixed(2)} <span style="font-size: 0.8rem">PH/s</span>`;
    if (val >= 1e9) return `${(val / 1e9).toFixed(2)} <span style="font-size: 0.8rem">TH/s</span>`;
    if (val >= 1e6) return `${(val / 1e6).toFixed(2)} <span style="font-size: 0.8rem">GH/s</span>`;
    if (val >= 1e3) return `${(val / 1e3).toFixed(2)} <span style="font-size: 0.8rem">MH/s</span>`;
    if (val >= 1) return `${val.toFixed(2)} <span style="font-size: 0.8rem">KH/s</span>`;
    return `${(val * 1000).toFixed(0)} <span style="font-size: 0.8rem">H/s</span>`;
}

function updateCardHTML(miner) {
    const tempClass = parseFloat(miner.temp) > 70 ? 'temp-high' : '';
    const displayName = (miner.miner && miner.miner !== 'Unknown') ? miner.miner : (miner.id || 'Unknown');
    const hashrateDisplay = formatHashrate(miner.hashrate);

    return `
                <div class="card-header">
                    <div>
                        <div class="miner-id">${displayName}</div>
                        <div class="miner-ip">${miner.ip}</div>
                    </div>
                    <div style="display: flex; align-items: center; gap: 0.5rem;">
                        <button onclick="openConfigModal('${miner.ip}')" class="btn-icon" style="background: none; border: none; color: #94a3b8; cursor: pointer; padding: 4px;">
                            <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                                <path d="M12.22 2h-.44a2 2 0 0 0-2 2v.18a2 2 0 0 1-1 1.73l-.43.25a2 2 0 0 1-2 0l-.15-.08a2 2 0 0 0-2.73.73l-.22.38a2 2 0 0 0 .73 2.73l.15.1a2 2 0 0 1 1 1.72v.51a2 2 0 0 1-1 1.74l-.15.09a2 2 0 0 0-.73 2.73l.22.38a2 2 0 0 0 2.73.73l.15-.08a2 2 0 0 1 2 0l.43.25a2 2 0 0 1 1 1.73V20a2 2 0 0 0 2 2h.44a2 2 0 0 0 2-2v-.18a2 2 0 0 1 1-1.73l.43-.25a2 2 0 0 1 2 0l.15.08a2 2 0 0 0 2.73-.73l.22-.39a2 2 0 0 0-.73-2.73l-.15-.09a2 2 0 0 1-1-1.74v-.51a2 2 0 0 1 1-1.74l.15-.09a2 2 0 0 0 .73-2.73l-.22-.38a2 2 0 0 0-2.73-.73l-.15.08a2 2 0 0 1-2 0l-.43-.25a2 2 0 0 1-1-1.73V4a2 2 0 0 0-2-2z"></path>
                                <circle cx="12" cy="12" r="3"></circle>
                            </svg>
                        </button>
                        <div class="status-badge">
                            <span class="status-dot"></span> Online
                        </div>
                    </div>
                </div>
                <div style="margin-bottom: 1rem; padding-bottom: 1rem; border-bottom: 1px solid rgba(255,255,255,0.05);">
                    <div style="font-size: 0.8rem; color: #94a3b8;">Pool: <span style="color: #f8fafc;">${miner.pool || 'Unknown'}</span></div>
                    ${miner.address ? `<div style="font-size: 0.8rem; color: #94a3b8; margin-top: 0.25rem;">Addr: <span style="color: #f8fafc; font-family: monospace;">...${miner.address.split('.')[0].slice(-8)}</span></div>` : ''}
                    ${(miner.miner && miner.miner !== 'Unknown') ? `<div style="font-size: 0.8rem; color: #94a3b8; margin-top: 0.25rem;">ID: <span style="font-family: monospace;">${miner.id}</span></div>` : ''}
                </div>
                <div class="stats-grid">
                    <div class="stat-item" style="grid-column: span 2;">
                        <span class="stat-label">Hashrate</span>
                        <span class="stat-value hashrate">${hashrateDisplay}</span>
                    </div>
                    <div class="stat-item">
                        <span class="stat-label">Valid Shares</span>
                        <span class="stat-value">${miner.valid}</span>
                    </div>
                    <div class="stat-item">
                        <span class="stat-label">Templates</span>
                        <span class="stat-value">${formatCount(parseFloat(miner.templates) || 0)}</span>
                    </div>
                    <div class="stat-item">
                        <span class="stat-label">Best Diff</span>
                        <span class="stat-value">${formatDifficulty(parseFloat(miner.bestDiff) || 0)}</span>
                    </div>
                    <div class="stat-item">
                        <span class="stat-label">Temp</span>
                        <span class="stat-value ${tempClass}">${miner.temp}°C</span>
                    </div>
                    <div class="stat-item">
                        <span class="stat-label">Uptime</span>
                        <span class="stat-value">${formatTime(miner.uptime)}</span>
                    </div>
                </div>
            `;
}

function render() {
    let totalHash = 0;

    // Sort miners by IP or ID
    const sortedKeys = Object.keys(miners).sort();

    sortedKeys.forEach(key => {
        const miner = miners[key];
        totalHash += parseFloat(miner.hashrate) || 0;

        let card = document.getElementById(`card-${key}`);
        if (!card) {
            card = createCard(key);
            grid.appendChild(card);
        }
        card.innerHTML = updateCardHTML(miner);
    });

    // Remove old cards
    const currentIds = new Set(sortedKeys.map(k => `card-${k}`));
    Array.from(grid.children).forEach(child => {
        if (!currentIds.has(child.id)) {
            child.remove();
        }
    });

    totalHashEl.innerHTML = `${formatHashrate(totalHash)} Total`;
}

socket.on('init_miners', (data) => {
    Object.assign(miners, data);
    render();
});

socket.on('miner_update', (data) => {
    const key = data.id || data.ip;
    miners[key] = data;
    render();
});

socket.on('miner_remove', (id) => {
    delete miners[id];
    render();
});

socket.on('bitcoin_stats', (data) => {
    document.getElementById('bitcoin-card').style.display = 'block';

    document.getElementById('btc-price').innerText = `$${data.price.toLocaleString()}`;
    document.getElementById('btc-height').innerText = data.height.toLocaleString();

    document.getElementById('btc-halving').innerText = `${data.halvingProgress}%`;
    document.getElementById('btc-halving-bar').style.width = `${data.halvingProgress}%`;

    const diffVal = data.difficulty; // Now validated
    let diffStr = diffVal.toLocaleString();
    if (diffVal > 1e12) diffStr = (diffVal / 1e12).toFixed(2) + ' T';
    document.getElementById('btc-diff').innerText = diffStr;

    // Use pre-fetched network hashrate (convert H/s to KH/s)
    document.getElementById('btc-network-hash').innerHTML = formatHashrate(data.networkHashrate / 1000);

    if (data.fees) {
        document.getElementById('btc-fees').innerHTML = `
                    <span style="color: #ef4444">${data.fees.fastestFee}</span> / 
                    <span style="color: #f7931a">${data.fees.hourFee}</span>
                `;
    }
});

// Chart.js Setup
const ctx = document.getElementById('hashrateChart').getContext('2d');
const hashrateChart = new Chart(ctx, {
    type: 'line',
    data: {
        labels: [],
        datasets: [{
            label: 'Total Hashrate (KH/s)',
            data: [],
            borderColor: '#10b981',
            backgroundColor: 'rgba(16, 185, 129, 0.1)',
            borderWidth: 2,
            fill: true,
            tension: 0.4,
            pointRadius: 0
        }]
    },
    options: {
        responsive: true,
        maintainAspectRatio: false,
        plugins: {
            legend: { display: false }
        },
        scales: {
            x: { display: false },
            y: {
                grid: { color: 'rgba(255, 255, 255, 0.05)' },
                ticks: { color: '#94a3b8' }
            }
        }
    }
});

function updateGraph(history) {
    const labels = history.map(p => new Date(p.timestamp).toLocaleTimeString());
    const data = history.map(p => p.hashrate);

    hashrateChart.data.labels = labels;
    hashrateChart.data.datasets[0].data = data;
    hashrateChart.update();

    // Calculate Average
    if (data.length > 0) {
        const sum = data.reduce((a, b) => a + b, 0);
        const avg = sum / data.length;
        document.getElementById('hourly-avg').innerHTML = formatHashrate(avg);
    }
}

socket.on('init_history', (history) => {
    updateGraph(history);
});

socket.on('history_update', (point) => {
    // Add new point
    hashrateChart.data.labels.push(new Date(point.timestamp).toLocaleTimeString());
    hashrateChart.data.datasets[0].data.push(point.hashrate);

    // Limit to 1440 points (24h)
    if (hashrateChart.data.labels.length > 1440) {
        hashrateChart.data.labels.shift();
        hashrateChart.data.datasets[0].data.shift();
    }
    hashrateChart.update();

    // Re-calculate Average
    const data = hashrateChart.data.datasets[0].data;
    const sum = data.reduce((a, b) => a + b, 0);
    const avg = sum / data.length;
    document.getElementById('hourly-avg').innerHTML = formatHashrate(avg);
});

function formatDifficulty(val) {
    if (val >= 1e18) return (val / 1e18).toFixed(2) + ' E';
    if (val >= 1e15) return (val / 1e15).toFixed(2) + ' P';
    if (val >= 1e12) return (val / 1e12).toFixed(2) + ' T';
    if (val >= 1e9) return (val / 1e9).toFixed(2) + ' G';
    if (val >= 1e6) return (val / 1e6).toFixed(2) + ' M';
    if (val >= 1e3) return (val / 1e3).toFixed(2) + ' k';
    return val.toFixed(2);
}

// Copy BTC address function
function copyBtcAddress() {
    const addressEl = document.getElementById('btc-donation-address');
    const address = addressEl.innerText;

    navigator.clipboard.writeText(address).then(() => {
        const btn = event.target.closest('.copy-btn');
        const originalText = btn.innerHTML;
        btn.innerHTML = '<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><polyline points="20 6 9 17 4 12"></polyline></svg> Copied!';
        btn.style.background = '#059669';

        setTimeout(() => {
            btn.innerHTML = originalText;
            btn.style.background = '';
        }, 2000);
    }).catch(err => {
        console.error('Failed to copy:', err);
        alert('Failed to copy address. Please copy manually.');
    });
}

// Configuration Modal Logic
const modal = document.getElementById('configModal');

async function openConfigModal(ip) {
    document.getElementById('configIp').value = ip;
    document.getElementById('configPool').value = 'Loading...';
    document.getElementById('configPort').value = '';
    document.getElementById('configAddr').value = '';
    document.getElementById('configPass').value = '';
    document.getElementById('configTz').value = '';

    modal.classList.add('active');

    // Find miner name
    const minerKey = Object.keys(miners).find(key => miners[key].ip === ip);
    const miner = miners[minerKey];
    const minerName = (miner && miner.miner && miner.miner !== 'Unknown') ? miner.miner : 'Miner';
    document.getElementById('configModalTitle').innerText = `${minerName} Configuration`;

    try {
        const res = await fetch(`/miners/${ip}/config`);
        if (!res.ok) throw new Error('Failed to fetch config');
        const data = await res.json();

        document.getElementById('configPool').value = data.pool || '';
        document.getElementById('configPort').value = data.port || '';
        document.getElementById('configAddr').value = data.address || '';
        document.getElementById('configPass').value = data.password || '';
        document.getElementById('configTz').value = data.timezone || 0;
    } catch (e) {
        alert('Error loading configuration: ' + e.message);
        closeConfigModal();
    }
}

function closeConfigModal() {
    modal.classList.remove('active');
}

async function saveConfig() {
    const ip = document.getElementById('configIp').value;
    const btn = document.querySelector('#configModal .btn-primary');
    const originalText = btn.innerText;

    const config = {
        pool: document.getElementById('configPool').value,
        port: parseInt(document.getElementById('configPort').value),
        address: document.getElementById('configAddr').value,
        password: document.getElementById('configPass').value,
        timezone: parseInt(document.getElementById('configTz').value)
    };

    btn.disabled = true;
    btn.innerText = 'Saving...';

    try {
        const res = await fetch(`/miners/${ip}/config`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(config)
        });

        const data = await res.json();
        if (!data.success) throw new Error(data.message || 'Save failed');

        alert('Configuration saved! Miner is restarting...');
        closeConfigModal();
    } catch (e) {
        alert('Error saving configuration: ' + e.message);
    } finally {
        btn.disabled = false;
        btn.innerText = originalText;
    }
}

// Close modal when clicking outside
modal.addEventListener('click', (e) => {
    if (e.target === modal) closeConfigModal();
});
