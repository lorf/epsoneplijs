#!/bin/sh

PATH=.:${PATH}

PAPERSIZE=a4

gs -sPAPERSIZE=a4 -dFIXEDMEDIA \
-sProcessColorModel=DeviceGray -dBitsPerSample=1 \
-sDEVICE=ijs -sIjsServer=ijs_server_epsonepl \
-sDeviceManufacturer=Epson -sDeviceModel=EPL5700L \
-sIjsParams="" \
-dIjsUseOutputFD \
-dNOPAUSE -dSAFER -dBATCH \
-sOutputFile=epl_test_5700L.epl epl_test.ps

gs -sPAPERSIZE=a4 -dFIXEDMEDIA \
-sProcessColorModel=DeviceGray -dBitsPerSample=1 \
-sDEVICE=ijs -sIjsServer=ijs_server_epsonepl \
-sDeviceManufacturer=Epson -sDeviceModel=EPL5800L \
-sIjsParams="" \
-dIjsUseOutputFD \
-dNOPAUSE -dSAFER -dBATCH \
-sOutputFile=epl_test_5800L.epl epl_test.ps

gs -sPAPERSIZE=a4 -dFIXEDMEDIA \
-sProcessColorModel=DeviceGray -dBitsPerSample=1 \
-sDEVICE=ijs -sIjsServer=ijs_server_epsonepl \
-sDeviceManufacturer=Epson -sDeviceModel=EPL5900L \
-sIjsParams="" \
-dIjsUseOutputFD \
-dNOPAUSE -dSAFER -dBATCH \
-sOutputFile=epl_test_5900L.epl epl_test.ps

gs -sPAPERSIZE=a4 -dFIXEDMEDIA \
-sProcessColorModel=DeviceGray -dBitsPerSample=1 \
-sDEVICE=ijs -sIjsServer=ijs_server_epsonepl \
-sDeviceManufacturer=Epson -sDeviceModel=EPL6100L \
-sIjsParams="" \
-dIjsUseOutputFD \
-dNOPAUSE -dSAFER -dBATCH \
-sOutputFile=epl_test_6100L.epl epl_test.ps

gs -sPAPERSIZE=a4 -dFIXEDMEDIA \
-sProcessColorModel=DeviceGray -dBitsPerSample=1 \
-sDEVICE=ijs -sIjsServer=ijs_server_epsonepl \
-sDeviceManufacturer=Epson -sDeviceModel=EPL6200L \
-sIjsParams="" \
-dIjsUseOutputFD \
-dNOPAUSE -dSAFER -dBATCH \
-sOutputFile=epl_test_6200L.epl epl_test.ps

gs -sPAPERSIZE=a4 -dFIXEDMEDIA \
-sProcessColorModel=DeviceGray -dBitsPerSample=1 \
-sDEVICE=ijs -sIjsServer=ijs_server_epsonepl \
-sDeviceManufacturer=Epson -sDeviceModel=EPL5700L \
-sIjsParams="EplFlowControl=nowhere" \
-dIjsUseOutputFD \
-dNOPAUSE -dSAFER -dBATCH \
-sOutputFile=/dev/null epl_test.ps

gs -sPAPERSIZE=a4 -dFIXEDMEDIA \
-sProcessColorModel=DeviceGray -dBitsPerSample=1 \
-sDEVICE=ijs -sIjsServer=ijs_server_epsonepl \
-sDeviceManufacturer=Epson -sDeviceModel=EPL5800L \
-sIjsParams="EplFlowControl=nowhere" \
-dIjsUseOutputFD \
-dNOPAUSE -dSAFER -dBATCH \
-sOutputFile=/dev/null epl_test.ps

gs -sPAPERSIZE=a4 -dFIXEDMEDIA \
-sProcessColorModel=DeviceGray -dBitsPerSample=1 \
-sDEVICE=ijs -sIjsServer=ijs_server_epsonepl \
-sDeviceManufacturer=Epson -sDeviceModel=EPL5900L \
-sIjsParams="EplFlowControl=nowhere" \
-dIjsUseOutputFD \
-dNOPAUSE -dSAFER -dBATCH \
-sOutputFile=/dev/null epl_test.ps

gs -sPAPERSIZE=a4 -dFIXEDMEDIA \
-sProcessColorModel=DeviceGray -dBitsPerSample=1 \
-sDEVICE=ijs -sIjsServer=ijs_server_epsonepl \
-sDeviceManufacturer=Epson -sDeviceModel=EPL6100L \
-sIjsParams="EplFlowControl=nowhere" \
-dIjsUseOutputFD \
-dNOPAUSE -dSAFER -dBATCH \
-sOutputFile=/dev/null epl_test.ps

gs -sPAPERSIZE=a4 -dFIXEDMEDIA \
-sProcessColorModel=DeviceGray -dBitsPerSample=1 \
-sDEVICE=ijs -sIjsServer=ijs_server_epsonepl \
-sDeviceManufacturer=Epson -sDeviceModel=EPL6200L \
-sIjsParams="EplFlowControl=nowhere" \
-dIjsUseOutputFD \
-dNOPAUSE -dSAFER -dBATCH \
-sOutputFile=/dev/null epl_test.ps
