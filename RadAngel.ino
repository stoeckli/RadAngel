// stoeckli.net
// display gamma spectral data from RadAngel device
// required hardware: M5Stack ESP32 (GRAY, Basic, Fire), USB Module with MAX3421E
// V1.0, Jan 2012

#include <M5Stack.h>
#include <usbhid.h>
#include <hiduniversal.h>
#include <hidescriptorparser.h>
#include <math.h>
#include "bitmap.h"

//buttons
#define ButtonA 39
#define ButtonB 38
#define ButtonC 37

// LCD data
// colors
#define c_lightblue 0x051d
#define c_white 0xffff
#define c_black 0x0000
#define c_gray 0x7bef

// LCD dimensions
#define d_width 320
#define d_hight 240
#define d_border_left 40
#define d_border_bottom 40
#define d_border_top 2
#define d_border_right 2

// graphics dimensions
const int g_width = d_width - d_border_left - d_border_right - 2;
const int g_hight = d_hight - d_border_top - d_border_bottom - 2;
const int g_left = d_border_left + 1;
const int g_bottom = d_hight - d_border_bottom - 1;
const int g_top = d_border_top + 1;
const int g_right = d_width - d_border_right - 1;
const float g_x_max = 1500;
const float g_y_max = 4;
const float g_x_ppu = g_width / g_x_max;
const float g_y_ppu = g_hight / g_y_max * -1.0;
const int g_pointer = 0;

// RadAngel channel data
#define ChOffset 362
#define ChRise 1.0
#define ChMax 1500.0

// 12-bit spectral data
int Channels[4096];

// histogram for display
int Col = 0;
int Cols[g_width];
int DisplayMode = 0;
#define DispMeasuring 1
#define DispInfo 2
#define DispReset 3

// SD Card
int SDpresent = true;


// HID Class 
class HIDUniversal2 : public HIDUniversal
{
public:
    HIDUniversal2(USB *usb) : HIDUniversal(usb) {};

protected:
    uint8_t OnInitSuccessful();
    void ParseHIDData(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf);
};

// handler for init of USB device
uint8_t HIDUniversal2::OnInitSuccessful()
{
    uint8_t    rcode = 0;
    HexDumper<USBReadParser, uint16_t, uint16_t>    Hex;
    ReportDescParser                                Rpt;

    if ((rcode = GetReportDescr(0, &Hex)))
        goto FailGetReportDescr1;

    if ((rcode = GetReportDescr(0, &Rpt)))
	goto FailGetReportDescr2;

    return 0;

FailGetReportDescr1:
    USBTRACE("GetReportDescr1:");
    goto Fail;

FailGetReportDescr2:
    USBTRACE("GetReportDescr2:");
    goto Fail;

Fail:
    Serial.println(rcode, HEX);
    Release();
    return rcode;
}


// handler for HID data received from the USB interface
void HIDUniversal2::ParseHIDData(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf){
  // check vor valid data
  if (len && buf)  {
    int channel;
    float epc;
    // we get 16 bits, but only 12 (MSB) are valid
    channel = (buf[1] * 256 + buf[2])/16;
    
    // did we already get data for this channel?
    if (Channels[channel] == NULL) Channels[channel] = 1;
    else Channels[channel] += 1;

    // calculate energy per channel
    epc = ChMax/(float)g_width;
    
    // set display column to update
    Col = (float)(channel - ChOffset)/epc;

    // limit display to max energy
    if (Col < g_width)
    {
        if (Cols[Col] == NULL) Cols[Col] = 1;
        else Cols[Col] += 1;
    }
    else Col = 0;

  }
}

// define USB HID interface
USB Usb;
HIDUniversal2 Hid(&Usb);
UniversalReportParser Uni;

// reset all data
void chanReset(){
  for (int i = 0; i < (sizeof(Channels)/sizeof(Channels[0])); i++){
      Channels[i] = 0;
    }
    for (int i = 0; i < g_width; i++){
      Cols[i] = 0;
    }
}

// display bar (histrogram) of corresponding channel
void chanRow (int column){
  if (!Cols[column]) return;
  int row = log10(Cols[column]) / g_y_max * (double)g_hight;
  if (row <= g_hight) M5.Lcd.drawLine(column + g_left, g_bottom, column + g_left, g_bottom - row, c_white);
}

