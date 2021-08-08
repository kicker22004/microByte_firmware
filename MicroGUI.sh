#!/bin/bash
#Variables
GITHUB="kicker22004"  #Change this to your github name.
ESP_IDF="/root/esp-ws/esp-idf/"
WORKINGDIR="/root/esp-ws/esp-idf/microByte_firmware/"
BACKUP="/root/esp-ws/esp-idf/backup/"
BAUD="2000000"



# Make sure we run with root privileges
if [ $UID != 0 ]; then
# not root, use sudo
	echo "This script needs root privileges, rerunning it now using sudo!"
	sudo "${SHELL}" "$0" $*
	exit $?
fi
# get real username
if [ $UID = 0 ] && [ ! -z "$SUDO_USER" ]; then
	USER="$SUDO_USER"
else
	USER="$(whoami)"
fi

do_update() {
backup="/root/esp-ws/esp-idf/backup/"
workingdir="/root/esp-ws/esp-idf/microByte_firmware/"
        if (whiptail --fb --title "Updater" --yesno "Welcome, this will update your build to the Master branch, this is distructive to your local changes so a backup will be made in '$backup'. \
To continue press Yes, to backout press No." 15 60) then
        (
        echo XXX
        echo 20
	echo "Rsync is moving files"
	sleep 2
	rsync --recursive $WORKINGDIR $BACKUP
        echo XXX
        echo XXX
        echo 40
	echo "git fetch --all"
	cd $WORKINGDIR
	git fetch --all > /dev/null
	echo XXX
        sleep 2
        echo XXX
        echo 80
        echo "git reset --hard origin/master"
	git reset --hard origin/master > /dev/null
        echo XXX
        sleep 2
        echo XXX
        echo 100
        echo "Completed"
        echo XXX
        sleep 2
) | whiptail --gauge "Backing up and Updating" 8 40 0
	whiptail --fb --msgbox "Backup was completed and the local files match the github version." 20 60
	clear
	echo "Backup was completed and the local files match the github version."
	do_menu
else
        do_menu
fi
}

do_build() {
if [ -d "$WORKINGDIR/build" ]; then
	#The first time you build it's going to build some of the parts so we will need to run this twice the first time.
	#Dropping to terminal to see the output and let the user press a key to continue in case of errors.
	cd $WORKINGDIR
	make -j4
	read -n 1 -s -r -p "Make -j4 - Has completed... Press any key to continue"  #Only here to allow for viewing of errors.
	make -j12 flash ESPPORT=/dev/$DEVICE ESPBAUD=$BAUD
	read -n 1 -s -r -p "Build & Flash compelted... Press any key to continue"  #Only here to allow for viewing of errors.
else
	cd $WORKINGDIR
	make -j4
	#Loop back
	do_build
fi
}

do_find() {
### Find out if the usb is detected, if not let's inform and close.
DEVICE=$(dmesg | grep "ch341-uart converter.*" | tail -n1 | awk '{print $NF}')
DEV_ARRAY=( $(ls /dev | grep tty.*) )

if [[ ${DEV_ARRAY[@]} =~ $DEVICE ]]; then
	clear
	echo "Your device was found at $DEVICE"
	echo "Preparing to Build..."
	do_build

else
if (whiptail --fb --title "Error" --yesno "Your device was not found... Try again?" 15 60) then
	do_find
else
	do_menu
fi
fi
}

do_pathcheck() {
PATHTEXT=$(echo $PATH | grep -o ~/esp-ws/esp-idf | tail -n1)

if [ ! "$PATHTEXT" = "/root/esp-ws/esp-idf" ]; then
	clear
	echo "Missing Path, Please run (. ./export.sh) from (/esp-ws/esp-idf/) first. Then Re-run ./MicroGUI.sh"
exit 1
fi
}

do_pull() {
if [ ! -d $WORKINGDIR ]; then
	cd $ESP_IDF
	git clone https://github.com/$GITHUB/microByte_firmware --recursive
	do_pull
fi
}

do_start() {
FILE=$ESP_IDF/install.sh
#Check for the installer, if found then it likely has been installed, so move on..
if [ ! -f "$FILE" ]; then
	clear
	echo "Installing requirements..."
	sudo apt-get -y update
	sudo apt-get -y install git wget flex bison gperf python python-pip python-setuptools cmake ninja-build ccache libffi-dev libssl-dev dfu-util whiptail
	clear
	echo "Setting up ESP-IDF (version 4.1.)"
        mkdir ~/esp-ws
        cd ~/esp-ws
        git clone https://github.com/espressif/esp-idf -b v4.1
        cd $ESP_IDF
        ./install.sh
	do_pathcheck
else
	do_pull
	do_pathcheck
	do_menu
fi

}

do_adminmenu() {
while true; do
	FUN=$(whiptail --fb --title "Update/Mods" --menu "WARNING!!!"  --cancel-button Back --ok-button Select 20 80 4 \
    		"1 Update" "[Update from Github]" \
    		3>&1 1>&2 2>&3)
  	RET=$?
if [ $RET -eq 1 ]; then
	do_menu
	elif [ $RET -eq 0 ]; then
		case "$FUN" in
		1\ *) do_update ;;
		*) whiptail --msgbox "Programmer error: unrecognized option" 20 60 1 ;;
	esac || whiptail --msgbox "There was an error running option $FUN" 20 60 1
fi
done
}

do_menu() {
while true; do
  FUN=$(whiptail --fb --title "Main Menu" --menu "Options"  --cancel-button Exit --ok-button Select 20 80 8 \
	"1 Admin Menu" "[Admin, Be carefule]" \
        "2 Build" "[Build from local & Flash]" \
	"3 Mods" "[Custom options]" \
    3>&1 1>&2 2>&3)
  RET=$?
  if [ $RET -eq 1 ]; then
    exit 1
  elif [ $RET -eq 0 ]; then
    case "$FUN" in
        1\ *) do_adminmenu ;;
	2\ *) do_find ;;
	3\ *) do_mod_menu ;;
	*) whiptail --msgbox "Programmer error: unrecognized option" 20 60 1 ;;
    esac || whiptail --msgbox "There was an error running option $FUN" 20 60 1
  else
    exit 1
  fi
done
}

#do_build
#do_menu
do_start
#do_find
