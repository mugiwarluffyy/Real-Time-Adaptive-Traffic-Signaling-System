#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <Ticker.h>

const char* ssid = "manaslap";
const char* password = "1234567890";
WebServer server(80);

Ticker decrementTicker;
Ticker yellowTicker;
Ticker greenTicker;

int vehicleCount1 = 0, vehicleCount2 = 0, vehicleCount3 = 0, vehicleCount4 = 0;
int currentPost = 1;
bool paused = false;

unsigned long greenDuration = 5000;
unsigned long yellowDuration = 2000;
unsigned long decrementRateGreen = 400;
unsigned long decrementRateYellow = 700;
unsigned long threshold = 5;

enum LightStatus { RED, YELLOW, GREEN, ADJACENT };
LightStatus lightStatus1 = RED, lightStatus2 = RED, lightStatus3 = RED, lightStatus4 = RED;
int timer1 = 0, timer2 = 0, timer3 = 0, timer4 = 0;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>Traffic Light Control</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      background-image: url(''); 
      background-size: cover;
      font-family: Arial, sans-serif;
      color: #fff;
      text-align: center;
      margin: 0;
      padding: 0;
    }
    h1 {
      margin-top: 50px;
      font-size: 2.5em;
      color: #4CAF50;
    }
    .container {
      display: flex;
      justify-content: center;
      flex-wrap: wrap;
      gap: 20px;
      padding: 20px;
    }
    .post-box {
      background: rgba(0, 0, 0, 0.8);
      border: 2px solid #4CAF50;
      border-radius: 10px;
      padding: 20px;
      width: 200px;
      text-align: center;
      transition: transform 0.3s, background 0.3s;
    }
    .post-box:hover {
      transform: scale(1.05);
      background: rgba(0, 0, 0, 0.9);
    }
    .timer {
      font-size: 1.5em;
      margin-bottom: 10px;
    }
    .round-timer {
      display: inline-block;
      width: 50px;
      height: 50px;
      border-radius: 50%;
      background: rgba(255, 255, 255, 0.5);
      line-height: 50px;
      font-size: 1.5em;
      color: #000;
      margin-bottom: 10px;
      transition: background 0.3s;
    }
    .round-timer:hover {
      background: rgba(255, 255, 255, 0.8);
    }
    .status-box {
      margin-top: 20px;
      padding: 10px;
      border-radius: 10px;
      background: rgba(0, 0, 0, 0.8);
    }
    .light-indicator {
      width: 30px;
      height: 30px;
      border-radius: 50%;
      display: inline-block;
      margin: 5px;
    }
    .red-light {
      background: red;
    }
    .yellow-light {
      background: yellow;
    }
    .green-light {
      background: green;
    }
    .adjacent-light {
      background: #00FF00; /* Different green for adjacent turn */
    }
    button, input[type=range] {
      background: #4CAF50;
      color: white;
      border: none;
      padding: 10px 20px;
      cursor: pointer;
      border-radius: 5px;
      transition: background 0.3s;
      margin: 10px;
    }
    button:hover, input[type=range] {
      background: #45a049;
    }
    .control-menu {
      background: rgba(0, 0, 0, 0.8);
      border: 2px solid #4CAF50;
      border-radius: 10px;
      padding: 20px;
      margin-top: 20px;
      text-align: left;
    }
  </style>
