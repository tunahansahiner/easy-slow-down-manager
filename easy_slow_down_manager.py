#!/usr/bin/env python
# Easy slow down manager GUI
# this code is published under GPL license
# author Sergey Kobelkov

import gtk
import os.path

class StatusIcc:

    # activate callback
    def activate( self, widget, data=None):
	gtk.main_quit()
    # about dialog
    def about( self, widget, data=None):
        dialog = gtk.MessageDialog(
        parent         = None,
        flags          = gtk.DIALOG_DESTROY_WITH_PARENT,
        type           = gtk.MESSAGE_INFO,
        buttons        = gtk.BUTTONS_OK,
        message_format = "Easy slow down manager GUI\n\nPublished under GPL license, author Sergey Kobelkov\nFor details see\nhttp://code.google.com/p/easy-slow-down-manager/")
        dialog.set_title('About...')
        dialog.connect('response', self.hide)
        dialog.show()

    def show_error(self):
        print("Easy slow down manager kernel module is not installed. Please, visit http://code.google.com/p/easy-slow-down-manager/ for mode details")
        quit()

    # hide callback
    def  hide(self, widget,data= None):
                widget.hide()

    # context menu creation
    def _createMenu(self):
      self.menu = gtk.Menu()
      quit = gtk.ImageMenuItem(gtk.STOCK_QUIT)
      quit.connect("activate", gtk.main_quit)
      about = gtk.ImageMenuItem(gtk.STOCK_ABOUT)
      about.connect("activate", self.about)

      perf_sub = gtk.Menu()
      self.perf0 = gtk.CheckMenuItem('Low performance')
      self.perf0.set_draw_as_radio(True)
      self.perf0.connect("activate",self.set_performance,1)
      self.perf1 = gtk.CheckMenuItem('Normal performance')
      self.perf1.set_draw_as_radio(True)
      self.perf1.connect("activate",self.set_performance,0)
      self.perf2 = gtk.CheckMenuItem('High performance')
      self.perf2.set_draw_as_radio(True)
      self.perf2.connect("activate",self.set_performance,2)

      perf_sub.add(self.perf0)
      perf_sub.add(self.perf1)
      perf_sub.add(self.perf2)
      
      perf = gtk.MenuItem('Performance')
      perf.set_submenu(perf_sub)

      wifi_sub = gtk.Menu()
      self.wifi_on = gtk.CheckMenuItem('Wifi on')
      self.wifi_on.set_draw_as_radio(True)
      self.wifi_on.connect("activate",self.set_wifi,1)
      self.wifi_off = gtk.CheckMenuItem('Wifi off')
      self.wifi_off.set_draw_as_radio(True)
      self.wifi_off.connect("activate",self.set_wifi,0)

      wifi_sub.add(self.wifi_on)
      wifi_sub.add(self.wifi_off)

      wifi = gtk.MenuItem('Wifi')
      wifi.set_submenu(wifi_sub)

      self.menu.add(about)
      self.menu.add(perf)
      self.menu.add(wifi)
      self.menu.add(quit)
      self.menu.show_all()

    def set_performance(self, widget, perf, data=None):
      if widget.get_active():
        if widget!=self.perf0:
          self.perf0.set_active(False)
        if widget!=self.perf1:
          self.perf1.set_active(False)
        if widget!=self.perf2:
          self.perf2.set_active(False)
        f = open('/proc/easy_slow_down_manager','w')
        f.write(str(perf))
        f.close()

    def get_performance(self):
      f = open('/proc/easy_slow_down_manager')
      perf = f.read()
      f.close()
      if perf=="1":
        if self.perf0.get_active()==False:
          self.perf0.set_active(True)
      if perf=="0":
        if self.perf1.get_active()==False:
          self.perf1.set_active(True)
      if perf=="2":
        if self.perf2.get_active()==False:
          self.perf2.set_active(True)

    def set_wifi(self, widget, wifi, data=None):
      if widget.get_active():
        if widget!=self.wifi_on:
          self.wifi_on.set_active(False)
        if widget!=self.wifi_off:
          self.wifi_off.set_active(False)
        f = open('/proc/easy_wifi_kill','w')
        f.write(str(wifi))
        f.close
    
    def get_wifi(self):
      f = open('/proc/easy_wifi_kill')
      wifi = f.read()
      f.close()
      if wifi=="1":
        if self.wifi_on.get_active()==False:
          self.wifi_on.set_active(True)
      if wifi=="0":
        if self.wifi_off.get_active()==False:
          self.wifi_off.set_active(True)

    def _showMenu(self, status_icon, button, activate_time):
      self.get_performance()
      self.get_wifi()
      self.menu.popup(None, None, gtk.status_icon_position_menu, button, activate_time, status_icon)

    # popup callback
    def popup(self, widget, button, time, data=None):
        self._showMenu(self.staticon, button, time)

    def __init__(self):
        # check if a module is present
        if (os.path.exists('/proc/easy_wifi_kill')==False) or (os.path.exists('/proc/easy_slow_down_manager')==False):
          self.show_error() 
        # create a new Status Icon
        self.staticon = gtk.StatusIcon()
        self.staticon.set_from_stock(gtk.STOCK_PROPERTIES)
        self.staticon.set_blinking(False)
        #self.staticon.connect("activate", self.activate)
        self.staticon.connect("popup_menu", self.popup)
        self.staticon.set_tooltip('Easy slow down utility')
        self.staticon.set_visible(True)
	self._createMenu()

        # invoking the main()
        gtk.main()

if __name__ == "__main__":
    statusicon = StatusIcc()

