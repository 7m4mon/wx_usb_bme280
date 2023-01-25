/**
 * wx_ch340_bme280
 *
 * USB Host Sield と BME280 を組み合わせてCH340にコールサインと気象データを送る
 *
 * 電文フォーマットは以下の通り(最大63bytes)
 * "To,From,Latitude,Longtitude,Altitude,Tempreture(x10),Humidity(x10),Pressure(x10),CRC8(hex)"
 * ex: 7M4MON,JM1ZLK,35.6866,139.7911,2.0,223,342,10204,3b
 * 
 * USB_Host_Shield_2.0 Library (GPLv2)
 * https://github.com/felis/USB_Host_Shield_2.0
 * 
 * CH340 Driver
 * https://www.suke-blog.com/arduino_usb_host_shield_with_ch340/
 *
 * SparkFun_BME280_Arduino_Library (MIT)
 * https://github.com/sparkfun/SparkFun_BME280_Arduino_Library
 *
 * @author     7M4MON <github.com/7m4mon>
 * @date       1, Feb, 2023
 * @license    GPL v2
 **/

#define WX_CALL_TO "7M4MON"
#define WX_CALL_FROM "JM1ZLK"
#define WX_LATITUDE "35.6866"
#define WX_LONGTITUDE "139.7911"
#define WX_ALTITUDE "2.0"

#define PIN_SEND_BUTTON 2

//#define USB_RCV_ENABLED
/* ボタン読み込み時のチャタリング防止用のマスク */
/* USBの受信データチェックに時間がかかるのでマスクを分ける */
#ifdef USB_RCV_ENABLED
#define AVOID_CHATTERING_MASK 0x03
#else
#define AVOID_CHATTERING_MASK 0x0F
#endif


#define MSB_CRC8 (0x07) // same as crc8.py https://github.com/niccokunzmann/crc8
// Source = https://blog.goo.ne.jp/masaki_goo_2006/e/69f286d90e6140e6e8c021e43a11c815
// Validated = https://crccalc.com/
uint8_t get_crc8(char *buff, uint8_t buff_size)
{
    unsigned char *p = (unsigned char *)buff;
    unsigned char crc8;
    for (crc8 = 0x00; buff_size != 0; buff_size--)
    {
        crc8 ^= *p++;
        for (uint8_t i = 0; i < 8; i++)
        {
            if (crc8 & 0x80)
            {
                crc8 <<= 1;
                crc8 ^= MSB_CRC8;
            }
            else
            {
                crc8 <<= 1;
            }
        }
    }
    return crc8;
}

// for BME280
#include <Wire.h>
#include "SparkFunBME280.h"
BME280 mySensor;

char wxStr[64];

// センサから読み取って気象データの文字列を作成する。
// 気温・湿度・気圧は10倍の値。
void make_wx_str()
{
    memset(wxStr, 0, sizeof(wxStr));
    int tempreture = (int)(mySensor.readTempC() * 10);
    int humidity = (int)(mySensor.readFloatHumidity() * 10);
    int pressure = (int)(mySensor.readFloatPressure() / 10);

    sprintf(wxStr, "%s,%s,%s,%s,%s,%d,%d,%d,",
            WX_CALL_TO, WX_CALL_FROM, WX_LATITUDE, WX_LONGTITUDE, WX_ALTITUDE, tempreture, humidity, pressure);
    uint8_t crc = get_crc8(wxStr, strlen(wxStr)); // CRCの計算範囲は8個目の","まで。
    char crcStr[3];
    memset(crcStr, 0, sizeof(crcStr));
    sprintf(crcStr, "%02x", crc); // hexのascii 2文字にして送る。
    strcat(wxStr, crcStr);
    Serial.print("\r\nwxStr: ");
    Serial.println(wxStr);
}

// ボタンが押されたことを検出する関数
uint8_t state_send_button = 0;
uint8_t det_send_button_pressed()
{
    uint8_t retval = 0;
    state_send_button <<= 1; // チャタリング防止で離されてしばらく経ったことを検出する
    state_send_button |= (!digitalRead(PIN_SEND_BUTTON));
    if ((state_send_button & AVOID_CHATTERING_MASK) == 1)
    {
        retval = true;
    }
    return retval;
}