</head>
<body>
  <h1>Traffic Light Control</h1>
  <div class="container">
    <div class="post-box" id="post1" onclick="editVehicleCount(1)">
      <div class="round-timer" id="timer1">0</div>
      <h2>Traffic Post 1</h2>
      <p>Vehicles: <span id="vehiclecount1">0</span></p>
      <div class="status-box">
        <div class="light-indicator red-light" id="lightStatus1Red"></div>
        <div class="light-indicator yellow-light" id="lightStatus1Yellow"></div>
        <div class="light-indicator green-light" id="lightStatus1Green"></div>
        <div class="light-indicator adjacent-light" id="lightStatus1Adjacent"></div>
      </div>
    </div>
    <div class="post-box" id="post2" onclick="editVehicleCount(2)">
      <div class="round-timer" id="timer2">0</div>
      <h2>Traffic Post 2</h2>
      <p>Vehicles: <span id="vehiclecount2">0</span></p>
      <div class="status-box">
        <div class="light-indicator red-light" id="lightStatus2Red"></div>
        <div class="light-indicator yellow-light" id="lightStatus2Yellow"></div>
        <div class="light-indicator green-light" id="lightStatus2Green"></div>
        <div class="light-indicator adjacent-light" id="lightStatus2Adjacent"></div>
      </div>
    </div>
    <div class="post-box" id="post3" onclick="editVehicleCount(3)">
      <div class="round-timer" id="timer3">0</div>
      <h2>Traffic Post 3</h2>
      <p>Vehicles: <span id="vehiclecount3">0</span></p>
      <div class="status-box">
        <div class="light-indicator red-light" id="lightStatus3Red"></div>
        <div class="light-indicator yellow-light" id="lightStatus3Yellow"></div>
        <div class="light-indicator green-light" id="lightStatus3Green"></div>
        <div class="light-indicator adjacent-light" id="lightStatus3Adjacent"></div>
      </div>
    </div>
    <div class="post-box" id="post4" onclick="editVehicleCount(4)">
      <div class="round-timer" id="timer4">0</div>
      <h2>Traffic Post 4</h2>
      <p>Vehicles: <span id="vehiclecount4">0</span></p>
      <div class="status-box">
        <div class="light-indicator red-light" id="lightStatus4Red"></div>
        <div class="light-indicator yellow-light" id="lightStatus4Yellow"></div>
        <div class="light-indicator green-light" id="lightStatus4Green"></div>
        <div class="light-indicator adjacent-light" id="lightStatus4Adjacent"></div>
      </div>
    </div>
  </div>
  <div class="control-menu">
    <h2>Control Menu</h2>
    <label for="decrementRate">Vehicle Count Decrement Rate (ms):</label>
    <input type="range" id="decrementRate" min="100" max="1000" step="100" value="400">
    <span id="decrementRateValue">400</span> ms
    <br>
    <label for="greenDuration">Green Light Duration (ms):</label>
    <input type="range" id="greenDuration" min="1000" max="10000" step="1000" value="5000">
    <span id="greenDurationValue">5000</span> ms
    <br>
    <label for="yellowDuration">Yellow Light Duration (ms):</label>
    <input type="range" id="yellowDuration" min="1000" max="5000" step="500" value="2000">
    <span id="yellowDurationValue">2000</span> ms
    <br>
    <label for="threshold">Vehicle Count Threshold:</label>
    <input type="range" id="threshold" min="1" max="20" step="1" value="5">
    <span id="thresholdValue">5</span>
    <br>
  </div>
  <button id="pauseButton">Pause</button>
  <button id="resumeButton" style="display:none;">Resume</button>
  <button id="switchMode">Switch to Density Mode</button>
  <script>
    let paused = false;
    document.getElementById('pauseButton').addEventListener('click', function() {
      paused = true;
      fetch('/pause');
      this.style.display = 'none';
      document.getElementById('resumeButton').style.display = 'inline';
    });
    document.getElementById('resumeButton').addEventListener('click', function() {
      paused = false;
      fetch('/resume');
      this.style.display = 'none';
      document.getElementById('pauseButton').style.display = 'inline';
    });
    document.getElementById('switchMode').addEventListener('click', function() {
      if (this.innerText === 'Switch to Density Mode') {
        this.innerText = 'Switch to Time Mode';
        fetch('/density');
      } else {
        this.innerText = 'Switch to Density Mode';
        fetch('/time');
      }
    });

    document.getElementById('decrementRate').addEventListener('input', function() {
      document.getElementById('decrementRateValue').innerText = this.value;
      fetch('/update?decrementRate=' + this.value);
    });
    document.getElementById('greenDuration').addEventListener('input', function() {
      document.getElementById('greenDurationValue').innerText = this.value;
      fetch('/update?greenDuration=' + this.value);
    });
    document.getElementById('yellowDuration').addEventListener('input', function() {
      document.getElementById('yellowDurationValue').innerText = this.value;
      fetch('/update?yellowDuration=' + this.value);
    });
    document.getElementById('threshold').addEventListener('input', function() {
      document.getElementById('thresholdValue').innerText = this.value;
      fetch('/update?threshold=' + this.value);
    });

    function editVehicleCount(postId) {
      const newCount = prompt('Enter new vehicle count:');
      if (newCount !== null) {
        fetch('/updateVehicleCount?postId=' + postId + '&count=' + newCount);
      }
    }

    const source = new EventSource('/events');

    source.onmessage = function(event) {
      const data = JSON.parse(event.data);
      for (let i = 1; i <= 4; i++) {
        document.getElementById('vehiclecount' + i).innerText = data['vehiclecount' + i];
        document.getElementById('timer' + i).innerText = data['timer' + i];
        document.getElementById('lightStatus' + i + 'Red').style.opacity = data['lightStatus' + i] === 'red' ? 1 : 0.3;
        document.getElementById('lightStatus' + i + 'Yellow').style.opacity = data['lightStatus' + i] === 'yellow' ? 1 : 0.3;
        document.getElementById('lightStatus' + i + 'Green').style.opacity = data['lightStatus' + i] === 'green' ? 1 : 0.3;
        document.getElementById('lightStatus' + i + 'Adjacent').style.opacity = data['lightStatus' + i] === 'adjacent' ? 1 : 0.3;
      }
    };
  </script>
