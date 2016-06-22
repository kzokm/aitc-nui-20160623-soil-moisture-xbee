require 'date-utils'

XBEE_API_MODE = true
XBEE_TARGET_ADDRESS = 'a258'
SERIAL_PORT = 'COM10'

XBeeAPI = require 'xbee-api'
XBee = XBeeAPI.constants
xbee = new XBeeAPI.XBeeAPI
  api_mode: 1

xbee.on 'error', (e)->
  console.log 'error: xbee: ', e

serialport = require 'serialport'
com = new serialport.SerialPort SERIAL_PORT,
  baudRate: 57600
  parser: xbee.rawParser()
  #parser: serialport.parsers.readline '\r\n'

com.on 'error', (e)->
  console.log 'error: serialport: ', e

plotly = (require 'plotly')
  username: process.env.PLOTLY_USERNAME
  apiKey: process.env.PLOTLY_API_KEY
MAX_STREAM_POINTS = 60 * 24 * 60

graph_options =
  fileopt: 'extend'
  filename: 'soil-moisture'
  layout:
    title: 'Soil Moisture'
    xaxis:
      title: 'date'
      type: 'date'
      autorange: true
    yaxis:
      title: 'moisture'
      titlefont: color: "#1f77b4"
      tickfont: color: "#1f77b4"
      range: [0, 800]
    yaxis2:
      title: 'temperature'
      titlefont: color: "#ff7f0e"
      tickfont: color: "#ff7f0e"
      range: [0, 42]
      overlaying: 'y'
      side: 'left'
      anchor: 'free'
      position: 0.05
    yaxis3:
      title: 'battery voltage'
      titlefont: color: "#d62728"
      tickfont: color: "#d62728"
      range: [2.0, 4.6]
      overlaying: 'y'
      side: 'right'

init_data = [
  x: []
  y: []
  name: 'moisture'
  stream:
    token: 'ilzbljmark'
    maxpoints: MAX_STREAM_POINTS
,
  x: []
  y: []
  name: 'temperature'
  yaxis: 'y2'
  stream:
    token: 'kfasb9s13j'
    maxpoints: MAX_STREAM_POINTS
,
  x: []
  y: []
  name: 'battery voltage'
  yaxis: 'y3'
  stream:
    token: 'yo45ch66ua'
    maxpoints: MAX_STREAM_POINTS
]

plotly.plot init_data, graph_options, (err, msg)->
  throw err if err

  create_stream = (token, failover)->
    stream = plotly.stream token, (err, res)->
      if err
        console.log err
        clearInterval timer
        stream.end ->
        failover create_stream token, failover
    timer = setInterval health_check(stream), 50000
    stream

  health_check = (stream)->
    -> stream.write '\r\n'

  stream_moisture = create_stream init_data[0].stream.token, (stream)->
    console.log 'reconnect stream_moisutre'
    stream_moisture = stream
  stream_temperature = create_stream init_data[1].stream.token, (stream)->
    console.log 'reconnect stream_temperature'
    stream_temperature = stream
  stream_battery = create_stream init_data[2].stream.token, (stream)->
    console.log 'reconnect stream_battery'
    stream_battery = stream

  com.on 'data', (buffer)->
    parseReceivedData buffer

  xbee.on 'frame_object', (frame)->
    #console.log 'frame>', frame
    if frame.type == XBee.FRAME_TYPE.ZIGBEE_RECEIVE_PACKET
      if frame.remote16 == XBEE_TARGET_ADDRESS
        prepareReceivedData frame.data
    return

  previous_data = ''

  prepareReceivedData = (buffer)->
    data = previous_data += buffer.toString()
    if (p = data.indexOf '\n') >= 0
      previous_data = data.substring p + 1
      parseReceivedData data.substring 0, p
    return

  parseReceivedData = (data)->
    now = new Date()
      .toFormat 'YYYY-MM-DD HH24:MI:SS'
    console.log now, data
    try
      data = JSON.parse data
    catch
      return console.log 'error: JSON.parse'

    stream_object = JSON.stringify
      x: now
      y: data.moisture.value
    stream_moisture.write stream_object + '\r\n'

    stream_object = JSON.stringify
      x: now
      y: data.temperature.value
    stream_temperature.write stream_object + '\r\n'

    stream_object = JSON.stringify
      x: now
      y: data.battery.value
    stream_battery.write stream_object + '\r\n'
