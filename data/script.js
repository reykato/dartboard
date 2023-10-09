var gateway = `ws://${window.location.hostname}/ws`;
  var websocket;
  window.addEventListener('load', onLoad);
  function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
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
    var eventData = event.data;
    console.log(eventData);
    //document.getElementById('state').innerHTML = eventData;
    if (eventData == 'on' || eventData == 'off') {
      document.getElementById('button').checked = (eventData == 'on');
    } else if (eventData[0] == "#") {
      document.getElementById('colorpicker').value = eventData;
    } else if (eventData[0] == "V") {
      document.getElementById('voltage').innerHTML = eventData.substring(1);
    } else {
      document.getElementById('myRange').value = eventData;
    }
  }
  function onLoad(event) {
    initWebSocket();
    initButton();
  }
  function initButton() {
    document.getElementById('button').addEventListener('click', sendMessage.bind(null, 'toggle'));
    document.getElementById('colorpicker').addEventListener('input', onHueChange);
    document.getElementById('white').addEventListener('click', sendMessage.bind(null, 'white'));
    document.getElementById('hue').addEventListener('click', sendMessage.bind(null, 'hue'));
    document.getElementById('rainbow').addEventListener('click', sendMessage.bind(null, 'rainbow'));
    document.getElementById('pulse').addEventListener('click', sendMessage.bind(null, 'pulse'));
    document.getElementById('myRange').addEventListener('input', onBrightnessChange);
  }
  function sendMessage(msg) {
    console.log(msg);
    websocket.send(msg);
  }
  function onHueChange() {
    var color = document.getElementById('colorpicker').value;
    console.log(color);
    websocket.send(color);
  }
  function onBrightnessChange() {
    var brightness = document.getElementById('myRange').value;
    console.log(brightness);
    websocket.send(brightness);
  }