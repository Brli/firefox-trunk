/* -*- Mode: javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
/* ***** BEGIN LICENSE BLOCK *****
 *	 Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is globalmenu-extension.
 *
 * The Initial Developer of the Original Code is
 * Canonical Ltd.
 * Portions created by the Initial Developer are Copyright (C) 2010
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * Chris Coulson <chris.coulson@canonical.com>
 * Nils Maier <maierman@web.de>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 * 
 * ***** END LICENSE BLOCK ***** */

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

(function unity_overrides() {
  "use strict";

  function enablePlacesNativeViewMenu(name) {
    // store the original ctor
    var menuCtor = window[name];

    // Override the original ctor, so that nativeView = true gets set
    // Note that somebody might have had the same stupid idea to override the ctor. ;)
    // So, try to interfere the least
    window[name] = function(aEvent) {
      var rootElt = aEvent.target;
      var viewElt = rootElt.parentNode;
      if (viewElt.parentNode.localName == "menubar") {
        this._nativeView = true;
        rootElt._startMarker = -1;
        rootElt._endMarker = -1;
      }
      menuCtor.apply(this, arguments);
    }
    // rewrite the prototype
    window[name].prototype = menuCtor.prototype;
  }

  enablePlacesNativeViewMenu("PlacesMenu");
  enablePlacesNativeViewMenu("HistoryMenu");

})();

(function unity_init() {
  "use strict";

  function $(id) document.getElementById(id);

  function Observer() {
    this.init();
  }
  Observer.prototype = {
    init: function() {
      this._os = "Services" in window
        ? Services.obs
         : Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService);
      this._os.addObserver(this, "menuservice-popup-open", false);

      this._menuService = Cc["@canonical.com/globalmenu-service;1"].getService(Ci.uIGlobalMenuService);
      this._menuService.registerNotification(this);

      if (this._menuService.online) {
        this.fixupUI(true);
      }
    },

    observe: function(subject, topic, data) {
      if (topic == "menuservice-popup-open") {
        if (data == "menu_EditPopup") {
          // This is really hacky, but the edit menu items only set the correct
          // sensitivity when the menupopup state == showing or open, which is
          // a read only property set in layout/xul/base/src/nsMenuPopupFrame.cpp
          // We can't do this off the popupshowing event, because we might run
          // before the handler hanging off the onpopupshowing attribute, which
          // will set the wrong sensitivity again, so we have our own notification
          // Uuuurgh! :(
          var saved_gEditUIVisible = gEditUIVisible;
          gEditUIVisible = true;
          goUpdateGlobalEditMenuItems();
          gEditUIVisible = saved_gEditUIVisible;
        }
      } else if (topic == "menuservice-online") {
        this.fixupUI(true);
      } else if (topic == "menuservice-offline") {
        this.fixupUI(false);
      }
    },

    fixupUI: function(online) {
      if (online == true) {
        this.autohideSaved = document.getElementById("toolbar-menubar").getAttribute("autohide");
        document.getElementById("toolbar-menubar").setAttribute("autohide", "false");
        document.getElementById("toolbar-menubar").removeAttribute("toolbarname");
      } else {
        document.getElementById("toolbar-menubar").setAttribute("autohide", this.autohideSaved);
        document.getElementById("toolbar-menubar").setAttribute("toolbarname", "&menubarCmd.label;");
      }

      updateAppButtonDisplay();
    },

    shutdown: function() {
      this._os.removeObserver(this, "menuservice-popup-open", false);
      this._menuService.unregisterNotification(this);
    }
  }


  var observer = null;

  addEventListener("load", function onLoad()
  {
    removeEventListener("load", onLoad, false);

    // XXX: This is just to start the menu loader, I can't figure out a way
    //      to start it without this (eg, on component registration)
    var loader = Cc["@canonical.com/globalmenu-loader;1"].getService(Ci.uIGlobalMenuLoader);
    if (!observer) {
      observer = new Observer();
    }
  }, false);

  addEventListener("unload", function onUnload()
  {
    removeEventListener("unload", onUnload, false);
    if (observer) {
      observer.shutdown();
      observer = null;
    }
  }, false);

})();