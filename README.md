# BoschXDKCloudConnectivity

BoschXDKCloudConnectivity helps to connect the [Bosch XDK Cross-Domain Development Kit](http://xdk.io) to the [Bosch IoT Suite](https://www.bosch-iot-suite.com). 

## How to run it

In order to run this project, you will need:

- The official [Bosch XDK development environment](https://xdk.bosch-connectivity.com/software-downloads)
- A Bosch XDK device, registered in Bosch IoT Things [as described here](http://xdk.io/cloud)

Clone this repository and import into the Eclipse workspace as a new project, then build & flash.

## Changes compared to original version

I've taken the source code for this project from the official Bosch XDK development environment v3.1.0. 

Additional changes:
- Accelerometer events are sent out once a threshold is exceeded


