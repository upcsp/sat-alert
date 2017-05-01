#!/bin/sh
wget -qr www.celestrak.com/NORAD/elements/amateur.txt -O ./tle/amateur.txt
wget -qr www.celestrak.com/NORAD/elements/visual.txt -O ./tle/visual.txt
wget -qr www.celestrak.com/NORAD/elements/weather.txt -O ./tle/weather.txt

#  Updates PREDICT Database
/usr/bin/predict -u ./tle/amateur.txt ./tle/visual.txt ./tle/weather.txt

#  One have to manually update Gpredict database.