// CH340はスケ様が公開していたドライバを使用 https://www.suke-blog.com/arduino_usb_host_shield_with_ch340/
#include <cdc_ch34x.h>      
#include <usbhub.h>
#include "pgmstrings.h"

// Satisfy the IDE, which needs to see the include statment in the ino too.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#include <SPI.h>
#endif

class CH34XAsyncOper : public CDCAsyncOper
{
public:
    uint8_t OnInit(CH34X *pch34x);
};

uint8_t CH34XAsyncOper::OnInit(CH34X *pch34x)
{
    uint8_t rcode;

    LINE_CODING lc;
    lc.dwDTERate = 115200;
    lc.bCharFormat = 0;
    lc.bParityType = 0;
    lc.bDataBits = 8;
    lc.bFlowControl = 0;

    rcode = pch34x->SetLineCoding(&lc);

    if (rcode)
        ErrorMessage<uint8_t>(PSTR("SetLineCoding"), rcode);

    return rcode;
}
USB Usb;
CH34XAsyncOper AsyncOper;
CH34X Ch34x(&Usb, &AsyncOper);

// forDebug
static FILE uartout;
static int uart_putchar(char c, FILE *stream)
{
    if (Serial.write(c) > 0)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}
void setup()
{
    // forDebug
    fdev_setup_stream(&uartout, uart_putchar, NULL, _FDEV_SETUP_WRITE);
    stdout = &uartout;

    pinMode(PIN_SEND_BUTTON, INPUT_PULLUP);
    Serial.begin(115200);
    Serial.println("Start");

    /* USB Host Sheild Setup */
    if (Usb.Init() == -1)
        Serial.println("OSC did not start.");

    delay(200);

    /* BME280 Setup */
    mySensor.setI2CAddress(0x76);
    if (mySensor.beginI2C() == false) // Begin communication over I2C
    {
        Serial.println("Bme280.Failed");
        while (1)
            ; // Stop
    }
    else
    {
        make_wx_str(); // センサから読み取って気象データの文字列を作成する。
    }
}


void loop()
{
    Usb.Task();

    if (Usb.getUsbTaskState() == USB_STATE_RUNNING && Ch34x.isReady())
    {
        uint8_t retval = 0;
        char usbBuff[256];
        if (det_send_button_pressed())
        {
            
            make_wx_str(); // センサから読み取って気象データの文字列を作成する。
            memset(usbBuff, 0x00, sizeof(usbBuff));
            strcpy(usbBuff, wxStr);
            // retval = Ch34x.SndData(strlen(usbBuff), usbBuff);
            /* 互換Nanoに搭載されている CH340C は一度に受信できるのは 32bytes までのようなので、32バイトずつ分けて送る */
            int16_t wxLen = strlen(usbBuff);
            uint8_t i = 0;
            while(wxLen > 32){
                 retval += Ch34x.SndData(32, &usbBuff[i*32]);
                 i++;
                 wxLen -= 32;
                 delay(5);
            }
            retval += Ch34x.SndData(wxLen, &usbBuff[i*32]);
            Serial.print("SndResult:");
            Serial.println(retval);

            delay(10);
        }
        #ifdef USB_RCV_ENABLED
        /* 受信はサンプルプログラムのから変更なし */
        memset(usbBuff, 0x00, sizeof(usbBuff));
        uint16_t size = sizeof(usbBuff);
        retval = Ch34x.RcvData(&size, usbBuff); //このタスクは1.5秒程度かかる

        #ifdef DEBUG_CH340
        Serial.print("RcvData:");
        Serial.println(retval);
        Serial.print("Size:");
        Serial.println(size);
        #endif

        if (size > 0)
        {
            Serial.print("Data received:");
            printf("%s\r\n", usbBuff);
        }
        #endif      //USB_RCV_ENABLED
        
        delay(50);
    }else{
        Serial.println("CH34X:not ready.");
        delay(100);
    }
}
