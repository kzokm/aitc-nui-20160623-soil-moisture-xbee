require 'date-utils'
serialport = require 'serialport'

com = new serialport.SerialPort 'COM10',
  baudRate: 57600
  parser: serialport.parsers.readline '\r\n'

com.on 'data', (buffer)->
  now = new Date()
    .toFormat 'YYYY-MM-DD HH24:MI:SS'
  console.log now, buffer
  try
    console.log JSON.parse buffer
  catch
    return console.log 'error'

com.on 'error', (e)->
  console.log 'error>', e
