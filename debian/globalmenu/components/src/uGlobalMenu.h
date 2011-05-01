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

#ifndef _U_GLOBALMENU_H
#define _U_GLOBALMENU_H

#include <prtypes.h>
#include <nsTArray.h>
#include <nsAutoPtr.h>
#include <nsCOMPtr.h>
#include <nsIContent.h>

#include <libdbusmenu-glib/server.h>

#include "uGlobalMenuObject.h"
#include "uMenuChangeObserver.h"

class uGlobalMenuItem;
class uGlobalMenuBar;
class uGlobalMenuDocListener;

class uGlobalMenu: public uGlobalMenuObject,
                   public uMenuChangeObserver
{
  friend class uGlobalMenuItem;
  friend class uGlobalMenuBar;
public:
  NS_DECL_UMENUCHANGEOBSERVER

  static uGlobalMenuObject* Create(uGlobalMenuObject *aParent,
                                   uGlobalMenuDocListener *aListener,
                                   nsIContent *aContent,
                                   uGlobalMenuBar *aMenuBar);

  ~uGlobalMenu();

  void OpenMenu();

private:
  uGlobalMenu(): uGlobalMenuObject(Menu), mPopupBound(PR_FALSE) { };

  // Initialize the menu structure
  nsresult Init(uGlobalMenuObject *aParent,
                uGlobalMenuDocListener *aListener,
                nsIContent *aContent,
                uGlobalMenuBar *aMenuBar);
  void InsertMenuObjectAt(uGlobalMenuObject *menuObj,
                          PRUint32 index);
  void AppendMenuObject(uGlobalMenuObject *menuObj);
  void RemoveMenuObjectAt(PRUint32 index);
  nsresult ConstructDbusMenuItem();
  void Rebuild();
  nsresult Build();
  void GetMenuPopupFromMenu(nsIContent **aResult);
  static PRBool MenuAboutToOpenCallback(DbusmenuMenuitem *menu,
                                        void *data);
  static PRBool MenuEventCallback(DbusmenuMenuitem *menu,
                                  const gchar *name,
                                  GVariant *value,
                                  guint timestamp,
                                  void *data);
  PRBool CanOpen();
  void AboutToOpen();
  void OnOpen();
  void OnClose();
  void Activate();
  void Deactivate();

  nsCOMPtr<nsIContent> mPopupContent;
  nsCOMPtr<nsIContent> mCommandContent;
  nsTArray< nsAutoPtr<uGlobalMenuObject> > mMenuObjects;
  PRUint32 mOpenHandlerID;
  PRUint32 mEventHandlerID;
  PRBool mPopupBound;
};

#endif