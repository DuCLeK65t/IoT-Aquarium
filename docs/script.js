// Sensors
//===============================================
// Get current sensor readings when the page loads
window.addEventListener('load', getReadings);

// Create Temperature Chart
var chartT = new Highcharts.Chart({
  chart: {
    renderTo: 'chart-temperature'
  },
  series: [
    {
      name: 'Humidity',
      type: 'line',
      color: '#FF0000',
      marker: {
        symbol: 'square',
        radius: 3,
        fillColor: '#FF0000',
      }
    },
    {
      name: 'Temperature',
      type: 'line',
      color: '#1B7CED',
      marker: {
        symbol: 'circle',
        radius: 3,
        fillColor: '#1B7CED',
      }
    },
  ],
  title: {
    text: undefined
  },
  time: {
    timezoneOffset: -7 * 60 //Time zone offset in minutes
  },
  xAxis: {
    type: 'datetime',
    dateTimeLabelFormats: { second: '%H:%M:%S' }
  },
  yAxis: {
    title: {
      text: 'Parameters'
    }
  },
  credits: {
    enabled: false
  }
});


//Plot temperature in the temperature chart
function plotTemperature(jsonValue) {

  var keys = Object.keys(jsonValue);
  console.log(keys);
  console.log(keys.length);

  for (var i = 0; i < keys.length; i++) {
    var x = (new Date()).getTime();
    console.log(x);
    const key = keys[i];
    var y = Number(jsonValue[key]);
    console.log(y);

    if (chartT.series[i].data.length > 40) {
      chartT.series[i].addPoint([x, y], true, true, true);
    } else {
      chartT.series[i].addPoint([x, y], true, false, true);
    }

  }
}

// Function to get current readings on the webpage when it loads for the first time
function getReadings() {
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function () {
    if (this.readyState == 4 && this.status == 200) {
      var myObj = JSON.parse(this.responseText);
      console.log(myObj);
      plotTemperature(myObj);
    }
  };
  xhr.open("GET", "/readings", true);
  xhr.send();
}

if (!!window.EventSource) {
  var source = new EventSource('/events');

  source.addEventListener('open', function (e) {
    console.log("Events Connected");
  }, false);

  source.addEventListener('error', function (e) {
    if (e.target.readyState != EventSource.OPEN) {
      console.log("Events Disconnected");
    }
  }, false);

  source.addEventListener('message', function (e) {
    console.log("message", e.data);
  }, false);

  source.addEventListener('new_readings', function (e) {
    console.log("new_readings", e.data);
    var myObj = JSON.parse(e.data);
    console.log(myObj);
    plotTemperature(myObj);
  }, false);
}

//LED
//===================================================
var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
function initWebSocket() {
  console.log('Trying to open a WebSocket connection...');
  websocket = new WebSocket(gateway);
  websocket.onopen = onOpen;
  websocket.onclose = onClose;
  websocket.onmessage = onMessage; // <-- add this line
}
function onOpen(event) {
  console.log('Connection opened');
}
function onClose(event) {
  console.log('Connection closed');
  setTimeout(initWebSocket, 2000);
}
function onMessage(event) {
  var state;
  if (event.data == "1") {
    state = "ON";
  }
  else {
    state = "OFF";
  }
  document.getElementById('state').innerHTML = state;
}
window.addEventListener('load', onLoad);
function onLoad(event) {
  initWebSocket();
  initButton();
}

function initButton() {
  document.getElementById('button').addEventListener('click', toggle);
}
function toggle() {
  websocket.send('toggle');
}