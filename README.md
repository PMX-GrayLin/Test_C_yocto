# RESTful commands:

**received port : 8765**\
**sent port     : ( default localhost:7654 )**

## ✨Register Restful client to receive message from fw_daemon
```
curl http://localhost:8765/fw/restful/register/{url}/{port}
```
**ex:**
```
curl http://localhost:8765/fw/restful/register/localhost/7654
```

## ✨Device
### 🏁PWM
```
curl http://localhost:8765/fw/pwm/x/y
x = 1 / 2
y = 0 ~ 100
```

### LED
```
curl http://localhost:8765/fw/led/x/y
x = 1 ~ 5
y = green / red / orange / off
```

### DO * 2
```
curl http://localhost:8765/fw/do/x/y
x = 1 / 2
y = on / off
ex : 
curl http://localhost:8765/fw/do/1/on
```

### DI * 2
**Start Monitor DI**
```
curl http://localhost:8765/fw/di/on
```
**Stop Monitor DI**
```
curl http://localhost:8765/fw/di/off
```

**Send Event to RESTful Register Client**
1. Start Monitoring will send current status once
2. Detect high/low change
```
curl http://localhost:7654/fw/di/x/status/y
x = 1 / 2
y = high / low
```
**ex:**
```
curl http://localhost:7654/fw/di/1/status/low
```

### Triger * 2 ( behavior same as DI )
**Start Monitor Triger**
```
curl http://localhost:8765/fw/triger/on
```
**Stop Monitor Triger**
```
curl http://localhost:8765/fw/triger/off
```

**Send Event to RESTful Register Client**
1. Start monitoring will send current status once
2. Detect high/low change
```
curl http://localhost:7654/fw/triger/x/status/y
x = 1 / 2
y = high / low
```

### DIO * 4
**Should to Set to DI or DO before Use**
```
curl http://localhost:8765/fw/dio/x/set/y
x = 1 ~ 4
y = di / do
```
**If Set to DI, Start Monitor...**
**Send Event to RESTful Register Client**
1. Start monitoring will send current status once
2. Detect high/low change
```
curl http://localhost:7654/fw/dio/x/status/y
x = 1 ~ 4
y = high / low
```

**If Set to DO, Direct Control**
```
curl http://localhost:8765/fw/dio/x/do/y
x = 1 ~ 4
y = on / off
ex :
curl http://localhost:8765/fw/dio/1/do/on
```

## ✨Camera CIS ( Omnivision OG05b10 )
### Streaming
**Start**
```
curl http://localhost:8765/fw/gst/start
```
**Stop**
```
curl http://localhost:8765/fw/gst/stop
```

### Streaming status
**Request**
```
curl http://localhost:8765/fw/gst/get/isStreaming
```
**Request Return or Status Change**
```
curl http://localhost:7654/fw/gst/isStreaming/x
x = true / false
```

### Set Resolution
```
curl http://localhost:8765/fw/gst/set/resolution/width*height
ex:
curl http://localhost:8765/fw/gst/set/resolution/1920*1080
```

### Take Picture
```
curl http://localhost:8765/fw/gst/tp/x
x = encoded file path name
ex:
if save to path : /mnt/reserved/12345.png
curl http://localhost:8765/fw/gst/tp/%252Fmnt%252Freserved%252F12345.png
```

### Get exposure_time_absolute
**Request:**
```
curl http://localhost:9876/fw/gst/get/exposure_time_absolute
```
**Response:**
```
curl http://localhost:7654/fw/gst/exposure_time_absolute/value
value : 
ex: curl http://localhost:7654/fw/gst/exposure_time_absolute/1000000
```

### Get white_balance_temperature
**Request:**
```
curl http://localhost:9876/fw/gst/get/white_balance_temperature
```
**Response:**
```
curl http://localhost:7654/fw/gst/white_balance_temperature/value
value : 
ex: curl http://localhost:7654/fw/gst/white_balance_temperature/2700
```

## ✨Camera GigE
### Streaming
**Start**
```
curl http://localhost:8765/fw/gige1/start
```
**Stop**
```
curl http://localhost:8765/fw/gige1/stop
```

### Streaming status
**Request**
```
curl http://localhost:8765/fw/gige1/get/isStreaming
```
**Request Return or Status Change**
```
curl http://localhost:7654/fw/gige1/isStreaming/x
x = true / false
```

### Set Resolution
```
curl http://localhost:8765/fw/gige1/set/resolution/width*height
ex:
curl http://localhost:8765/fw/gige1/set/resolution/1920*1080
```

### Take Picture
```
curl http://localhost:8765/fw/gige1/tp/x
x = encoded file path name
ex:
if save to path : /mnt/reserved/12345.png
curl http://localhost:8765/fw/gige1/tp/%252Fmnt%252Freserved%252F12345.png
```

### Camera Configs
**Set exposure-auto**
```
curl http://localhost:8765/fw/gige1/set/exposure-auto/x
x = 0/1/2 (Auto exposure mode: 0=off, 1=once, 2=continuous)
ex :
curl http://localhost:8765/fw/gige1/set/exposure-auto/0
```

**Set exposure**
```
curl http://localhost:8765/fw/gige1/set/exposure/x
x = range of ExposureTime
  // # arv-tool-0.8 control ExposureTime
  // Hikrobot-MV-CS060-10GM-PRO-K44474092 (192.168.11.22)
  // ExposureTime = 15000 min:25 max:2.49985e+06

ex :
curl http://localhost:8765/fw/gige1/set/exposure/50000
```

**Set gain-auto**
```
curl http://localhost:8765/fw/gige1/set/gain-auto/x
x = 0/1/2 (Auto Gain mode: 0=off, 1=once, 2=continuous)
ex :
curl http://localhost:8765/fw/gige1/set/gain-auto/0
```

**Set gain**
```
curl http://localhost:8765/fw/gige1/set/gain/x
x = range of Gain
  // # arv-tool-0.8 control Gain
  // Hikrobot-MV-CS060-10GM-PRO-K44474092 (192.168.11.22)
  // Gain = 10.0161 dB min:0 max:23.9812

ex :
curl http://localhost:8765/fw/gige1/set/gain/15
```

