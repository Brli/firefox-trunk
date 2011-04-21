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

(function() {
  "use strict";

  function $(id) document.getElementById(id);

  function Observer() {
    this.init();
  }
  Observer.prototype = {
    init: function() {
      this.spinnerMoved = false;
      var menuService = Cc["@canonical.com/globalmenu-service;1"].getService(Ci.uIGlobalMenuService);
      menuService.registerNotification(this);
      if (menuService.online == true) {
        this.maybeMoveSpinner();
      }
    },

    observe: function(subject, topic, data) {
      if(topic == "menuservice-online") {
        this.maybeMoveSpinner();
      }
    },

    maybeMoveSpinner: function() {
      if (this.spinnerMoved == true) {
        return;
      }

      var menuBar = $("mail-toolbar-menubar2");
      if (!menuBar) {
        return;
      }

      var mailBar = $("mail-bar3");
      if (!mailBar || mailBar.hidden == true) {
        return;
      }

      var curSet = menuBar.currentSet;
      var throbberPos = curSet.indexOf("throbber-box");
      if (throbberPos == -1) {
        return;
      }

      if (throbberPos == 0) {
        var newSet = curSet.replace(/throbber-box,/,"");
      } else {
        var newSet = curSet.replace(/,throbber-box/,"");
      }

      menuBar.currentSet = newSet;

      curSet = mailBar.currentSet;
      newSet = curSet + ",throbber-box";
      mailBar.currentSet = newSet;

      this.spinnerMoved = true;
    },

    shutdown: function() {
      var menuService = Cc["@canonical.com/globalmenu-service;1"].getService(Ci.uIGlobalMenuService);
      menuService.unregisterNotification(this);
    }
  }

  const Cc = Components.classes;
  const Ci = Components.interfaces;

  var menuObserver = null;

  addEventListener("load", function onLoad() {
    removeEventListener("load", onLoad, false);

    // XXX: This is just to start the menu loader, I can't figure out a way
    //      to start it without this (eg, on component registration)
    var loader = Cc["@canonical.com/globalmenu-loader;1"].getService(Ci.uIGlobalMenuLoader);
    if (menuObserver == null) {
      menuObserver = new Observer();
    }
  }, false);

  addEventListener("load", function onUnload() {
    removeEventListener("unload", onUnload, false);
    if (menuObserver) {
      menuObserver.shutdown();
      menuObserver = null;
    }
  }, false);

})();