</body>
</html>
)rawliteral";


void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("");
  Serial.print("Connected to WiFi:");
  Serial.println(WiFi.localIP());

  if (!MDNS.begin("esp32")) {
    Serial.println("Error starting mDNS");
    return;
  }

  server.on("/", handleRoot);
  server.on("/pause", handlePause);
  server.on("/resume", handleResume);
  server.on("/density", handleDensity);
  server.on("/time", handleTime);
  server.on("/update", handleUpdate);
  server.on("/updateVehicleCount", handleUpdateVehicleCount);
  server.on("/events", handleEvents);
  server.begin();

  greenTicker.attach_ms(greenDuration, switchToNextPost);
}

void handleRoot() {
  server.send_P(200, "text/html", index_html);
}

void handlePause() {
  paused = true;
  server.send(200, "text/plain", "Paused");
}

void handleResume() {
  paused = false;
  server.send(200, "text/plain", "Resumed");
}

void handleDensity() {
  // Implementation for switching to density mode
  server.send(200, "text/plain", "Switched to Density Mode");
}

void handleTime() {
  // Implementation for switching to time mode
  server.send(200, "text/plain", "Switched to Time Mode");
}

void handleUpdate() {
  if (server.hasArg("decrementRate")) {
    decrementRateGreen = server.arg("decrementRate").toInt();
  }
  if (server.hasArg("greenDuration")) {
    greenDuration = server.arg("greenDuration").toInt();
    greenTicker.attach_ms(greenDuration, switchToNextPost);
  }
  if (server.hasArg("yellowDuration")) {
    yellowDuration = server.arg("yellowDuration").toInt();
  }
  if (server.hasArg("threshold")) {
    threshold = server.arg("threshold").toInt();
  }
  server.send(200, "text/plain", "Updated");
}

void handleUpdateVehicleCount() {
  int postId = server.arg("postId").toInt();
  int count = server.arg("count").toInt();
  switch (postId) {
    case 1: vehicleCount1 = count; break;
    case 2: vehicleCount2 = count; break;
    case 3: vehicleCount3 = count; break;
    case 4: vehicleCount4 = count; break;
  }
  server.send(200, "text/plain", "Vehicle count updated");
}

void handleEvents() {
  String json = createJson();
  server.send(200, "text/event-stream", json.c_str());
}

