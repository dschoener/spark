# Firmware project Spark - based on Mongoose OS

## Overview

This is the Mongoose OS app Spark.

## How to build this app

The required mongoose-os version is 1.22 now!
```
# mos --arch esp8266
```

## How to install this app

- Install and start [mos tool](https://mongoose-os.com/software.html)
- Switch to the Project page, find and import this app, build and flash it:

<p align="center">
  <img src="https://mongoose-os.com/images/app1.gif" width="75%">
</p>

## Setup device side
Setup WiFi:

```
# mos wifi WIFI_NETWORK_NAME WIFI_PASSWORD
```

Register device on Google IoT Core. If a device is already registered, this command deletes it, then registers again:

```
# mos gcp-iot-setup --gcp-project YOUR_PROJECT_NAME --gcp-region europe-west1 --gcp-registry iot-registry
```