#!/usr/bin/env python
# enable/disable backlight
# author Kobelkov Sergey
# license GPL, 2010

try:
    import gtk, pygtk, os, os.path, pynotify
    pygtk.require('2.0')
except:
    print "Error: need python-notify, python-gtk2 and gtk"

if __name__ == '__main__':
    if not pynotify.init("Timekpr notification"):
        sys.exit(1)
    if os.path.exists('/proc/easy_backlight')==False:
        print("Easy slow down manager kernel module is not installed. Please, visit http://code.google.com/p/easy-slow-down-manager/ for mode details")
        sys.exit(1)
        
    f = open('/proc/easy_backlight')
    s = int(f.read())
    f.close
    s = 1-s
    g = open('/proc/easy_backlight','w')
    g.write(str(s))
    g.close

