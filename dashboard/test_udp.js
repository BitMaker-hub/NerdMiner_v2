const dgram = require('dgram');
const client = dgram.createSocket('udp4');
client.bind(0, () => {
    client.setBroadcast(true);
});

const message = JSON.stringify({
    id: "TEST_MINER_01",
    miner: "Miner_Alpha",
    pool: "pool.nerdminers.org:3333",
    bestDiff: "42.00001",
    hashrate: "123.45",
    shares: 10,
    valid: 9,
    temp: "45.0",
    uptime: 100
});

console.log("Sending test packet...");
client.send(message, 33333, '127.0.0.1', (err) => {
    if (err) {
        console.error(err);
        client.close();
    } else {
        console.log("Packet sent to 127.0.0.1:33333");
    }
});

client.send(message, 33333, '255.255.255.255', (err) => {
    if (err) {
        console.error(err);
        client.close();
    } else {
        console.log("Packet sent to 255.255.255.255:33333");
        client.close();
    }
});
