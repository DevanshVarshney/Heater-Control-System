# Heater-Control-System

This repository contains simulation links and code for an Automatic Heater Control System.  
Created as an assignment for the upliance.ai Embedded Systems Internship.

---

### Note:
1. BLE cannot be simulated on Wokwi. To test BLE functionality, hardware implementation is required.
2. `main.cpp` contains the code **without** BLE support.
3. `main(BLE).cpp` contains the code **with** BLE support.

---

### Wokwi Simulation Link:
https://wokwi.com/projects/430502347775092737
---

### Components Used:
- DS18B20 Temperature Sensor  
- ESP32  
- Servo Motor  
- LED  
- 4x4 Keypad  
- Resistors  

---

### Variables:
- **Target Temperature**: Temperature set by the user using the keypad.
- **MAX_TEMP**: Temperature above which overheating is detected.

---

### How to Use This Repository:
1. Download the ZIP or clone the repository.
2. Open the folder in PlatformIO (VS Code) and initialize it as a PlatformIO project.
3. Click **Build** to compile the code.
4. Click **Upload** to flash the code to your ESP32.
5. Use the **Serial Monitor** to view real-time debug messages and instructions.

---

### Simulation Instructions (using Wokwi Link):
1. Open the simulation link provided above.
2. Enter the **Target Temperature** using the keypad.
3. Follow the instructions shown on the Serial Monitor.
4. Based on the temperature and input, the system will behave as follows:

#### System States:

| State           | Conditions                                      | Action                                  |
|----------------|--------------------------------------------------|-----------------------------------------|
| **HEATING**     | Target temperature entered, sensor temp < target | LED ON, Servo rotates                   |
| **OVERHEATING** | Sensor temp > MAX_TEMP (e.g., >100°C)            | LED OFF, Servo rotates to 0             |
| **TARGET_REACHED** | Temp within target ± threshold                 | LED OFF, Servo rotates to 0             |
| **IDLE**        | No target temp set or sensor temp > target      | LED OFF, Servo rotates to 0             |
| **STABILIZING** | Temp near target but still adjusting            | LED/Servo state depends on temperature  |

---

Feel free to test the simulation or flash it to actual hardware
