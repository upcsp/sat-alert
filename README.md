# sat-alert

> Add crontab for periodic TLE Update (updateTLE.sh) :
```"crontab -e"
```
In crontab file add the follwoing line to run an hourly update of the TLE.
> 0 * * * * ./home/tgs/repos/sat-alert/updateTLE.sh


## PREDICT is put into server mode
``` predict -s
```
> By default, PREDICT uses socket port 1210 for communicating with client applications.



## To Do
	1. Cronjob for sat-alarm.c
	2. Get Weekly Resume