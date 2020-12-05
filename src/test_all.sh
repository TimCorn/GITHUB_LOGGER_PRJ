#!/bin/sh

#In test program work some (four) threads. Each of them do 
#writings to a Log files.
#So, Total is wanted number of records in Log files for one thread.
#I.e. it is option -c for utility "main_test".
Total=20 

#Parameters of Log files that will be created.
NUM_REGS=25 
REG_SIZE=700
MAX_STR_LEN=250

#The place of our Log directory
LogDir=../LOD_DIR/JOURNAL/xx


#Error counter for all checks.
ERR_COUNT=0


rm -f $LogDir/* 
./main_test -p ./Logger_common.cfg -c $Total -t 4 > /dev/null

echo "\n\n"
echo "Checking how to form Log level messages."
echo "/main_test -p ./Logger_common.cfg -c $Total -t 4 "
echo "\n"

res="$(grep -roh DEBUG $LogDir | wc -w)" 
if [ $res -eq $Total ]
then
   echo "Checking DEBUG messages...OK"
else
   echo "Checking DEBUG messages...ERR"
   ERR_COUNT=$((ERR_COUNT+1))
fi

res="$(grep -roh INFO $LogDir | wc -w)" 
if [ $res -eq $Total ]
then
   echo "Checking INFO messages...OK"
else
   echo "Checking INFO messages...ERR"
   ERR_COUNT=$((ERR_COUNT+1))
fi

res="$(grep -roh WARN $LogDir | wc -w)" 
if [ $res -eq $Total ]
then
   echo "Checking WARN messages...OK"
else
   echo "Checking WARN messages...ERR"
   ERR_COUNT=$((ERR_COUNT+1))
fi

res="$(grep -roh ERROR $LogDir | wc -w)" 
if [ $res -eq $Total ]
then
   echo "Checking ERROR messages...OK"
else
   echo "Checking ERROR messages...ERR"
   ERR_COUNT=$((ERR_COUNT+1))
fi


echo "\n\n"
echo "Checking size of created files and number of them."
echo "\n"

#The size of created Log files can be from 0 to (REG_SIZE+MAX_STR_LEN).
#The number of cretaed files can be from 0 to NUM_REGS.
CURR_LEN=$((REG_SIZE+MAX_STR_LEN))
CURR_NUM_OF_FILES=0

for entry in "$LogDir"/*
do
   size="$(wc -c $entry | awk '{print $1}')"
   #echo "$entry: $size"
   CURR_NUM_OF_FILES=$((CURR_NUM_OF_FILES+1))
   if [ $size -le $CURR_LEN ]  
   then
      echo "$entry size...OK"
   else
      echo "$entry size...ERR"
      ERR_COUNT=$((ERR_COUNT+1))
   fi
done

if [ $CURR_NUM_OF_FILES -le $NUM_REGS ]
then
    echo "Number of created files = $CURR_NUM_OF_FILES...OK"
else
    echo "Number of created files = $CURR_NUM_OF_FILES...ERR"
    ERR_COUNT=$((ERR_COUNT+1))
fi


echo "\n\n"
echo "Checking barrier level of messages."
echo "Set WARN level."
echo "./main_test -p ./Logger_barrier.cfg  -c $Total -t 4 "
echo "\n"

rm -f $LogDir/* 
./main_test -p ./Logger_barrier.cfg  -c $Total -t 4 > /dev/null

res="$(grep -roh DEBUG $LogDir | wc -w)" 
if [ $res -eq 0 ]
then
   echo "Checking NO DEBUG messages...OK"
else
   echo "Checking NO DEBUG messages...ERR"
   ERR_COUNT=$((ERR_COUNT+1))
fi

res="$(grep -roh INFO $LogDir | wc -w)" 
if [ $res -eq 0 ]
then
   echo "Checking NO INFO messages...OK"
else
   echo "Checking NO INFO messages...ERR"
   ERR_COUNT=$((ERR_COUNT+1))
fi

res="$(grep -roh WARN $LogDir | wc -w)" 
if [ $res -eq $Total ]
then
   echo "Checking WARN messages...OK"
else
   echo "Checking WARN messages...ERR"
   ERR_COUNT=$((ERR_COUNT+1))
fi

res="$(grep -roh ERROR $LogDir | wc -w)" 
if [ $res -eq $Total ]
then
   echo "Checking ERROR messages...OK"
else
   echo "Checking ERROR messages...ERR"
   ERR_COUNT=$((ERR_COUNT+1))
fi


echo "\n\n"
echo "Checking filtering messages by pattern."
echo "Set pattern Third (that is a class)."
echo "./main_test -p ./Logger_pattern.cfg -c  -t 4 $Total "
echo "\n"

rm -f $LogDir/* 
./main_test -p ./Logger_pattern.cfg -c  $Total -t 4 > /dev/null

res="$(grep -roh Third $LogDir | wc -w)" 
if [ $res -eq $Total ]
then
   echo "Checking messages with class Third...OK"
else
   echo "Checking messages with class Third...ERR"
   ERR_COUNT=$((ERR_COUNT+1))
fi

echo "\n\n"
echo "Checking ReConfigure() - changing the settings"
echo "without program restart."


#If Total>30 (such done for debug purposes) each thread
#number #3 in main program will call ReConfig(). 
#So the number of records about ReConfigure() calls 
#will be equal 8. Such put in code.

NUM_REGS=41

Total=32
RcfNum=$((Total / 4))

echo "./main_test -p ./Logger_Reconf.cfg -c $Total -t 4 "
echo "\n"

rm -f $LogDir/* 
./main_test -p ./Logger_Reconf.cfg -c $Total -t 4 > /dev/null

res="$(grep -roh ReConfigure $LogDir | wc -w)" 
if [ $res -eq $RcfNum ]
then
   echo "Checking ReConfig(). Have $res calls ...OK"
else
   echo "Checking ReConfig(). Have $res calls ...ERR"
   ERR_COUNT=$((ERR_COUNT+1))
fi

echo "\n"

echo "*******************************************************"

echo "\n"

if [ $ERR_COUNT -eq 0 ]
then
   echo "The set of tests executed ...OK"
else
    echo "The set of tests executed $ERR_COUNT...ERR"
fi

echo "\n\n"




