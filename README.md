Project Water Activity Meter

An ESP32-based Water Activity (aw) Meter that estimates water activity from relative humidity (%RH) and sample temperature. The system includes thermoelectric temperature control, TFT and LCD displays, RGB status indication, local push-button control, and a Wi-Fi Access Point with an integrated Web Dashboard.


Main firmware: 3310.inoThis project is intended for experimental and prototype applications. Measurements should be verified against a suitable reference instrument before being used for official reporting, regulatory compliance, or product-release decisions.

Features

•
Measures water activity (aw) from relative humidity

•
Supports the following humidity sensors:

•
SHT45 over I2C as the primary sensor

•
DHT22/DHT11 over a single-wire GPIO connection as a fallback sensor



•
Measures sample temperature using a DS18B20 over the 1-Wire bus

•
Controls a thermoelectric cooler and fan using PWM

•
Maintains the target temperature using PID control

•
Provides PID Auto-Tune functionality

•
Displays measurement data on a TFT display and a 16x2 I2C LCD

•
Provides a local menu system for:

•
Standard aw measurement

•
Equilibrium aw prediction

•
A/B sample comparison

•
Measurement-record management

•
Wi-Fi information

•
System health information



•
Uses an RGB LED to indicate measurement stability:

•
Red: measurement is still changing

•
Yellow: measurement is becoming stable

•
Green: measurement is stable and may be saved



•
Provides graphs and data export through the Web Dashboard

•
Displays a QR code for Wi-Fi connection

•
Includes I2C bus recovery and sensor fault handling

•
Rejects invalid DS18B20 readings, including the 85 °C startup value and abnormal temperature spikes

•
Stores selected settings and operating data in ESP32 NVS

•
Supports RAW and CALIBRATED aw calculation modes

aw Calculation Modes

The calculation mode is selected in 3310.ino:

C++


#define AW_RAW_MODE 0



Value
Mode
Description
0
CALIBRATED
Uses the piecewise-linear calibration table and temperature compensation
1
RAW
Calculates aw = %RH / 100 directly without applying the calibration table




The RAW mode is intended for raw-sensor evaluation and comparison with another firmware version. The CALIBRATED mode should be used only after the instrument has been calibrated against an appropriate reference.

Hardware and Wiring

Device
Device Pin
ESP32 GPIO
Notes
SHT45
SDA
GPIO21
Shares the I2C bus with the LCD
SHT45
SCL
GPIO22
The firmware checks I2C addresses 0x44 and 0x45
16x2 I2C LCD
SDA/SCL
GPIO21/GPIO22
The default address in the firmware is 0x27
DHT22/DHT11
DATA
GPIO15
Fallback humidity sensor; select the sensor type using DHT_TYPE
DS18B20
DQ
GPIO13
A 4.7 kΩ pull-up resistor is recommended between DQ and 3.3 V
UP button
Switch to GND
GPIO32
Configured with INPUT_PULLUP
DOWN button
Switch to GND
GPIO33
Configured with INPUT_PULLUP
KY-016 RGB LED
Red
GPIO25
Confirm whether the module is common-cathode or common-anode
KY-016 RGB LED
Green
GPIO26


KY-016 RGB LED
Blue
GPIO27


RB046 driver module
TRIG/PWM
GPIO17
Controls the thermoelectric cooler and fan together
RB046 driver module
GND
GND
Must share a common ground with the ESP32




Power Supply Requirements

•
The thermoelectric cooler and fan should be powered by a suitable external supply; they must not be powered directly from the ESP32 3.3 V rail.

•
The ESP32, sensors, and driver module must share a correct common ground.

•
The power supply must support the current required by the thermoelectric cooler and fan, including startup transients.

•
If the ESP32 resets when Wi-Fi or the thermoelectric cooler is enabled, inspect the power supply, ground wiring, and transient current before changing firmware parameters.

Required Libraries

Install the following libraries through the Arduino IDE Library Manager:

•
TFT_eSPI

•
DHT sensor library by Adafruit

•
Adafruit Unified Sensor

•
LiquidCrystal_I2C

•
OneWire

•
DallasTemperature

The following components are provided by the ESP32 Arduino core or standard Arduino environment:

•
WiFi.h

•
WebServer.h

•
SPI.h

•
Wire.h

•
Preferences.h

•
esp_task_wdt.h

•
esp_system.h

•
esp_wifi.h

qrcodegen Files

The following files must be placed in the same sketch directory as 3310.ino:

Plain Text


qrcodegen.c
qrcodegen.h



These files are from 

Nayuki's QR-Code-generator project. Use the C implementation. The files do not need to be installed through the Arduino IDE Library Manager.

Arduino IDE Configuration

1.
Install the Arduino IDE and add the ESP32 board package provided by Espressif Systems.

2.
Select the ESP32 board corresponding to the hardware in use, for example ESP32 Dev Module.

3.
Configure TFT_eSPI to match the installed TFT display by editing User_Setup.h or the selected TFT_eSPI setup file.

4.
Create a sketch directory containing the following files:

Plain Text


3310/
├── 3310.ino
├── qrcodegen.c
└── qrcodegen.h





5.
Open 3310.ino in the Arduino IDE.

6.
Review the following settings before uploading:

•
AW_RAW_MODE

•
wifiPassword

•
DHT_TYPE

•
SHT_MINUS_DS_OFFSET_C

•
TARGET_TEMP_C



7.
Select the ESP32 serial port.

8.
Click Verify to compile the firmware.

9.
Click Upload to upload the firmware to the board.

Wi-Fi and Web Dashboard

After startup, the ESP32 creates a Wi-Fi Access Point using the following settings in the firmware:

C++


const char* ssid = "AW_Meter";
char wifiPassword[13] = "aw12345678";



