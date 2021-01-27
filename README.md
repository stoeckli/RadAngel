# RadAngel Display
M5Stack core code to display gamma spectroscopy data from a (now discontinued) [Kromek](https://www.kromek.com/)(TM) [RadAngel](https://www.kromek.com/news/radangel-czt-based-gamma-ray-detector-now-in-stock/) detector.

## Idea
The idea behind this project is to create a simple display to show gamma spectral data from the Kromek RadAngel detector:

![hello](https://github.com/stoeckli/RadAngel/blob/master/RadAngel%20Display.jpg)

## Implementation
A M5Stack core unit (basic, gray or fire) is attached to a M5Stack USB Module. Using the [USB Host Shield 2.0 library] (https://github.com/felis/USB_Host_Shield_2.0) to capture the HID data.
