**Development of an Automation and Regulation System for Road Lighting**

**About the Author:**
Author: Maria Andreevna Terekhova
Age: 16 years old
Education: 10th grade, Specialized Educational Scientific Center of Novosibirsk State University (SESC NSU)
Achievement: The project participated in the ISSC (International Scientific Student Conference) in the "Robotics and Engineering Modeling (school section)" direction and won a prize.\

**About the Project:**
This project is an adaptive street lighting control system based on ESP microcontrollers, designed to optimize energy consumption and improve road safety.\

**The Problem**
In Russia, a significant part of roads (especially local and rural ones) lacks regular lighting. According to Rosavtodor, only about 54.6% of roads are illuminated. The lack of light is a contributing factor in 15–20% of nighttime pedestrian accidents. At the same time, there is an irrational use of electricity on low-traffic sections.

**The Goal**
To develop an automated lighting system for highways remote from the city
in areas with a low population percentage or rarely visited
roads.

**System Model Variants**
Three concepts were considered and tested during the work:

    Simple model: An ultrasonic/laser sensor measures the distance and turns on the light. Devices operate independently.

    Complex model (Mesh network): Object detection, speed measurement (via Wi-Fi signal), data packet formation, and transmission via a mesh network (without a router) to subsequent devices for proactive activation.

    Advanced model (AI): Using a camera (ESP32-CAM) and a trained neural network to recognize objects and their speed, followed by data transmission over the network.
    *(Currently, a prototype based on the first model has been implemented and brought to stable operation using FreeRTOS multitasking).*

**Hardware (1:100 Scale Model)**

The following components were used to build the model:

    Microcontroller: ESP32 (ESP32-S3 N16R8)

    Sensors:

        Ultrasonic distance sensor HC-SR04

        Laser distance sensor VL53L0X (more accurate)

        Light intensity sensor GY-302 (BH1750)

        Indication and sound: 128x64 OLED display, speaker, LEDs.

        Other: 220 Ohm resistors, connecting wires, breadboard, plywood mockup.

**Software Implementation**
The code is written in the Arduino IDE environment using FreeRTOS for ESP32.
Operating algorithm: when an object approaches closer than 40 cm, the light turns on. After the object passes, the light remains on for another 5 seconds (hold mode).
Noise reduction: a median and exponential smoothing (filtering) algorithm is implemented so that the sensor does not react to random spikes and does not cause lamp flickering. A hysteresis is also applied to confirm the state.
Multitasking: Interrupts (ISR) are used to read the echo, and a separate FreeRTOS task polls the sensor, which prevents CPU blocking.

**Future Perspectives:**

    Refining the device using more accurate components.

    Integrating the ESP32-CAM and training a neural network to detect objects in adverse weather conditions.

    Setting up a mesh network (ESP-WIFI-MESH or ESP-NOW) for synchronous operation of streetlights on the highway.

    Adapting the system for connection to real high-voltage streetlights via relays.