To access the Web Dashboard:

1.
Connect a phone or computer to the AW_Meter Wi-Fi network.

2.
Enter the password defined in wifiPassword.

3.
Open the following address in a web browser:

Plain Text


http://192.168.4.1/





4.
Alternatively, open WiFi Info on the device to view the connection information and scan the displayed QR code.


Change the default password before deploying the device. WPA2 passwords must contain at least eight characters.

Local User Interface

•
Press UP or DOWN to move through menu items.

•
Hold either button for approximately 500 ms to select an item.

•
Press UP and DOWN simultaneously to go back.

The main menu contains:

Plain Text


1. Measure AW
2. Recording
3. WiFi Info
4. System Health



The measurement menu contains:

Plain Text


1.1 Start     Standard aw measurement
1.2 Predict   Equilibrium aw prediction
1.3 Compare   A/B sample comparison
1.4 Cancel    Cancel the current measurement mode



Typical Measurement Procedure

1.
Power on the device without placing a sample on the sensor.

2.
Allow the system to detect the sensors and capture the ambient reference conditions during startup.

3.
Place the sample in the measurement position.

4.
Select Measure AW > Start.

5.
The firmware performs sensor conditioning. This may include activating the SHT45 heater and waiting for the sensor to cool.

6.
Wait for the measurement to reach a stable condition. The RGB LED changes from red to yellow and then green as stability improves.

7.
Save the result when the on-screen confirmation prompt appears.

8.
After a measurement, the firmware may activate the SHT45 heater to remove residual moisture before returning to the menu.

Calibration and Temperature Compensation

In CALIBRATED mode, the firmware can use:

•
A piecewise-linear calibration table

•
Quick Calibration using an external reference value

•
Compensation for the temperature difference between the SHT45 sensor and the sample measured by the DS18B20

•
Multi-round automatic calibration through the administrator functions in the Web Dashboard

•
PID Auto-Tune for estimating suitable Kp, Ki, and Kd values

The following parameter requires particular attention:

C++


float SHT_MINUS_DS_OFFSET_C = 0.0f;



To determine a suitable offset, place the device in a room with stable temperature, disable the heater and thermoelectric cooler, and allow the sensors to stabilize for approximately 30 minutes. Then open System Health, read the SHT-sample dT value, and use an appropriate measured offset instead of 0.0f.

Troubleshooting

Compilation Error: state was not declared in this scope

Ensure that a forward declaration is placed before any function that references state:

C++


extern AppState state;



The actual variable definition must appear exactly once, for example:

C++


AppState state = ST_BOOT_WARMUP;



LiquidCrystal_I2C Architecture Warning

The Arduino IDE may display a warning similar to:

Plain Text


library LiquidCrystal_I2C claims to run on all architecture(s )



This is generally a library metadata warning rather than a compilation error. If compilation succeeds and the LCD operates correctly, the warning can usually be ignored. Nevertheless, use a LiquidCrystal_I2C library that is known to work with ESP32 and verify the LCD I2C address.

SHT45 Reading Failure

•
Verify SDA on GPIO21 and SCL on GPIO22.

•
Verify the sensor power supply and ground connection.

•
Check I2C address 0x44 or 0x45.

•
Keep I2C wiring short and away from thermoelectric cooler power and PWM wiring.

•
If using the DHT fallback sensor, verify DHT_PIN and DHT_TYPE.

DS18B20 Fault or 85 °C Reading

•
Verify the data line on GPIO13.

•
Install a 4.7 kΩ pull-up resistor.

•
Verify the sensor power and ground connections.

•
Keep the sensor cable away from high-current and PWM wiring.

ESP32 Resets When Wi-Fi or the Thermoelectric Cooler Starts

•
Use a power supply with sufficient current capacity.

•
Power the thermoelectric load separately from the ESP32 supply.

•
Confirm that all devices share a correct common ground.

•
Verify the polarity and wiring of the RB046 driver module.

•
Reduce Wi-Fi transmit power if the power supply experiences transient voltage drops.

Corrupted LCD Characters

•
Verify that the LCD address is 0x27, unless the hardware uses another address.

•
Check SDA, SCL, and ground wiring.

•
Reduce the I2C cable length.

•
The firmware includes SafeLCD, ACK monitoring, and I2C bus recovery; however, these functions cannot correct an incorrect wiring configuration or an inadequate power supply.

Web API

The firmware includes the following principal endpoints:

Endpoint
Description
/
Web Dashboard
/data
Current measurement data and device status in JSON format
/info
Device information and metadata
/calpoints
Read the current calibration points
/calset
Set calibration points
/calreset
Restore the default calibration points
/calquick
Perform Quick Calibration using a reference value
/pidset
Set PID parameters
/pidautotune
Start or cancel PID Auto-Tune
/pidautotune/status
Read the PID Auto-Tune status
/adv/status
Read the ADV module status
/adv/set
Configure the ADV module
/clocksync
Synchronize the device clock with the browser
/setop
Set the current operator identifier




Calibration and administrator endpoints should only be used on a trusted network. The firmware does not provide the same security controls as a production web service.

Project Files

Plain Text


project-Water-activity/
├── 3310.ino       # Main ESP32 firmware
├── qrcodegen.c     # QR Code generator implementation
├── qrcodegen.h     # QR Code generator header
└── README.md       # Project documentation



Project Status

This project is under active development. Measurement stability, temperature control, calibration, and fault recovery continue to be refined. Several constants and thresholds are exposed in the firmware so they can be adjusted for the specific sensors, power supply, enclosure, and operating environment.

License

No formal project license has been specified at this time. If the project is redistributed or used as the basis for another project, add a LICENSE file and clearly define the applicable terms of use.

