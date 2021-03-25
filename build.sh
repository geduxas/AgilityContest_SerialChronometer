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

# copy config docs and startup files
cp ${BASE}/INSTALL.txt .
cp ${BASE}/LICENSE .
cp ${BASE}/serial_chrono.ini .
cp ${BASE}/sc_dialog.{sh,exe} .
cp ${BASE}/agilitycontest_64x64.png .

linux_dir=${BASE}/cmake-build-debug
windows_dir=${BASE}/cmake-build-debug-mingw
osx_dir=${BASE}/cmake-build-debug-xcode

# Linux files
if [ -d $linux_dir ]; then
  cp ${linux_dir}/digican.so .
  cp ${linux_dir}/generic.so .
  cp ${linux_dir}/canometroweb.so .
  cp ${linux_dir}/dummy.so .
  cp ${linux_dir}/SerialChronometer .
else
  echo "WARNING: cannot find linux compilation"
fi

# Windows files
if [ -d ${BASE}/cmake-build-debug-mingw ]; then
  cp ${windows_dir}/digican.so digican.dll
  cp ${windows_dir}/generic.so generic.dll
  cp ${windows_dir}/canometroweb.so canometroweb.dll
  cp ${windows_dir}/dummy.so dummy.dll
  cp ${windows_dir}/SerialChronometer.exe .
else
  echo "WARNING: Cannot find Windows related files"
fi

# MacOSX files
if [ -d ${osx_dir} ]; then
  cp ${osx_dir}/digican.dylib digican.dylib
  cp ${osx_dir}/generic.dylib generic.dylib
  cp ${osx_dir}/canometroweb.dylib canometroweb.dylib
  cp ${osx_dir}/dummy.dylib dummy.dylib
  cp ${osx_dir}/SerialChronometer.osx .
else
  echo "WARNING: cannot find MacOSX compiled files"
fi

# Web pages
cp -r ${BASE}/html .

# and compose zip file with contents
cd ${BASE}
zip -r ${BASE}/${BUILD}-${FECHA}.zip ${BUILD} && rm -rf ${BASE}/${BUILD}
