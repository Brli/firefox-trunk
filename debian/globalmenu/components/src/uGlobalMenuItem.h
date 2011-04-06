/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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

#ifndef _U_GLOBALMENUITEM_H
#define _U_GLOBALMENUITEM_H

#include <prtypes.h>
#include <nsCOMPtr.h>
#include <nsStringAPI.h>

#include <libdbusmenu-glib/server.h>

#include "uGlobalMenuObject.h"
#include "uMenuChangeObserver.h"

class nsIContent;
class uGlobalMenuDocListener;
class uGlobalMenuBar;

enum uMenuItemType {
  Normal,
  CheckBox,
  Radio
};

class uGlobalMenuItem: public uGlobalMenuObject,
                       public uMenuChangeObserver
{
public:
  NS_DECL_UMENUCHANGEOBSERVER

  static uGlobalMenuObject* Create(uGlobalMenuObject *aParent,
                                   uGlobalMenuDocListener *aListener,
                                   nsIContent *aContent,
                                   uGlobalMenuBar *aMenuBar);

private:
  uGlobalMenuItem(): uGlobalMenuObject(MenuItem) { };

  nsresult Init(uGlobalMenuObject *aParent,
                uGlobalMenuDocListener *aListener,
                nsIContent *aContent,
                uGlobalMenuBar *aMenuBar);
  ~uGlobalMenuItem();

  PRUint32 GetKeyCode(nsAString &aKeyName);
  PRUint32 MozKeyCodeToGdkKeyCode(PRUint32 aMozKeyCode);
  void SyncAccelFromContent();
  void SyncProperties();
  void SyncTypeAndStateFromContent();
  nsresult ConstructDbusMenuItem();
  static void ItemActivatedCallback(DbusmenuMenuitem *menuItem,
                                    PRUint32 timeStamp,
                                    void *data);
  void Activate();
  void UncheckSiblings();

  nsCOMPtr<nsIContent> mCommandContent;
  nsCOMPtr<nsIContent> mKeyContent;
  PRUint32 mHandlerID;
  PRBool mIsToggle;
  PRBool mToggleState;
  uMenuItemType mType;
};


#endif
