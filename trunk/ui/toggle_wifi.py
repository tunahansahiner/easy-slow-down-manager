#!/usr/bin/env python
# enable/disable wifi device
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
    if os.path.exists('/proc/easy_wifi_kill')==False:
        print("Easy slow down manager kernel module is not installed. Please, visit http://code.google.com/p/easy-slow-down-manager/ for mode details")
        sys.exit(1)
        
    f = open('/proc/easy_wifi_kill')
    s = int(f.read())
    f.close
    s = 1-s
    g = open('/proc/easy_wifi_kill','w')
    g.write(str(s))
    g.close
    if s==0:
      msg = "off"
    else:
      msg = "on"

    n = pynotify.Notification("Wifi state notification", "Wifi is "+msg+" now")
    n.set_urgency(pynotify.URGENCY_CRITICAL)
    n.set_timeout(1000) # 3 seconds
    n.set_category("device")

    #Call an icon
    helper = gtk.Button()
    icon = helper.render_icon(gtk.STOCK_DIALOG_WARNING, gtk.ICON_SIZE_DIALOG)
    n.set_icon_from_pixbuf(icon)

    if not n.show():
        print "Failed to send notification"
        sys.exit(1)

