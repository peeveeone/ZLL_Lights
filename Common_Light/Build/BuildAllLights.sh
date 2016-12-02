#!/bin/bash
#Usage below
#    1. make sure that environment Path variable must contain C:\NXP\bstudio_nxp\msys\bin in the beginning!
#    2. ./BuildAllLights.sh <JN5168 or JN5169>
echo "Building Lights from JN-AN-1171"

start=$(date +"%T")
echo "Start Time : $start"
    
if [ "$1" == "JN5168" ] || [ "$1" == "JN5169" ]; then
    make JENNIC_SDK=JN-SW-4168 JENNIC_CHIP=$1 LIGHT=Light_ColorLight            clean >> BuildLog_Light_ColorLight_$1.txt &
    make JENNIC_SDK=JN-SW-4168 JENNIC_CHIP=$1 LIGHT=Light_ColorTemperatureLight clean >> BuildLog_Light_ColorTemperatureLight_$1.txt &
    make JENNIC_SDK=JN-SW-4168 JENNIC_CHIP=$1 LIGHT=Light_DimmableLight         clean >> BuildLog_Light_DimmableLight_$1.txt &
    make JENNIC_SDK=JN-SW-4168 JENNIC_CHIP=$1 LIGHT=Light_DimmablePlug          clean >> BuildLog_Light_DimmablePlug_$1.txt &
    make JENNIC_SDK=JN-SW-4168 JENNIC_CHIP=$1 LIGHT=Light_ExtendedColorLight    clean >> BuildLog_Light_ExtendedColorLight_$1.txt &
    make JENNIC_SDK=JN-SW-4168 JENNIC_CHIP=$1 LIGHT=Light_OnOffLight            clean >> BuildLog_Light_OnOffLight_$1.txt &
    make JENNIC_SDK=JN-SW-4168 JENNIC_CHIP=$1 LIGHT=Light_OnOffPlug             clean >> BuildLog_Light_OnOffPlug_$1.txt &
    echo "Cleaning; Please wait"
    wait

    make JENNIC_SDK=JN-SW-4168 JENNIC_CHIP=$1 LIGHT=Light_ColorLight            >> BuildLog_Light_ColorLight_$1.txt &
    make JENNIC_SDK=JN-SW-4168 JENNIC_CHIP=$1 LIGHT=Light_ColorTemperatureLight >> BuildLog_Light_ColorTemperatureLight_$1.txt &
    make JENNIC_SDK=JN-SW-4168 JENNIC_CHIP=$1 LIGHT=Light_DimmableLight         >> BuildLog_Light_DimmableLight_$1.txt &
    make JENNIC_SDK=JN-SW-4168 JENNIC_CHIP=$1 LIGHT=Light_DimmablePlug          >> BuildLog_Light_DimmablePlug_$1.txt &
    make JENNIC_SDK=JN-SW-4168 JENNIC_CHIP=$1 LIGHT=Light_ExtendedColorLight    >> BuildLog_Light_ExtendedColorLight_$1.txt &
    make JENNIC_SDK=JN-SW-4168 JENNIC_CHIP=$1 LIGHT=Light_OnOffLight            >> BuildLog_Light_OnOffLight_$1.txt &
    make JENNIC_SDK=JN-SW-4168 JENNIC_CHIP=$1 LIGHT=Light_OnOffPlug             >> BuildLog_Light_OnOffPlug_$1.txt &
    echo "Building; Please wait"
    wait
else
    echo "Usage ./BuildAllLights.sh <JN5168 or JN5169>"
fi

end=$(date +"%T")
echo "End Time : $end"

echo "Done !!!"
