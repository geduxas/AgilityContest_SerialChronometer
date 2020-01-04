#!/usr/bin/env bash
BASE=`pwd`
FECHA=`date +"%Y%m%d_%H%M"`
BUILD=SerialChrono
rm -rf ${BASE}/${BUILD} ${BASE}/${BUILD}-${FECHA}.zip
mkdir -p ${BASE}/${BUILD}
cd ${BASE}/${BUILD}
wget http://www.agilitycontest.es/downloads/ac_SerialProtocol.pdf
cp ${BASE}/INSTALL.txt .
cp ${BASE}/LICENSE .
cp ${BASE}/serial_chrono.{exe,ini,sh}
cp .${BASE}/agilitycontest_64x64.png .
cp ${BASE}/cmake-build-debug/digican.so .
cp ${BASE}/cmake-build-debug/generic.so .
cp ${BASE}/cmake-build-debug/SerialChronometer .
cp ${BASE}/cmake-build-debug-mingw/digican.so digican.dll
cp ${BASE}/cmake-build-debug-mingw/generic.so generic.dll
cp ${BASE}/cmake-build-debug-mingw/SerialChronometer.exe .
cp -r ${BASE}/html .
cd ${BASE}
zip -r ${BASE}/${BUILD}-${FECHA}.zip ${BUILD} && rm -rf ${BASE}/${BUILD}
