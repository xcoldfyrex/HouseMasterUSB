#include <SPI.h>
#include "usbdrv.h"
#include <Wire.h>
#include "printf.h"

#define USB_DATA_INIT 0
#define USB_DATA_IN 4
#define USB_DATA_EXEC 5
#define USB_DATA_OUT 6

unsigned char USBBuffer[64];
static uchar initBuf[3] = "OK";
static uchar usbOut[32] = "";
static uchar dataReceived = 0, dataLength = 0; // for USB_DATA_IN

void setup() {
  usbDeviceDisconnect();  /* enforce re-enumeration, do this while interrupts are disabled! */
  usbDeviceConnect();
  Wire.begin(1);
  Wire.onReceive(receiveEvent);
  SPI.begin();
  usbInit();
  sei();
  Serial.begin(115200);
  printf_begin();
}

void loop() {
  usbPoll();
}

void process_USB_buffer() {
  if (strlen((char *) USBBuffer) < 12) {
    printf("ERR: USB payload [%s] is too small, got %d bytes\n\r", (char *) USBBuffer, strlen((char *)USBBuffer));
    return;
  }
  Wire.beginTransmission(2); 
  Wire.write((char *)USBBuffer);
  Wire.endTransmission();
  printf("USB: %s \n\r", (char *) USBBuffer);
  memset(USBBuffer,0, sizeof(USBBuffer));
}

usbMsgLen_t usbFunctionSetup(uchar data[128])
{
  usbRequest_t    *rq = (usbRequest_t*)((void *)data);
  switch (rq->bRequest) { // custom command is in the bRequest field
    case USB_DATA_INIT:
      usbMsgPtr = initBuf;
      return sizeof(initBuf);
      
    case USB_DATA_IN: //add to buffer
      dataLength  = (uchar)rq->wLength.word;
      dataReceived = 0;
      return USB_NO_MSG; 

    case USB_DATA_EXEC: //act on buffer
      process_USB_buffer();
      return 0;
      
    case USB_DATA_OUT: // send data to PC
      usbMsgPtr = usbOut;
      return sizeof(usbOut);
  }
  return 0;
}

USB_PUBLIC uchar usbFunctionWrite(uchar *data, uchar len) {
  uchar i;
  for (i = 0; dataReceived < dataLength && i < len; i++, dataReceived++) {

    USBBuffer[dataReceived] = data[i];
  }
  return (dataReceived == dataLength);
}

void receiveEvent(int howMany) {
  while (1 < Wire.available()) { // loop through all but the last
    char c = Wire.read(); // receive byte as a character
    Serial.print(c);         // print the character
  }
  int x = Wire.read();    // receive byte as an integer
  Serial.println(x);         // print the integer
}
