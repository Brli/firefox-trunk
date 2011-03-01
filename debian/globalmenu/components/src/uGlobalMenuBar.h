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

#ifndef _U_GLOBALMENUBAR_H
#define _U_GLOBALMENUBAR_H

#include <prtypes.h>
#include <nsTArray.h>
#include <nsAutoPtr.h>
#include <nsStringAPI.h>
#include <nsIDOMEventTarget.h>
#include <nsIContent.h>
#include <nsIDOMFocusListener.h>
#include <nsIDOMKeyListener.h>

#include <libdbusmenu-glib/server.h>
#include <gtk/gtk.h>

#include "uMenuChangeObserver.h"
#include "uGlobalMenuObject.h"

class nsIObserver;
class nsIWidget;
class nsIDOMEvent;
class uGlobalMenuService;
class uGlobalMenu;
class uGlobalMenuBar;
class nsIDOMKeyEvent;

class uGlobalMenuBarListener: public nsIDOMKeyListener,
                              public nsIDOMFocusListener
{
public:
  NS_DECL_ISUPPORTS

  NS_IMETHOD KeyPress (nsIDOMEvent *aKeyEvent);
  NS_IMETHOD KeyUp (nsIDOMEvent *aKeyEvent) { return NS_OK; }
  NS_IMETHOD KeyDown (nsIDOMEvent *aKeyEvent) { return NS_OK; }

  NS_IMETHOD Focus (nsIDOMEvent *aEvent);
  NS_IMETHOD Blur (nsIDOMEvent *aEvent) { return NS_OK; }
  NS_IMETHOD HandleEvent (nsIDOMEvent *aEvent) { return NS_OK; }

  uGlobalMenuBarListener (uGlobalMenuBar *aMenuBar):
                          mMenuBar(aMenuBar) { };
  ~uGlobalMenuBarListener () { };

private:
  uGlobalMenuBar *mMenuBar;
};

class uGlobalMenuBar: public uGlobalMenuObject,
                      public uMenuChangeObserver
{
  friend class uGlobalMenuBarListener;
public:
  NS_DECL_UMENUCHANGEOBSERVER

  static uGlobalMenuBar* Create(nsIWidget *aWindow,
                                nsIContent *aMenuBar);
  ~uGlobalMenuBar ();

  // Return the native ID of the window
  PRUint32 GetWindowID ();

  // Returns the path of the menubar on the session bus
  const char* GetMenuPath ();

  // Checks if the menubar shares the same top level window as the 
  // specified nsIWidget
  PRBool WidgetHasSameToplevelWindow (nsIWidget *aWidget);

  void ChildPopupOpen (uGlobalMenu *aMenu);
  void ChildPopupClosed (uGlobalMenu *aMenu);

  PRBool OpenedByKeyboard() { return mOpenedByKeyboard; }

  // Called from the menu service. Used to hide the DOM element for the menubar
  void SetXULMenuBarHidden (PRBool hidden);

protected:
  void Focus ();
  nsresult KeyPress (nsIDOMEvent *aKeyEvent);
private:
  uGlobalMenuBar (): uGlobalMenuObject(MenuBar),
                     mServer(nsnull),
                     mTopLevel(nsnull),
                     mOpenMenu(nsnull),
                     mOpenedByKeyboard(PR_FALSE),
                     mOpenMenuPending(PR_FALSE) { };
  // Initialize the menu structure
  nsresult Init (nsIWidget *aWindow,
                 nsIContent *aMenuBar);

  GtkWidget* WidgetToGTKWindow (nsIWidget *aWidget);
  nsresult Build ();
  PRUint32 GetModifiersFromEvent (nsIDOMKeyEvent *aKeyEvent);

  void RemoveMenuObjectAt (PRUint32 index);
  void InsertMenuObjectAt (uGlobalMenuObject *menu,
                           PRUint32 index);
  void AppendMenuObject (uGlobalMenuObject *menu);
  PRBool ShouldParentStayVisible (nsIContent *aContent);
  PRBool IsParentOfMenuBar (nsIContent *aContent);

  DbusmenuServer *mServer;
  GtkWidget *mTopLevel;
  nsCString mPath;

  nsCOMPtr<nsIDOMEventTarget> mDOMWinTarget;
  nsCOMPtr<nsIContent> mHiddenElement;
  nsCOMPtr<nsIDOMEventTarget> mDocTarget;
  PRBool mRestoreHidden;
  PRBool mXULMenuHidden;
  nsRefPtr<uGlobalMenuBarListener> mEventListener;
  PRUint32 mAccessKeyMask;
  uGlobalMenu *mOpenMenu;
  PRBool mOpenedByKeyboard;
  PRBool mOpenMenuPending;

  // Should probably have a container class and subclass that
  nsTArray< nsAutoPtr<uGlobalMenuObject> > mMenuObjects;
};

#endif
