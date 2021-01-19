#include <M5Stack.h>
#include <usbhid.h>
#include <hiduniversal.h>
#include <hidescriptorparser.h>
#include <usbhub.h>
#include <math.h>
#include "pgmstrings.h"

// Satisfy the IDE, which needs to see the include statment in the ino too.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif
#include <SPI.h>

// channel data
#define ChOffset 362
#define ChRise 1.0

int Channels[4096];
int MaxChannel = 0;
int Col = 0;
int Cols[320];

// LCD data
// colors
#define c_lightblue 0x051d
#define c_white 0xffff
#define c_black 0x0000
#define c_gray 0x7bef

// dimensions
#define d_width 320
#define d_hight 240
#define d_border_left 40
#define d_border_bottom 40
#define d_border_top 2
#define d_border_right 2

int g_width = d_width - d_border_left - d_border_right - 2;
int g_hight = d_hight - d_border_top -d_border_bottom - 2;
int g_left = d_border_left + 1;
int g_bottom = d_hight - d_border_bottom - 1;
int g_top = d_border_top + 1;
int g_right = d_width - d_border_right - 1;
float g_x_max = 7.7;
float g_y_max = 3;
float g_x_ppu = g_width / g_x_max;
float g_y_ppu = g_hight / g_y_max * -1.0;
int g_pointer = 0;

// HID Class 
class HIDUniversal2 : public HIDUniversal
{
public:
    HIDUniversal2(USB *usb) : HIDUniversal(usb) {};

protected:
    uint8_t OnInitSuccessful();
    void ParseHIDData(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf);
};

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


// HID data received from the USB interface
void HIDUniversal2::ParseHIDData(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf){
  if (len && buf)  {
    int channel;
    float epc;
    channel = (buf[1] * 256 + buf[2])/16;
    //D_PrintHex<uint8_t > (buf[i], 0x80);
    
    if (Channels[channel] == NULL) Channels[channel] = 1;
    else Channels[channel] += 1;

    if (channel > MaxChannel) MaxChannel = channel;
    Serial.print (channel);
    Serial.print ("/");
    Serial.println(MaxChannel);

    epc = 1500.0/(float)g_width;
    Col = (float)(channel - ChOffset)/epc;

    if (Col < g_width)
    {
        if (Cols[Col] == NULL) Cols[Col] = 1;
        else Cols[Col] += 1;
    }
    else Col = 0;
    
    //M5.Lcd.drawLine(col + g_left, g_bottom, col + g_left, 100,c_white);

  }
}

// USB data
USB Usb;
HIDUniversal2 Hid(&Usb);
UniversalReportParser Uni;

/*void lcd_logo(){
    M5.Lcd.fillScreen(c_white);
    M5.Lcd.drawBitmap(0,0, 320,240, (uint16_t*)Stoeckli_net_trigger);
    }*/

void lcd_clear(){
  // clear all
  M5.Lcd.fillScreen(c_black);
  
  // draw frame
  M5.Lcd.drawRect(d_border_left, d_border_top, d_width - d_border_left - d_border_right, d_hight - d_border_top - d_border_bottom, c_lightblue);

  // draw axis units
  M5.Lcd.setTextColor(c_white);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(d_width - 40, d_hight - 28);
  M5.Lcd.printf("keV");

  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(1, 5);
  M5.Lcd.printf("10E");
  
}

void grid_wide(){
  g_x_max = 1500;
  g_y_max = 4;
  g_x_ppu = g_width / g_x_max;
  g_y_ppu = g_hight / g_y_max * -1.0;


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

//convert RGB to 565
unsigned short int RGB565( int RGB )
{
    int red   = ( RGB >> 16) & 0xFF;
    int green = ( RGB >> 8 ) & 0xFF;
    int blue  =   RGB        & 0xFF;

    unsigned short  B =  (blue  >> 3)        & 0x001F;
    unsigned short  G = ((green >> 2) <<  5) & 0x07E0;
    unsigned short  R = ((red   >> 3) << 11) & 0xF800;

    return (unsigned short int) (R | G | B);
}

// converts x value to point on grid
float x2c(float x)
{
  return ((float)g_left +  ((float)g_width/g_x_max) * x);
}

// convers y value to point on grid
float y2c(float y)
{
  return ((float)g_bottom - ((float)g_hight/g_y_max) * y);
}

void setup()
{
  Serial.begin( 115200 );
#if !defined(__MIPSEL__)
  while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
#endif

  // initialize the M5Stack object
  M5.begin();

  /*
    Power chip connected to gpio21, gpio22, I2C device
    Set battery charging voltage and current
    If used battery, please call this function in your project
  */
  M5.Power.begin();

  lcd_clear();
  grid_wide();

  if (Usb.Init() == -1)
      Serial.println("OSC did not start.");

  delay( 200 );

  if (!Hid.SetReportParser(0, &Uni))
      ErrorMessage<uint8_t>(PSTR("SetReportParser"), 1  );


}

void loop()
{
    Usb.Task();
    if (Col)
    {
        int row;
        row = log10(Cols[Col]) / 4.0 * (double)g_hight;
        M5.Lcd.drawLine(Col + g_left, g_bottom, Col + g_left, g_bottom - row, c_white);
        Col = 0;
    }
}

