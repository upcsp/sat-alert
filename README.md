# sat-alert

sat-alert is a small service oriented to be execute by the computer cronetab. It takes a date, a satellite name and url to post as CLI arguments. It returns a list of all the satellite passes above the ground station that **PREDICT** is configured to use.

The package is composed by:

1. **sat-alarm.c**. It retrieves data from **PREDICT** in server mode.

2. **updateTLE.sh**. This script only updates the local TLE's for *PREDICT* or *GPREDICT* to use. It does so by downloading the last published TLE'S at celestrak.com.

This code is intended to be run on a server where *PREDICT* is running in server mode. Also, both codes are inteded to be added into a UNIX cronetab. **sat-alarm.c** should run once per day, while **updateTLE.sh** once per hour.

## External libraries
**sat-alarm.c** uses several libraries that need to be installed before compilation.
Two of those need to be added dinamically while compiling. These are:
1. **curl.h**
2. **json-c.h**
Therefore, after installing all the libraries, to compile:
```
gcc sat-alarm.c -ljson -lcurl
```
## Crontab for periodic execution:
```
crontab -e
```
In crontab file add the follwoing line to run in an hourly basis, for example.
```
0 * * * * ./path/to/scheduled/code
```

## PREDICT configuration
NOTE: By default, PREDICT uses socket port 1210 for communicating with client applications.
First, install *PREDICT*.
```
sudo apt install predict
```
Now configure your QTH information and perform a first manual update of its TLE's.
Finally, start it in server mode.

```
predict -s
```
## Example of manual usage
After follwoing the previous steps just run the code like:

```
sat-alert -s ISS -d 20/06/2017 -u https://hooks.slack.com/services/xxxxx
```
This would POST to the given URL information about the ISS passing above the ground station, the 20th of June of 2017.

| Date and Time (UTC)       	 | El   |  Az  | Ph   | Lat  | Lon  | R    | Orbit|
| -------------------------------|:----:|:----:|:----:|:----:|:----:|:----:|:----: |
| 1493799554 Wed 03May17 08:19:14| 0    | 159  | 192  | 23   | 65   | 2304 | 5477 + |
| 1493799624 Wed 03May17 08:20:24| 2	| 147  | 195  | 26   | 62   | 2080 | 5477 + |

Its raw format in JSON would be:
```
{
    "attachments": [
        {
            "color": "#36a64f",
            "pretext": "SAT NAME + DATE",
            "title": "Date and Time (UTC)  | El   |  Az  | Ph   | Lat  | Lon  | R    | Orbit",
            "text": "1493828443 Wed 03May17 16:20:43    0  305  242   50   99   2313   5482 *"
        }
    ]
}
```


If date is not given, then the current date is taken.
For automatic usage the command is the same but run by cronetab.

## Thanks
We would like to thank the *PREDICT* developers for the great CLI tool for Satellite tracking that they have generated.
https://github.com/koansys/predict