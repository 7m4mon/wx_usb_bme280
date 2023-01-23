# wx_usb_bme280
### USB Host Sield と BME280 を組み合わせて ArduinoからFTDI (FT232RL) にコールサインと気象データを送るプログラム

BME280で計測した 気温, 湿度, 気圧 に宛先と送り元のコールサインとCRCをつけて  
USB Host Shield に接続された FTDI [FT232RL] にカンマ区切りで送るプログラムです。

電文フォーマットは以下の通りです。(最大63bytes)  
``` To,From,Latitude,Longtitude,Altitude,Tempreture(x10),Humidity(x10),Pressure(x10),CRC8(hex) ```  
ex: 7M4MON,JM1ZLK,35.6866,139.7911,2.0,223,342,10204,3b

緯度、経度、高度は設置場所の位置の値を#defineマクロで定義する必要があります。  


* https://github.com/7m4mon/dprs_usb_bme280 からFTDIバージョンに変更しました。  
* ookrx_wx_reporter.py と組み合わせることで Raspberry Pi から気象データをAPRS網に送信することが出来ます。  
* wx_ftdi_bme280 は FTDI(FT232RL)チップバージョン、wx_ch340_bme280 はCH340チップバージョンです。


![](https://github.com/7m4mon/wx_ftdi_bme280/blob/main/wx_ftdi_bme280_block.png)

![](https://github.com/7m4mon/wx_ftdi_bme280/blob/main/wx_ftdi_bme280_inside.jpg)
