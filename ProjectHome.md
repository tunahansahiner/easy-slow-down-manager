**Now all this stuff is implemented by samsung\_laptop kernel module**

Easy slow down manager provides Linux users with functionality similar to "Samsung Easy Speed Up Manager". It also allows to turn WiFi on and off.

It consist of kernel module named easy\_slow\_down\_manager. To use it first issue "modprobe easy\_slow\_down\_manager". If everything goes fine you get /proc/easy\_slow\_down\_manager and /proc/easy\_wifi\_kill files.

  * "echo 0 > /proc/easy\_slow\_down\_manager" turns laptop to "normal" mode.
  * "echo 1 > /proc/easy\_slow\_down\_manager" turns laptop to "silent" mode.
  * "echo 2 > /proc/easy\_slow\_down\_manager" turns laptop to "speed" mode.
  * "echo 0 > /proc/easy\_wifi\_kill" turns WiFi off.
  * "echo 1 > /proc/easy\_wifi\_kill" turns WiFi on.

To build the module you need linux kernel source. Unpack module archive and issue:

``make -C /path/to/linux/source M=`pwd` modules``

This use to work on most linux distros:

``make -C /lib/modules/`uname -r`/build M=`pwd` modules``