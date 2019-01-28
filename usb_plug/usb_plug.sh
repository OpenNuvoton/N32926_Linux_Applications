#!/bin/sh

KERNEL_VERSION=`cat /proc/version | awk '{print $3}'`
MOUNT_TABLE="/tmp/mounts"
MASS_UNPLUG="/tmp/usb_unplug.sh"

if [ -f $MOUNT_TABLE ] ; then
	echo "$MOUNT_TABLE is existed, remove it"
	rm -f $MOUNT_TABLE
fi
cp /proc/mounts $MOUNT_TABLE

if [ -f $MASS_UNPLUG ] ; then
	echo "$MASS_UNPLUG is existed, remove it"
	rm -f $MASS_UNPLUG
fi

echo "#!/bin/sh" > $MASS_UNPLUG 
echo >> $MASS_UNPLUG 
if [ $KERNEL_VERSION = "2.6.17.14" ]; then
	killall -9 mass
	echo "killall -9 mass" >> $MASS_UNPLUG 
else
	echo "rmmod g_file_storage" >> $MASS_UNPLUG
fi
chmod +x $MASS_UNPLUG

awk -v MASS_UNPLUG=$MASS_UNPLUG ' 
{
	if ($0 ~ /^\/dev\/loop/) {
		print "umount " $2;
		if (system("umount " $2)) {
			print "umount fail!";
		}
	}
} ' $MOUNT_TABLE

awk -v MASS_UNPLUG=$MASS_UNPLUG -v KERNEL_VERSION=$KERNEL_VERSION ' 
{
	if ($0 ~ /^\/dev\/sd/ || $0 ~ /^\/dev\/mmcblk/) {
		print "umount " $2;
		if (system("umount " $2)) {
			print "umount fail!";
		} else {
			if (plug_devs) {
				if (KERNEL_VERSION == "2.6.17.14") {
					plug_devs = plug_devs " " $1;
				} else {
					plug_devs = plug_devs "," $1;
				}
			} else {
				plug_devs = $1;
			}
			sub(/,codepage=[a-z|0-9]+/, "", $4);
			system("echo mount -t " $3 " -o " $4 " " $1 " " $2 " >> " MASS_UNPLUG);
			print "plug_devs: \"" plug_devs "\"";
		}
	}
}
END {
	if (KERNEL_VERSION == "2.6.17.14") {
		system("echo \"/usr/mass /dev/null&\" >> " MASS_UNPLUG);
		if (plug_devs == "") {
			plug_devs = "/dev/null";
		}
		print "/usr/mass " plug_devs " &";
		system("/usr/mass " plug_devs " &");
	} else {
		print "insmod /usr/g_file_storage.ko file=" plug_devs " stall=0 removable=1 &";
		system("insmod /usr/g_file_storage.ko file=" plug_devs " stall=0 removable=1 &");
	}
} ' $MOUNT_TABLE
