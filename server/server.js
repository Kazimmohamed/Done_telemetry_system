// server.js
const express = require("express");
const http = require("http");
const WebSocket = require("ws");
const cors = require("cors");

const app = express();
app.use(cors()); // Allow CORS for frontend communication

const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

// Store frontend clients separately
const clients = new Set();

wss.on("connection", (ws, req) => {
  console.log("New client connected.");

  ws.on("message", (message) => {
    try {
      const data = JSON.parse(message);

      // Basic structure validation (optional)
      if (
        "pitch" in data &&
        "roll" in data &&
        "yaw" in data &&
        "temperature" in data &&
        "humidity" in data &&
        "battery" in data
      ) {
        console.log("Received data from ESP32:", data);

        // Broadcast to all frontend clients
        clients.forEach((client) => {
          if (client.readyState === WebSocket.OPEN) {
            client.send(JSON.stringify(data));
          }
        });
      } else {
        console.log("Received message but not valid telemetry:", data);
      }
    } catch (err) {
      console.error("Error parsing message:", err);
    }
  });

  ws.on("close", () => {
    clients.delete(ws);
    console.log("Client disconnected.");
  });

  // Add to frontend clients if it’s not ESP32
  const ip = req.socket.remoteAddress;
  if (!ip.includes("192.168")) {
    // Assuming ESP32 is in 192.168.x.x range, others are frontend
    clients.add(ws);
  }
});

// Simple health endpoint
app.get("/", (req, res) => {
  res.send("WebSocket server is running.");
});

// Start server
const PORT = 3000;
server.listen(PORT, () => {
  console.log(`Server running on http://localhost:${PORT}`);
});
