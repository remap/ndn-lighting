#!/bin/sh
until `time /usr/bin/python /var/www/html/lighting/app/sequence.py >> sequencer.log; echo $?`; do
     echo "exited on error, restarting" >> sequencer.log
 done
