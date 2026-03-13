# Nightscout-lamp
A simple way to display blood sugar using nightscout and an RGB LED. 

This project covers creation of a simple glucose level indicator using Nightscout in conjunction with an ESP32 family microcontroller and a ws2812b RGB led(s).  
  
If you prefer something of the shelf consider the glowcose or sugarpixel.  

This example is configured by default with 5 static colours that map to 5 distinct glucose ranges as below, the colours and ranges can be set to your preference:  
  
- Off - no data / error.  
- 0 - 3.9 mmol/l - blue i.e. blue is cold / low glucose (at least how I see it in my mind).  
- \> 3,9 and < 5 mmol/l purple - lower range  
- \> 5 and < 8 mmol/l green - i.e. in range.  
- \> 8 and < 10 mmol/l orange upper range.  
- \> 10 red - high i.e. red hot.  
  
Separate intensity ranges are available for day and night, each intensity and time when applied is adjustable.  

### Dependencies 
This project is dependant on the following Arduino libraries which need to be installed before compiling:

 - [ArduinoJson](https://arduinojson.org/v7/how-to/install-arduinojson/)
 - [Adafruit_NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel)

### Basic Configuration  

To get started configure the source with the following minimum parameters unique to your environment, build and upload to an ESP32 family microcontroller:  
  
- Your wifi SSID / Password.  
- Your Nightscout url and API code.  
- Pin which LED is connected.
- Number of LEDs in use. i.e. single led or neopixel strip etc.  
- Your local timezone.  

#### Other Customizable attributes:

In addition to the minimum required parameters the following can also be customized:  
  
- dayStart = 0800 - Time day brightness is applied, hours followed by minutes 0800 is 8AM - 0830 would be 8:30AM.  
- dayEnd = 2100 - Time night brightness is applied in 24 hour time 2100 - 9PM, 2130 would be 9:30PM.  
- dayBrightness - Brightness level applied during the day 0 - 100% - Caution, using high brightness can cause damage in some instances if adequate heat sinking is not provided. 
- nightBrightness.
- low, lowerInRange, inRange, higherInRange, high = specific threshold and RGB values to form the colour for each of the glucose ranges. Note none of this is validated, it is up to you to make sure the thresholds make sense and the RGB values are correct otherwise you will get unexpected results.  
- muteTime in minutes, default 0.  
	- If muteTime is > 0 mute button is enabled using the pin defined by muteButton .  
	- If enabled, when the mute button is pressed (pin grounded) the LED is set to a low intensity warm white for the configured time. This option is intended as a way to "mute" the light if you have already treated a high / low glucose event and don't need further reminders.  
	- muteButton - pin which mute button is connected, disabled by default.   
	- muteColour - RGB values used for muteColour, warm white by default. 
	- muteBrightness - 0 - 100% - same value as nightBrightness by default. 
