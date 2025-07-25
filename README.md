# RESTful commands:

**received port : 8765**
**sent port     : ( default localhost:7654 )**

## Register to receive message from fw_daemon
```
curl http://localhost:8765/fw/register/{url}/{port}
```
**ex:**
```
curl http://localhost:8765/fw/register/localhost/7654
```

## Camera CIS ( Omnivision OG05b10 )
### Streaming



## Camera GigE
### Streaming
**Start**
```
curl http://localhost:8765/fw/gige1/start
```
**Stop**
```
curl http://localhost:8765/fw/gige1/stop
```

## Streaming status
$ get
curl http://localhost:8765/fw/gige1/get/isStreaming
$ return or status change
ex:
curl http://localhost:7654/fw/gige1/isStreaming/x
x = true / false

# set streaming resolution
curl http://localhost:8765/fw/gige1/set/resolution/w*h
ex:
curl http://localhost:8765/fw/gige1/set/resolution/1920*1080

# Take Picture
curl http://localhost:8765/fw/gst/tp/x
x = encoded file path name

# PWM
curl http://localhost:8765/fw/pwm/x/y
x = 1 / 2
y = 0 ~ 100

# LED
curl http://localhost:8765/fw/led/x/y
x = 1 ~ 5
y = green/red/orange/off

# DO * 2
curl http://localhost:8765/fw/do/x/y
x = 1 / 2
y = on / off
ex : 
curl http://localhost:8765/fw/do/1/on

# DI * 2
curl http://localhost:8765/fw/di/on
curl http://localhost:8765/fw/di/off
start/stop thread to monitor DI

* detect DI high/low change >> send out RESTful 
curl http://localhost:7654/fw/di/x/status/y
x = 1 / 2
y = high / low

ex :
curl http://localhost:7654/fw/di/1/status/low

# Triger * 2 ( behavior same as DI )
curl http://localhost:8765/fw/triger/on
curl http://localhost:8765/fw/triger/off
start/stop thread to monitor Triger

* detect Triger high/low change >> send out RESTful 
curl http://localhost:7654/fw/triger/x/status/y
x = 1 / 2
y = high / low

# DIO * 4
curl http://localhost:8765/fw/dio/x/set/y
x = 1 ~ 4
y = di / do

* set to DI, auto start monitor DI
* detect DI high/low change >> send out RESTful 
curl http://localhost:7654/fw/dio/x/status/y
x = 1 ~ 4
y = high / low

* set to DO, control
curl http://localhost:8765/fw/dio/x/do/y
x = 1 ~ 4
y = on / off
ex :
curl http://localhost:8765/fw/dio/1/do/on

# Camera Configs
* set exposure-auto
curl http://localhost:8765/fw/gige1/set/exposure-auto/x
x = 0/1/2 (Auto exposure mode: 0=off, 1=once, 2=continuous)
ex :
curl http://localhost:8765/fw/gige1/set/exposure-auto/0

* set exposure
curl http://localhost:8765/fw/gige1/set/exposure/x
x = range of ExposureTime
  // # arv-tool-0.8 control ExposureTime
  // Hikrobot-MV-CS060-10GM-PRO-K44474092 (192.168.11.22)
  // ExposureTime = 15000 min:25 max:2.49985e+06

ex :
curl http://localhost:8765/fw/gige1/set/exposure/50000

* set gain-auto
curl http://localhost:8765/fw/gige1/set/gain-auto/x
x = 0/1/2 (Auto Gain mode: 0=off, 1=once, 2=continuous)
ex :
curl http://localhost:8765/fw/gige1/set/gain-auto/0

* set gain
curl http://localhost:8765/fw/gige1/set/gain/x
x = range of Gain
  // # arv-tool-0.8 control Gain
  // Hikrobot-MV-CS060-10GM-PRO-K44474092 (192.168.11.22)
  // Gain = 10.0161 dB min:0 max:23.9812

ex :
curl http://localhost:8765/fw/gige1/set/exposure/15


# get exposure_time_absolute
request:
curl http://localhost:9876/fw/gst/get/exposure_time_absolute

response:
curl http://localhost:7654/fw/gst/exposure_time_absolute/value
value : 
ex: curl http://localhost:7654/fw/gst/exposure_time_absolute/1000000

# get white_balance_temperature
request:
curl http://localhost:9876/fw/gst/get/white_balance_temperature

response:
curl http://localhost:7654/fw/gst/white_balance_temperature/value
value : 
ex: curl http://localhost:7654/fw/gst/white_balance_temperature/2700
