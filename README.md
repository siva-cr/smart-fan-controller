# Smart Fan Controller

Turned an old table fan into an occupancy-aware IoT smart fan using ESP8266.

## Features

* Real-time temperature monitoring
* Real-time humidity monitoring
* Occupancy detection using HC-SR04
* Automatic fan control
* AUTO / FORCE ON / FORCE OFF modes
* Mobile-friendly web dashboard
* Temperature history tracking
* Occupancy timeout for energy efficiency

## Hardware Used

* ESP8266 NodeMCU
* DHT11
* HC-SR04 Ultrasonic Sensor
* Relay Module
* Table Fan

## How It Works

The system monitors temperature and room occupancy.

The fan automatically turns ON when:

* Temperature exceeds the threshold
* A person is detected nearby

The fan automatically turns OFF when:

* Temperature falls below the threshold
* No person is detected for the occupancy timeout period

## Author

Siva C R