// setup screen area (frame and units)
void lcd_clear(){
  // clear all
  M5.Lcd.fillScreen(c_black);
  
  // draw frame
  M5.Lcd.drawRect(d_border_left, d_border_top, d_width - d_border_left - d_border_right + 1, d_hight - d_border_top - d_border_bottom + 1, c_lightblue);

  // draw axis units
  M5.Lcd.setTextColor(c_white);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(d_width - 40, d_hight - 28);
  M5.Lcd.printf("keV");

  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(1, 5);
  M5.Lcd.printf("10E");
  
}

// diplay logo on LCD screen
void lcd_logo(){
  M5.Lcd.drawBitmap(0,0, 320,240, (uint16_t*)logo);
}

// calulate and display grid
void lcd_grid(){

  // vertical grid
  for (float x = 100.0; x < g_x_max; x = x + 100) {
    float xl = x2c(x);
    M5.Lcd.drawLine(xl , g_bottom - 1, xl, g_top, RGB565(0x202020));
  }

  // vertical labels
  for (float x = 0.0; x < g_x_max; x = x + 500) {
    float xl = x2c(x);
    M5.Lcd.setCursor(xl - 15, d_hight - 28);
    M5.Lcd.printf("%2u", (int)x);
  }

  // horizontal grid
  for (float y = 1; y < g_y_max; y = y + 1) {
    float yl = y2c(y);
    M5.Lcd.drawLine(g_left , yl, g_right, yl, RGB565(0x202020));
  }

  // horizontal labels
  for (float y = 0; y < g_y_max; y = y + 1) {
    float yl = y2c(y);
    M5.Lcd.setCursor(5, yl - 10);
    M5.Lcd.printf("%2u",(int)y);
  }
}

// convert RGB to 565
unsigned short int RGB565( int RGB ){
  int red   = ( RGB >> 16) & 0xFF;
  int green = ( RGB >> 8 ) & 0xFF;
  int blue  =   RGB        & 0xFF;

  unsigned short  B =  (blue  >> 3)        & 0x001F;
  unsigned short  G = ((green >> 2) <<  5) & 0x07E0;
  unsigned short  R = ((red   >> 3) << 11) & 0xF800;

    return (unsigned short int) (R | G | B);
}

// converts x value to point on grid
float x2c(float x){
  return ((float)g_left +  ((float)g_width/g_x_max) * x);
}

// converts y value to point on grid
float y2c(float y){
  return ((float)g_bottom - ((float)g_hight/g_y_max) * y);
}

// main setup
void setup(){

  // init serial port
  Serial.begin( 115200 );
  while(!Serial);

  // init USB host port
  if (Usb.Init() == -1)
      Serial.println("USB did not start.");

  delay( 200 );

  // subscribe to USB data parser
  if (!Hid.SetReportParser(0, &Uni))
      ErrorMessage<uint8_t>(PSTR("SetReportParser"), 1  );

  M5.Power.begin();

  // init LCD
  M5.Lcd.begin();
  M5.Lcd.clear();

  // display logo
  lcd_logo();

  // solving an issue where M5.button does not work
  pinMode(ButtonA, INPUT_PULLUP);
  pinMode(ButtonB, INPUT_PULLUP);
  pinMode(ButtonC, INPUT_PULLUP);

  // initialzing all data
  chanReset();
  
}

// main program loop
void loop()
{
      
  // poll usb, process data
  Usb.Task();

  if ((digitalRead(ButtonA) == 0) && (DisplayMode != DispReset)){
    // reset data
    DisplayMode = DispMeasuring;
    chanReset();
    lcd_clear();
    lcd_grid();

    // debounce
    delay (50);
    while (!digitalRead(ButtonA));
    delay (500);
  }
  
  if ((digitalRead(ButtonB) == 0) && (DisplayMode != DispMeasuring)){
    // start data acquisition
    DisplayMode = DispMeasuring;
    lcd_clear();
    lcd_grid();
    for (int i = 0; i < g_width; i++){
      chanRow(i);
    }
  }

  if ((digitalRead(ButtonC) == 0) && (DisplayMode != DispInfo)){
      // display logo
      M5.Lcd.begin();
      M5.lcd.setBrightness(100);
      DisplayMode = DispInfo;
      lcd_logo();
  }
  
  // did we recive new data?
  if (Col && (DisplayMode == DispMeasuring)){
      // calculate new bar for channel, using log10 scale
      chanRow(Col);
      // done processing of this data
      Col = 0;
  }
}