void switchToNextPost() {
  if (paused) return;

  switch (currentPost) {
    case 1:
      if (vehicleCount1 >= threshold) {
        // Skip or delay
        break;
      }
      lightStatus1 = GREEN;
      lightStatus2 = RED;
      lightStatus3 = RED;
      lightStatus4 = RED;
      greenTicker.detach();
      yellowTicker.attach_ms(yellowDuration, switchToYellow1);
      decrementTicker.attach_ms(decrementRateGreen, decrementVehicleCount1);
      break;
    case 2:
      if (vehicleCount2 >= threshold) {
        // Skip or delay
        break;
      }
      lightStatus1 = RED;
      lightStatus2 = GREEN;
      lightStatus3 = RED;
      lightStatus4 = RED;
      greenTicker.detach();
      yellowTicker.attach_ms(yellowDuration, switchToYellow2);
      decrementTicker.attach_ms(decrementRateGreen, decrementVehicleCount2);
      break;
    case 3:
      if (vehicleCount3 >= threshold) {
        // Skip or delay
        break;
      }
      lightStatus1 = RED;
      lightStatus2 = RED;
      lightStatus3 = GREEN;
      lightStatus4 = RED;
      greenTicker.detach();
      yellowTicker.attach_ms(yellowDuration, switchToYellow3);
      decrementTicker.attach_ms(decrementRateGreen, decrementVehicleCount3);
      break;
    case 4:
      if (vehicleCount4 >= threshold) {
        // Skip or delay
        break;
      }
      lightStatus1 = RED;
      lightStatus2 = RED;
      lightStatus3 = RED;
      lightStatus4 = GREEN;
      greenTicker.detach();
      yellowTicker.attach_ms(yellowDuration, switchToYellow4);
      decrementTicker.attach_ms(decrementRateGreen, decrementVehicleCount4);
      break;
  }
  currentPost = (currentPost % 4) + 1;
}

void switchToYellow1() {
  lightStatus1 = YELLOW;
  decrementTicker.attach_ms(decrementRateYellow, decrementVehicleCount1);
  yellowTicker.detach();
  greenTicker.attach_ms(greenDuration, switchToNextPost);
}

void switchToYellow2() {
  lightStatus2 = YELLOW;
  decrementTicker.attach_ms(decrementRateYellow, decrementVehicleCount2);
  yellowTicker.detach();
  greenTicker.attach_ms(greenDuration, switchToNextPost);
}

void switchToYellow3() {
  lightStatus3 = YELLOW;
  decrementTicker.attach_ms(decrementRateYellow, decrementVehicleCount3);
  yellowTicker.detach();
  greenTicker.attach_ms(greenDuration, switchToNextPost);
}

void switchToYellow4() {
  lightStatus4 = YELLOW;
  decrementTicker.attach_ms(decrementRateYellow, decrementVehicleCount4);
  yellowTicker.detach();
  greenTicker.attach_ms(greenDuration, switchToNextPost);
}

void decrementVehicleCount1() {
  if (vehicleCount1 > 0) vehicleCount1--;
}

void decrementVehicleCount2() {
  if (vehicleCount2 > 0) vehicleCount2--;
}

void decrementVehicleCount3() {
  if (vehicleCount3 > 0) vehicleCount3--;
}

void decrementVehicleCount4() {
  if (vehicleCount4 > 0) vehicleCount4--;
}

String createJson() {
  DynamicJsonDocument doc(1024);
  doc["vehiclecount1"] = vehicleCount1;
  doc["vehiclecount2"] = vehicleCount2;
  doc["vehiclecount3"] = vehicleCount3;
  doc["vehiclecount4"] = vehicleCount4;
  doc["timer1"] = timer1;
  doc["timer2"] = timer2;
  doc["timer3"] = timer3;
  doc["timer4"] = timer4;
  doc["lightStatus1"] = lightStatus1 == RED ? "red" : lightStatus1 == YELLOW ? "yellow" : lightStatus1 == GREEN ? "green" : "adjacent";
  doc["lightStatus2"] = lightStatus2 == RED ? "red" : lightStatus2 == YELLOW ? "yellow" : lightStatus2 == GREEN ? "green" : "adjacent";
  doc["lightStatus3"] = lightStatus3 == RED ? "red" : lightStatus3 == YELLOW ? "yellow" : lightStatus3 == GREEN ? "green" : "adjacent";
  doc["lightStatus4"] = lightStatus4 == RED ? "red" : lightStatus4 == YELLOW ? "yellow" : lightStatus4 == GREEN ? "green" : "adjacent";

  String json;
  serializeJson(doc, json);
  return json;
}

void loop() {
  server.handleClient();
}
