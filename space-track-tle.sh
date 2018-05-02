
#!/bin/sh
# Based on https://www.space-track.org/documentation#howto-api_intro
user="USERNAME"
password="PASSWORD"

NORAD_CAT_ID="25544"

# Gets last JSON of selected NORAD object. It contains complete orbit information.
query1='https://www.space-track.org/basicspacedata/query/class/tle/format/tle_last/NORAD_CAT_ID/'"$NORAD_CAT_ID"'/orderby/EPOCH%20desc/limit/1'
# Gets last TLE of selected NORAD object
query2='https://www.space-track.org/basicspacedata/query/class/tle/format/tle/NORAD_CAT_ID/'"$NORAD_CAT_ID"'/orderby/EPOCH%20desc/limit/2'

curl https://www.space-track.org/ajaxauth/login -d 'identity='"$user"'&password='"$password"'&query='"$query1" > ISS.txt