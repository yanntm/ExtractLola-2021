#!/bin/bash

while true
do
  	usedMemory=$(free | awk '/^Mem:/{print $3}')
        dateTime=`date '+%Y-%m-%d-%H:%M:%S'`
        echo "$dateTime memory usage is: $usedMemory"
        ###########################################################################################
        # Check if we use more than x mb of ram ###################################################
        ###########################################################################################
        if [ $usedMemory -gt 970000000 ]
        #if [ $usedMemory -gt 1400000000 ]
        then
                # Get the lola process which is using the most memory
                lolaProcess=`ps -eo pid,user,vsz,start,time,command --sort vsz | grep 'lola' | tail -n 1`
                echo "Following LoLA process will be killed: $lolaProcess"
                echo "Following LoLA process will be killed: $lolaProcess" >> kills.out
                kill -9 `ps -eo pid,user,vsz,command --sort vsz | grep 'lola' | tail -n 1 | awk '{print $1}'`
        fi
	
        ###########################################################################################
	# Get sara processes belongig to the parent id = 1 ########################################
        ###########################################################################################
        saraProcess=`ps -eo ppid,comm,pid,user,vsz,start,time --sort=vsz | sed 's/^ *//' | grep '^1 sara' | tail -n 1`

        if [ "$saraProcess" ]
        then
                echo "Following sara process will be killed: $saraProcess"
                echo "Following sara process will be killed: $saraProcess" >> kills.out
                kill -9 `echo $saraProcess | awk '{print $3}'`
        fi

        ###########################################################################################
        # Get the longest running lola process and get the seconds it is running ##################
        ###########################################################################################
        # Maximum allowed run time
        MaxTimeRunning=330
        lolaLongProcess=`ps -eo pid,etime,user,command --sort etime | grep 'lola' | tail -n 1`
        seconds=`echo $lolaLongProcess | awk '{split($2, a, ":"); if (length(a)==2) sec=a[1]*60+a[2]; \
            else if (length(a)==3) sec=a[1]*3600+a[2]*60+a[3]; print sec;}'`

        if [ $seconds -gt $MaxTimeRunning ]
        then
            echo "Following sara process will be killed: $lolaLongProcess"
            echo "Following sara process will be killed: $lolaLongProcess" >> kills.out
            kill -9 `echo $lolaLongProcess | awk '{print $1}'`
        # else
        # 	echo "nothing to see here"
        fi
	
        ###########################################################################################
        # Get lola processes belongig to the parent id = 1 ########################################
        ###########################################################################################
#        lolaProcess=`ps -eo ppid,comm,pid,user,vsz,start,time --sort=vsz | sed 's/^ *//' | grep '^1 lola' | tail -n 1`

#        if [ "$lolaProcess" ]
#        then
#                echo "Following lola process will be killed: $lolaProcess"
#                echo "Following lola process will be killed: $lolaProcess" >> kills.out
#                kill -9 `echo $lolaProcess | awk '{print $3}'`
#        fi

        sleep 3
done