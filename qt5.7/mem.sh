#!/bin/sh  
#get /proc/pid/task/tid/stat  
#$1 is tid  
#$14  is user cpu   
#$15 is sys cpu  
echo "tid	user	sys"  
for file in /proc/$1/task/*  
do  
    if test -d $file  
    then  
        cat $file/stat | awk -F" " '{print $1 " " $14 " " $15}'  
    fi  
done  