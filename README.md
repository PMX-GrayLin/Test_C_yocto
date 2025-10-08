# RESTful commands:
zxc
## <a name="anchor-topic"> <ins>Topic</ins>: </a>
[RESTful](#anchor-restful)\
[Device](#anchor-device)\
[Camera CIS ](#anchor-cam-cis)\
[Camera UVC ](#anchor-cam-uvc)\
[Camera Gige ](#anchor-cam-gige)

## ✨<a name="anchor-restful">Register Restful Client to Receive Message (from fw_daemon)</a>
**received port : 8765**\
**sent port     : ( default localhost:7654 )**
```
curl http://localhost:8765/fw/restful/register/{url}/{port}
```
**ex:**
```
curl http://localhost:8765/fw/restful/register/localhost/7654
```

## <a name="anchor-device"> ✨Device </a> <sub> [Back to Top](#anchor-topic) </sub>
### 🏁PWM
```
curl http://localhost:8765/fw/pwm/x/y
x = 1 / 2
y = 0 ~ 100
```

### 🏁LED
```
curl http://localhost:8765/fw/led/x/y
x = 1 ~ 5
y = green / red / orange / off
```

### 🏁DO * 2
```
curl http://localhost:8765/fw/do/x/y
x = 1 / 2
y = on / off
ex : 
curl http://localhost:8765/fw/do/1/on
```

### 🏁DI * 2
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

### 🏁Triger * 2 ( behavior same as DI )
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

### 🏁DIO * 4
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

## ✨<a name="anchor-cam-cis"> Camera CIS ( Omnivision OG05b10 ) </a> <sub>[Back to Top](#anchor-topic) </sub>
### 🏁Streaming
**Start**
```
curl http://localhost:8765/fw/gst/start
```
**Stop**
```
curl http://localhost:8765/fw/gst/stop
```

**Request Streaming status**
```
curl http://localhost:8765/fw/gst/get/isStreaming
```
**Request Return or Status Change**
```
curl http://localhost:7654/fw/gst/isStreaming/x
x = true / false
```

### 🏁Set Resolution
will effect next streaming
```
curl http://localhost:8765/fw/gst/set/resolution/width*height
ex:
curl http://localhost:8765/fw/gst/set/resolution/1920*1080
```

### 🏁Take Picture
```
curl http://localhost:8765/fw/gst/tp/x
x = encoded file path name
ex:
if save to path : /mnt/reserved/12345.png
curl http://localhost:8765/fw/gst/tp/%252Fmnt%252Freserved%252F12345.png
```

### 🏁Get exposure_time_absolute
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

### 🏁Get white_balance_temperature
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

## ✨<a name="anchor-cam-uvc">UVC Cameras </a> <sub>[Back to Top](#anchor-topic)</sub>
### 🏁Streaming
**Start**
```
curl http://localhost:8765/fw/uvc/start
```
**Stop**
```
curl http://localhost:8765/fw/uvc/stop
```

**Request Streaming status**
```
curl http://localhost:8765/fw/uvc/get/isStreaming
```
**Request Return or Status Change**
```
curl http://localhost:7654/fw/uvc/isStreaming/x
x = true / false
```

### 🏁Set Resolution
will effect next streaming
```
curl http://localhost:8765/fw/uvc/set/resolution/width*height
ex:
curl http://localhost:8765/fw/uvc/set/resolution/1920*1080
```

### 🏁Take Picture
```
curl http://localhost:8765/fw/uvc/tp/x
x = encoded file path name
ex:
if save to path : /mnt/reserved/12345.png
curl http://localhost:8765/fw/uvc/tp/%252Fmnt%252Freserved%252F12345.png
```

### 🏁Get exposure_time_absolute
**Request:**
```
curl http://localhost:9876/fw/uvc/get/exposure_time_absolute
```
**Response:**
```
curl http://localhost:7654/fw/uvc/exposure_time_absolute/value
value : 
ex: curl http://localhost:7654/fw/uvc/exposure_time_absolute/1000000
```

### 🏁Get white_balance_temperature
**Request:**
```
curl http://localhost:9876/fw/uvc/get/white_balance_temperature
```
**Response:**
```
curl http://localhost:7654/fw/uvc/white_balance_temperature/value
value : 
ex: curl http://localhost:7654/fw/uvc/white_balance_temperature/2700
```

## ✨<a name="anchor-cam-gige"> Camera GigE </a> <sub>[Back to Top](#anchor-topic)</sub>
Max number of GigE camera is 2 \
curl http://localhost:8765/fw/{gige-index}/... \
{gige-index} can be **gige1** or **gige2**

### 🏁Streaming
**Start**
```
curl http://localhost:8765/fw/{gige-index}/start
```
**Stop**
```
curl http://localhost:8765/fw/gige1/stop
```

**Request Streaming status**
```
curl http://localhost:8765/fw/{gige-index}/get/isStreaming
```
**Request Return or Status Change**
```
curl http://localhost:7654/fw/gige1/isStreaming/x
x = true / false
```

### 🏁Set Resolution
will effect next streaming
```
curl http://localhost:8765/fw/{gige-index}/set/resolution/width*height
ex:
curl http://localhost:8765/fw/gige1/set/resolution/1920*1080
```

### 🏁Take Picture
```
curl http://localhost:8765/fw/{gige-index}/tp/x
x = encoded file path name
ex:
if save to path : /mnt/reserved/12345.png
curl http://localhost:8765/fw/gige1/tp/%252Fmnt%252Freserved%252F12345.png
```

### 🏁Camera Configs
**Set exposure-auto**
```
curl http://localhost:8765/fw/{gige-index}/set/exposure-auto/x
x = 0/1/2 (Auto exposure mode: 0=off, 1=once, 2=continuous)
ex :
curl http://localhost:8765/fw/gige1/set/exposure-auto/0
```

**Set exposure**
```
curl http://localhost:8765/fw/{gige-index}/set/exposure/x
x = range of ExposureTime
  // # arv-tool-0.8 control ExposureTime
  // Hikrobot-MV-CS060-10GM-PRO-K44474092 (192.168.11.22)
  // ExposureTime = 15000 min:25 max:2.49985e+06

ex :
curl http://localhost:8765/fw/gige1/set/exposure/50000
```

**Set gain-auto**
```
curl http://localhost:8765/fw/{gige-index}/set/gain-auto/x
x = 0/1/2 (Auto Gain mode: 0=off, 1=once, 2=continuous)
ex :
curl http://localhost:8765/fw/gige1/set/gain-auto/0
```

**Set gain**
```
curl http://localhost:8765/fw/{gige-index}/set/gain/x
x = range of Gain
  // # arv-tool-0.8 control Gain
  // Hikrobot-MV-CS060-10GM-PRO-K44474092 (192.168.11.22)
  // Gain = 10.0161 dB min:0 max:23.9812

ex :
curl http://localhost:8765/fw/gige1/set/gain/15
```

### 🏁Trigger Mode:
**On**
```
curl http://localhost:8765/fw/gige1/set/trigger-mode/on
```
**Off**
```
curl http://localhost:8765/fw/gige1/set/trigger-mode/off
```

### 🏁Trigger Mode Image:
**path & naming prefix**
```
curl http://localhost:8765/fw/gige1/set/imagePathPrefix/x
x : full saved path & image prefix (double encode)
ex : "%252Fhome%252Froot%252Fprimax%252FTest_Workstation"
```

**naming index**
```
curl http://localhost:8765/fw/gige1/set/imageMaxIndex/x
x = auto save image post index;
ex: x = 4, prefix = "/home/root/primax/Test_Workstation" (decoded)
loop saved image will be /home/root/primax/Test_Workstation_1~4.png 
```

### 🏁Trigger Mode PWM:
**Trigger Mode Linked PWM Value**

```
curl http://localhost:8765/fw/pwmTrigger/x/y
x = 1 or 2
y = 0~100 ( default : 50 )
```


