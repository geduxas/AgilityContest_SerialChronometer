#!/usr/bin/env bash
BASE=`pwd`
FECHA=`date +"%Y%m%d_%H%M"`
BUILD=SerialChrono
rm -rf ${BASE}/${BUILD} ${BASE}/${BUILD}-${FECHA}.zip
mkdir -p ${BASE}/${BUILD}
cd ${BASE}/${BUILD}
# regenerate PDF documentation file from source
# wget http://www.agilitycontest.es/downloads/ac_SerialProtocol.pdf
libreoffice \
  --headless \
  "-env:UserInstallation=file:///tmp/LibreOffice_Conversion_${USER}" \
  --convert-to pdf:writer_pdf_Export \
  --outdir . \
  ${BASE}/ac_SerialProtocol.odt

# copy required files
cp ${BASE}/INSTALL.txt .
cp ${BASE}/LICENSE .
cp ${BASE}/serial_chrono.{exe,ini,sh} .
cp ${BASE}/agilitycontest_64x64.png .
cp ${BASE}/cmake-build-debug/digican.so .
cp ${BASE}/cmake-build-debug/generic.so .
cp ${BASE}/cmake-build-debug/canometroweb.so .
cp ${BASE}/cmake-build-debug/SerialChronometer .
cp ${BASE}/cmake-build-debug-mingw/digican.so digican.dll
cp ${BASE}/cmake-build-debug-mingw/generic.so generic.dll
cp ${BASE}/cmake-build-debug-mingw/canometroweb.so canometroweb.dll
cp ${BASE}/cmake-build-debug-mingw/SerialChronometer.exe .
cp -r ${BASE}/html .

# and compose zip file with contents
cd ${BASE}
zip -r ${BASE}/${BUILD}-${FECHA}.zip ${BUILD} && rm -rf ${BASE}/${BUILD}
