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

const nsIObserverService = Ci.nsIObserverService;
const uIGlobalMenuLoader = Ci.uIGlobalMenuLoader;
const uIGlobalMenuService = Ci.uIGlobalMenuService;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

var observer = null;

window.addEventListener('load', onLoad, false);
window.addEventListener('unload', onUnload, false);

function Observer()
{
  this.init();
}

Observer.prototype = {
  init: function() {
    var os = Cc["@mozilla.org/observer-service;1"].getService(nsIObserverService);
    os.addObserver(this, "menuservice-popup-open", false);

    var menuService = Cc["@canonical.com/globalmenu-service;1"].getService(uIGlobalMenuService);
    menuService.registerNotification(this);

    if (menuService.online == true) {
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
    var os = Cc["@mozilla.org/observer-service;1"].getService(nsIObserverService);
    os.removeObserver(this, "menuservice-popup-open", false);

    var menuService = Cc["@canonical.com/globalmenu-service;1"].getService(uIGlobalMenuService);
    menuService.unregisterNotification(this);
  }
}

function onLoad()
{
  // XXX: This is just to start the menu loader, I can't figure out a way
  //      to start it without this (eg, on component registration)
  var loader = Cc["@canonical.com/globalmenu-loader;1"].getService(uIGlobalMenuLoader);
  if (!observer) {
    observer = new Observer();
  }
}

function onUnload()
{
  if (observer) {
    observer.shutdown();
    delete observer;
  }
}

// Note that we need to initialize _startMarker and _endMarker ourselves.
// This normally comes from XBL bindings on non-Mac platforms when the menu
// frame is drawn (which never happens here), or some #ifdef'd code on Mac.
// We also need to ensure that _nativeView is true before the menu is built,
// which happens normally when creating a new PlacesMenu or HistoryMenu. To
// do this, we subclass PlacesMenu and HistoryMenu and do the required
// initialization ourselves. We also override the popupshowing handlers to
// instantiate our classes

function PlacesMenuUnityImpl(aPopupShowingEvent, aPlace)
{
  this._rootElt = aPopupShowingEvent.target; // <menupopup>
  this._viewElt = this._rootElt.parentNode;   // <menu>
  this._viewElt._placesView = this;
  this._addEventListeners(this._rootElt, ["popupshowing", "popuphidden"], true);
  this._addEventListeners(window, ["unload"], false);

  if (this._viewElt.parentNode.localName == "menubar") {
    this._nativeView = true;
    this._rootElt._startMarker = -1;
    this._rootElt._endMarker = -1;
  }

  PlacesViewBase.call(this, aPlace);
  this._onPopupShowing(aPopupShowingEvent);
}

PlacesMenuUnityImpl.prototype = {
  __proto__: PlacesMenu.prototype
};

function HistoryMenuUnityImpl(aPopupShowingEvent)
{
  this.__proto__.__proto__.__proto__ = PlacesMenu.prototype;

  XPCOMUtils.defineLazyServiceGetter(this, "_ss",
                                     "@mozilla.org/browser/sessionstore;1",
                                     "nsISessionStore");

  this._rootElt = aPopupShowingEvent.target; // <menupopup>
  this._viewElt = this._rootElt.parentNode;   // <menu>
  this._viewElt._placesView = this;
  this._addEventListeners(this._rootElt, ["popupshowing", "popuphidden"], true);
  this._addEventListeners(window, ["unload"], false);

  if (this._viewElt.parentNode.localName == "menubar") {
    this._nativeView = true;
    this._rootElt._startMarker = -1;
    this._rootElt._endMarker = -1;
  }

  PlacesViewBase.call(this, "place:redirectsMode=2&sort=4&maxResults=10");
  this._onPopupShowing(aPopupShowingEvent);
}

HistoryMenuUnityImpl.prototype = {
  __proto__: HistoryMenu.prototype
};
