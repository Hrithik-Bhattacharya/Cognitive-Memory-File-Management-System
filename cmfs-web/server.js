const { spawn } = require('child_process');
const WebSocket = require('ws');
const http = require('http');
const fs = require('fs');
const path = require('path');

console.log("1. Script started...");

const exePath = path.join(__dirname, 'cmfs.exe');

// Check if the file actually exists
if (!fs.existsSync(exePath)) {
    console.error("❌ ERROR: cmfs.exe not found at:", exePath);
    process.exit(1);
}

console.log("2. Found cmfs.exe, attempting to spawn...");

const cmfs = spawn(exePath);

// Handle process crash
cmfs.on('error', (err) => {
    console.error("❌ ERROR: Failed to start C++ process:", err);
});

cmfs.on('close', (code) => {
    console.log(`3. C++ process exited with code ${code}`);
});

const server = http.createServer();
const wss = new WebSocket.Server({ server });

wss.on('connection', (ws) => {
    console.log("✅ React UI Connected");

    ws.on('message', (message) => {
        // Just forward the JSON from React directly to C++
        // C++ is now expecting the JSON format
        cmfs.stdin.write(message.toString() + "\n");
    });
});

cmfs.stdout.on('data', (data) => {
    console.log("C++ >>", data.toString());
    wss.clients.forEach(c => c.readyState === WebSocket.OPEN && c.send(data.toString()));
});

server.listen(3001, () => {
    console.log("🚀 4. Node Server running on http://localhost:3001");
});