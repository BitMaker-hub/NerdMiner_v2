# NerdMiner Dashboard

This is a local dashboard to monitor and configure your NerdMiner fleet via UDP, and Rest API.

> **Updated Firmware Required**: In order to use the dashboard you must update the firmware on your specific device, The firmware has been updated to broadcast a json payload via UDP including the miner's IP address. Broadcasting the miner's IP resolves connectivity issues when running in Docker/Umbrel where the source IP is masked. Please flash the latest firmware to use Remote Configuration.

## Firmware Configuration

The NerdMiner firmware has been modified to broadcast stats to `255.255.255.255` on port `33333` via UDP.
Ensure your miners and this computer are on the same network/subnet. you can then expose this via Tailscail

## Remote Configuration

To configure a miner remotely:
1.  Ensure you are running the **latest firmware** with `api.cpp` enabled.
2.  On the dashboard, click the **Gear Icon (⚙️)** on the miner card.
3.  A modal will appear showing the current settings.
4.  Update your Pool, Wallet, etc., and click **Save Changes**.
5.  The miner will save settings to NVS and restart automatically.

**Note:** This uses a secure proxy through the dashboard backend to communicate with the miner's local API.

## Tips / donations.

Found this useful? buy me a beer or send a tip, In no way required what so ever, but much appreciated

**BTC:** bc1qjqhg5c2f6da8y4qr7upegwhkvl2376xzlpwf5p\
**ETH:**
0x1c054d43c8b6452ceb5d9fe773cc7da66764c283\
**SOL:**
GTMphvuZU3QsHbieCwWutf1gRGmLWWEVY5dPq73pkgnz\
**USDC on Ethereum:**
0x1c054d43c8b6452ceb5d9fe773cc7da66764c283

## Features

*   **Real-time Monitoring**: Live updates via UDP Broadcast (port 33333).
*   **Auto-Discovery**: Miners appear automatically when they start hashing.
*   **Detailed Stats**:
    *   Hashrate (Dynamic units: H/s, KH/s, MH/s)
    *   Miner Name (extracted from wallet address)
    *   Pool Address & Port
    *   Uptime, Temperature, Valid Shares, Templates, Best Difficulty
*   **Remote Configuration Rest API**:
    *   **Settings**: Change Pool, Port, Address, and Password remotely.
    *   **Persistence**: Settings saved to flash memory (NVS).
    *   **Auto-Restart**: Miner applies settings automatically.
*   **Bitcoin Network Stats**:
    *   Real-time Price (USD)
    *   Network Hashrate & Difficulty
    *   Halving Progress Bar
    *   Block Height & Fees
*   **Fleet Performance**:
    *   **24-Hour Graph**: Real-time line chart showing total hashrate history.
    *   **24h Average**: Rolling average calculation.
    *   **Smart Formatting**: Difficulty (k, M, G, T, P, E) and Hashrate auto-scaling.
*   **Zero Configuration**: No IP setup needed on miners; just flash and run.

## Non docker setup

1.  **Install Dependencies**:
    ```bash
    npm install
    ```

2.  **Start the Server**:
    ```bash
    node server.js
    ```

3.  **Access Dashboard**:
    Open your browser and navigate to `http://localhost:3000`.

## Docker & Umbrel Support

You can run this dashboard as a Docker container.

### Linux / Umbrel (Recommended for Production)
```bash
docker-compose up -d --build
```
*Uses `network_mode: "host"` for proper UDP broadcast reception.*

### Windows (Testing/Development)
```bash
docker-compose -f docker-compose.windows.yml up -d --build
```
*Uses port mapping. Access at `http://localhost:3000`*

**Note:** UDP broadcasts from miners may not reach the container on Windows due to Docker's networking limitations. For full functionality, deploy on Linux/Umbrel.

### Umbrel
This app is ready for Umbrel.
1.  install Portainer from the umbrel app store
2.  Once in portainer, navigate to the environment you want to add this to, then click on "Add Container"
3.  for docker.io image, use `ocybress/nerdminer-dashboard-linux:r0.0.8`
4.  for the ports, add `3000` TCP, and `33333` UDP
5.  click "diploy the container"
6.  wait for the container to start
7.  navigate to `http://localhost:3000` to access the dashboard
8.  you can expose this via tailscail if you want to access it from other devices

<img width="1903" height="1113" alt="Dashboard" src="https://github.com/user-attachments/assets/6d91323c-5c8c-4ad9-beb9-4f27e5b845e0" />
<img width="1904" height="1111" alt="Dashboard2" src="https://github.com/user-attachments/assets/772245c8-053c-433e-88d7-74bd811a85f6" />
<img width="1905" height="1112" alt="EditConfig" src="https://github.com/user-attachments/assets/67a7864e-7795-4adf-9b11-d412df330058" />




