#!/bin/bash

bin_dir=$(dirname $0)
dro_dir=$(cd $bin_dir/..; pwd)
lib_dir=$dro_dir/lib
mkdir -p $lib_dir

pushd $lib_dir

git clone https://github.com/Tecsmith/DS3232RTC.git
git clone https://github.com/milesburton/Arduino-Temperature-Control-Library.git
git clone https://github.com/nigelb/Arduino-DS2762.git
git clone https://github.com/PaulStoffregen/Time.git

rm .onewire.zip
rm -Rf OneWire
wget http://www.pjrc.com/teensy/arduino_libraries/OneWire.zip -O .onewire.zip
unzip .onewire.zip

#rm .time.zip
#rm -Rf Time
#wget http://www.pjrc.com/teensy/arduino_libraries/Time.zip -O .time.zip
#unzip .time.zip
