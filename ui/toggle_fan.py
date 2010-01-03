#!/usr/bin/env python
# easy slow down manager ui 
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
    if os.path.exists('/proc/easy_slow_down_manager')==False:
        print("Easy slow down manager kernel module is not installed. Please, visit http://code.google.com/p/easy-slow-down-manager/ for mode details")
        sys.exit(1)
        
    f = open('/proc/easy_slow_down_manager')
    s = int(f.read())
    f.close
    s = (s+1)%3
    g = open('/proc/easy_slow_down_manager','w')
    g.write(str(s))
    g.close
    if s==0:
      msg = "normal"
    if s==1:
      msg = "silent"
    if s==2:
      msg = "speed"

    n = pynotify.Notification("Easy slow down manager notification", "Changed system state to "+msg)
    n.set_urgency(pynotify.URGENCY_CRITICAL)
    n.set_timeout(500) # half a second
    n.set_category("device")

    #Call an icon
    helper = gtk.Button()
    icon = helper.render_icon(gtk.STOCK_DIALOG_WARNING, gtk.ICON_SIZE_DIALOG)
    n.set_icon_from_pixbuf(icon)

    if not n.show():
        print "Failed to send notification"
        sys.exit(1)

