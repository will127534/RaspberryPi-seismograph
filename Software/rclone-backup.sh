#!/bin/bash
if pidof -o %PPID -x “rclone-backup.sh”; then
exit 1
fi

DATE=`date +%Y_%m_%d`
FILEDATE=`date +%Y%m%d`
/usr/bin/rclone -vv -P copy  /home/pi/earthquake/ google:$DATE  --retries 1 --fast-list --include "$FILEDATE*.txt" --no-traverse 

DATE_Before=$(date +%Y_%m_%d -d "1 day ago")
FILEDATE_Before=$(date +%Y%m%d -d "1 day ago")
echo $DATE_Before
/usr/bin/rclone -vv -P copy  /home/pi/earthquake/ google:$DATE_Before  --retries 1 --fast-list --include "$FILEDATE_Before*.txt" --no-traverse


find /home/pi/earthquake/ -name '*.txt' -type f -mmin +1440  -delete

exit

