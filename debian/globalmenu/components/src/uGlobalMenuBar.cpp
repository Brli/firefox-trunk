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

#include <nsDebug.h>
#include <nsIObserver.h>
#include <nsIWidget.h>
#include <nsServiceManagerUtils.h>
#include <nsIDOMWindow.h>
#include <nsIXULWindow.h>
#include <nsIInterfaceRequestorUtils.h>
#include <nsIDocShell.h>
#include <nsIDOMNSEvent.h>
#include <nsIPrefBranch.h>
#include <nsIPrefService.h>
#include <nsIDOMKeyEvent.h>
#include <nsIDOMEventListener.h>
#include <nsICaseConversion.h>
#include <nsUnicharUtilCIID.h>

#include <glib-object.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include "uGlobalMenuBar.h"
#include "uGlobalMenu.h"
#include "uGlobalMenuFactory.h"
#include "uIGlobalMenuService.h"
#include "uWidgetAtoms.h"

#define MODIFIER_SHIFT    1
#define MODIFIER_CONTROL  2
#define MODIFIER_ALT      4
#define MODIFIER_META     8

NS_IMPL_ADDREF(uGlobalMenuBarListener)
NS_IMPL_RELEASE(uGlobalMenuBarListener)
NS_INTERFACE_MAP_BEGIN(uGlobalMenuBarListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMKeyListener)
  NS_INTERFACE_MAP_ENTRY(nsIDOMFocusListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsIDOMEventListener,nsIDOMKeyListener)
NS_INTERFACE_MAP_END

NS_IMETHODIMP
uGlobalMenuBarListener::KeyPress(nsIDOMEvent *aKeyEvent)
{
  return mMenuBar->KeyPress(aKeyEvent);
}

NS_IMETHODIMP
uGlobalMenuBarListener::KeyUp(nsIDOMEvent *aKeyEvent)
{
  return mMenuBar->KeyUp(aKeyEvent);
}

NS_IMETHODIMP
uGlobalMenuBarListener::KeyDown(nsIDOMEvent *aKeyEvent)
{
  return mMenuBar->KeyDown(aKeyEvent);
}

NS_IMETHODIMP
uGlobalMenuBarListener::Blur(nsIDOMEvent *aEvent)
{
  if (mMenuBar) {
    mMenuBar->Blur();
  }
  return NS_OK;
}

NS_IMETHODIMP
uGlobalMenuBarListener::Focus(nsIDOMEvent *aEvent)
{
  if (mMenuBar) {
    mMenuBar->Focus();
  }
  return NS_OK;
}

GtkWidget*
uGlobalMenuBar::WidgetToGTKWindow(nsIWidget *aWidget)
{
  // Get the main GDK drawing window from our nsIWidget
  GdkWindow *window = static_cast<GdkWindow *>(aWidget->GetNativeData(NS_NATIVE_WINDOW));
  if (!window)
    return nsnull;

  // Get the widget for the main drawing window, which should be a MozContainer
  gpointer user_data = nsnull;
  gdk_window_get_user_data(window, &user_data);
  if (!user_data || !GTK_IS_CONTAINER(user_data))
    return nsnull;

  return gtk_widget_get_toplevel(GTK_WIDGET(user_data));
}

void
uGlobalMenuBar::AppendMenuObject(uGlobalMenuObject *menu)
{
  dbusmenu_menuitem_child_append(mDbusMenuItem,
                                 menu->GetDbusMenuItem());

  mMenuObjects.AppendElement(menu);
}

void
uGlobalMenuBar::InsertMenuObjectAt(uGlobalMenuObject *menu,
                                   PRUint32 index)
{
  dbusmenu_menuitem_child_add_position(mDbusMenuItem,
                                       menu->GetDbusMenuItem(),
                                       index);

  mMenuObjects.InsertElementAt(index, menu);
}

void
uGlobalMenuBar::RemoveMenuObjectAt(PRUint32 index)
{
  dbusmenu_menuitem_child_delete(mDbusMenuItem,
                                 mMenuObjects[index]->GetDbusMenuItem());

  mMenuObjects.RemoveElementAt(index);
}

nsresult
uGlobalMenuBar::Build()
{
  PRUint32 count = mContent->GetChildCount();

  for (PRUint32 i = 0; i < count; i++) {
    nsIContent *menuContent = mContent->GetChildAt(i);
    uGlobalMenuObject *newItem =
      NewGlobalMenuItem(static_cast<uGlobalMenuObject *>(this),
                        mListener, menuContent, this);
    if (!newItem) {
      return NS_ERROR_FAILURE;
    }
    AppendMenuObject(newItem);
  }
  return NS_OK;
}

nsresult
uGlobalMenuBar::Init(nsIWidget *aWindow,
                     nsIContent *aMenuBar)
{
  NS_ENSURE_ARG(aWindow);
  NS_ENSURE_ARG(aMenuBar);

  mContent = aMenuBar;

  mTopLevel = WidgetToGTKWindow(aWindow);
  if (!GTK_IS_WINDOW(mTopLevel))
    return NS_ERROR_FAILURE;

  g_object_ref(mTopLevel);

  mPath = NS_LITERAL_CSTRING("/com/canonical/menu/");
  char xid[10];
  sprintf(xid, "%X", (PRUint32) GDK_WINDOW_XID(gtk_widget_get_window(mTopLevel)));
  mPath.Append(xid);

  mServer = dbusmenu_server_new(mPath.get());
  if (!mServer)
    return NS_ERROR_OUT_OF_MEMORY;

  mDbusMenuItem = dbusmenu_menuitem_new();
  if (!mDbusMenuItem)
    return NS_ERROR_OUT_OF_MEMORY;

  dbusmenu_server_set_root(mServer, mDbusMenuItem);

  mListener = new uGlobalMenuDocListener();
  if (!mListener)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = mListener->Init(mContent);
  NS_ENSURE_SUCCESS(rv, rv);

  mListener->RegisterForContentChanges(mContent, this);

  rv = Build();
  NS_ENSURE_SUCCESS(rv, rv);

  // Find the top-level DOM window from our nsIWidget, so we
  // can register the menubar as a focus event listener, in order
  // for it to cancel menus when it the window gets focus
  void *clientData;
  aWindow->GetClientData(clientData);
  nsISupports *data = static_cast<nsISupports *>(clientData);
  nsCOMPtr<nsIXULWindow> xulWindow = do_QueryInterface(data);
  NS_ENSURE_TRUE(xulWindow, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDocShell> docShell;
  xulWindow->GetDocShell(getter_AddRefs(docShell));
  NS_ENSURE_TRUE(docShell, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDOMWindow> domWindow = do_GetInterface(docShell);
  mDOMWinTarget = do_QueryInterface(domWindow);

  mEventListener = new uGlobalMenuBarListener(this);
  NS_ENSURE_TRUE(mEventListener, NS_ERROR_OUT_OF_MEMORY);

  mDOMWinTarget->AddEventListener(NS_LITERAL_STRING("focus"),
                                  (nsIDOMFocusListener *)mEventListener,
                                  PR_TRUE);
  mDOMWinTarget->AddEventListener(NS_LITERAL_STRING("blur"),
                                  (nsIDOMFocusListener *)mEventListener,
                                  PR_FALSE);

  mDocTarget = do_QueryInterface(mContent->GetDocument());

  mDocTarget->AddEventListener(NS_LITERAL_STRING("keypress"),
                               (nsIDOMKeyListener *)mEventListener,
                               PR_FALSE);
  mDocTarget->AddEventListener(NS_LITERAL_STRING("keydown"),
                               (nsIDOMKeyListener *)mEventListener,
                               PR_FALSE);
  mDocTarget->AddEventListener(NS_LITERAL_STRING("keyup"),
                               (nsIDOMKeyListener *)mEventListener,
                               PR_FALSE);

  nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
  NS_ENSURE_TRUE(prefs, NS_ERROR_FAILURE);

  prefs->GetIntPref("ui.key.menuAccessKey", &mAccessKey);
  if (mAccessKey == nsIDOMKeyEvent::DOM_VK_SHIFT) {
    mAccessKeyMask = MODIFIER_SHIFT;
  } else if (mAccessKey == nsIDOMKeyEvent::DOM_VK_CONTROL) {
    mAccessKeyMask = MODIFIER_CONTROL;
  } else if (mAccessKey == nsIDOMKeyEvent::DOM_VK_ALT) {
    mAccessKeyMask = MODIFIER_ALT;
  } else if (mAccessKey == nsIDOMKeyEvent::DOM_VK_META) {
    mAccessKeyMask = MODIFIER_META;
  } else {
    mAccessKeyMask = MODIFIER_ALT;
  }

  mListener->RegisterForAllChanges(this);

  nsCOMPtr<uIGlobalMenuService> service =
    do_GetService("@canonical.com/globalmenu-service;1");
  NS_ENSURE_TRUE(service, NS_ERROR_FAILURE);

  service->RegisterGlobalMenuBar(this);
  return NS_OK;
}

PRBool
uGlobalMenuBar::ShouldParentStayVisible(nsIContent *aContent)
{
  static nsIAtom *blacklist[] =
    { uWidgetAtoms::toolbarspring, nsnull };

  nsIContent *parent = aContent->GetParent();
  if (!parent) {
    return PR_TRUE;
  }

  PRUint32 count = parent->GetChildCount();

  if (count <= 1) {
    // It's just us
    return PR_FALSE;
  }

  for (PRUint32 i = 0 ; i < count ; i++) {
    nsIContent *child = parent->GetChildAt(i);
    if (child == aContent) {
      continue;
    }

    PRBool found = PR_FALSE;
    for (PRUint32 j = 0 ; blacklist[j] != nsnull ; j++) {
      if (child->Tag() == blacklist[j]) {
        found = PR_TRUE;
        break;
      }
    }

    if (!found) {
      return PR_TRUE;
    }
  }

  return PR_FALSE;
}

PRUint32
uGlobalMenuBar::GetModifiersFromEvent(nsIDOMKeyEvent *aKeyEvent)
{
  PRUint32 modifiers = 0;
  PRBool modifier;

  aKeyEvent->GetAltKey(&modifier);
  if (modifier) {
    modifiers |= MODIFIER_ALT;
  }

  aKeyEvent->GetShiftKey(&modifier);
  if (modifier) {
    modifiers |= MODIFIER_SHIFT;
  }

  aKeyEvent->GetCtrlKey(&modifier);
  if (modifier) {
    modifiers |= MODIFIER_CONTROL;
  }

  aKeyEvent->GetMetaKey(&modifier);
  if (modifier) {
    modifiers |= MODIFIER_META;
  }

  return modifiers;
}

PRBool
uGlobalMenuBar::IsParentOfMenuBar(nsIContent *aContent)
{
  nsIContent *tmp = mContent->GetParent();

  while (tmp) {
    if (tmp == aContent) {
      return PR_TRUE;
    }

    tmp = tmp->GetParent();
  }

  return PR_FALSE;
}

uGlobalMenuBar::uGlobalMenuBar():
  uGlobalMenuObject(MenuBar), mServer(nsnull), mTopLevel(nsnull),
  mOpenedByKeyboard(PR_FALSE)
{
  MOZ_COUNT_CTOR(uGlobalMenuBar);
}

uGlobalMenuBar::~uGlobalMenuBar()
{
  mListener->UnregisterForAllChanges(this);
  mListener->UnregisterForContentChanges(mContent);

  SetXULMenuBarHidden(PR_FALSE);

  mDOMWinTarget->RemoveEventListener(NS_LITERAL_STRING("focus"),
                                     (nsIDOMFocusListener *)mEventListener,
                                     PR_TRUE);
  mDOMWinTarget->RemoveEventListener(NS_LITERAL_STRING("blur"),
                                     (nsIDOMFocusListener *)mEventListener,
                                     PR_FALSE);

  mDocTarget->RemoveEventListener(NS_LITERAL_STRING("keypress"),
                                  (nsIDOMKeyListener *)mEventListener,
                                  PR_FALSE);
  mDocTarget->RemoveEventListener(NS_LITERAL_STRING("keydown"),
                                  (nsIDOMKeyListener *)mEventListener,
                                  PR_FALSE);
  mDocTarget->RemoveEventListener(NS_LITERAL_STRING("keyup"),
                                  (nsIDOMKeyListener *)mEventListener,
                                  PR_FALSE);

  mListener->Destroy();

  if (mTopLevel)
    g_object_unref(mTopLevel);

  if (mDbusMenuItem)
    g_object_unref(mDbusMenuItem);

  if (mServer)
    g_object_unref(mServer);

  MOZ_COUNT_DTOR(uGlobalMenuBar);
}

/*static*/ uGlobalMenuBar*
uGlobalMenuBar::Create(nsIWidget *aWindow,
                       nsIContent *aMenuBar)
{
  uGlobalMenuBar *menubar = new uGlobalMenuBar();
  if (!menubar) {
    return nsnull;
  }

  if (NS_FAILED(menubar->Init(aWindow, aMenuBar))) {
    delete menubar;
    return nsnull;
  }

  return menubar;
}

void
uGlobalMenuBar::Blur()
{
  dbusmenu_server_set_status(mServer, DBUSMENU_STATUS_NORMAL);
}

void
uGlobalMenuBar::Focus()
{
  mOpenedByKeyboard = PR_FALSE;
}

PRBool
uGlobalMenuBar::ShouldHandleKeyEvent(nsIDOMEvent *aKeyEvent)
{
  nsCOMPtr<nsIDOMNSEvent> nsEvent = do_QueryInterface(aKeyEvent);
  if (!nsEvent) {
    return PR_FALSE;
  }

  PRBool handled, trusted;
  nsEvent->GetPreventDefault(&handled);
  nsEvent->GetIsTrusted(&trusted);

  if (handled || !trusted) {
    return PR_FALSE;
  }

  return PR_TRUE;
}

nsresult
uGlobalMenuBar::KeyDown(nsIDOMEvent *aKeyEvent)
{
  if (!ShouldHandleKeyEvent(aKeyEvent)) {
    return NS_OK;
  }

  nsCOMPtr<nsIDOMKeyEvent> keyEvent = do_QueryInterface(aKeyEvent);
  if (keyEvent) {
    PRUint32 keyCode;
    keyEvent->GetKeyCode(&keyCode);
    PRUint32 modifiers = GetModifiersFromEvent(keyEvent);
    if ((keyCode == static_cast<PRUint32>(mAccessKey)) &&
        ((modifiers & ~mAccessKeyMask) == 0)) {
      dbusmenu_server_set_status(mServer, DBUSMENU_STATUS_NOTICE);
    }
  }

  return NS_OK;
}

nsresult
uGlobalMenuBar::KeyUp(nsIDOMEvent *aKeyEvent)
{
  if (!ShouldHandleKeyEvent(aKeyEvent)) {
    return NS_OK;
  }

  nsCOMPtr<nsIDOMKeyEvent> keyEvent = do_QueryInterface(aKeyEvent);
  if (keyEvent) {
    PRUint32 keyCode;
    keyEvent->GetKeyCode(&keyCode);
    if (keyCode == static_cast<PRUint32>(mAccessKey)) {
      dbusmenu_server_set_status(mServer, DBUSMENU_STATUS_NORMAL);
    }
  }

  return NS_OK;
}

nsresult
uGlobalMenuBar::KeyPress(nsIDOMEvent *aKeyEvent)
{
  if (!ShouldHandleKeyEvent(aKeyEvent)) {
    return NS_OK;
  }

  nsCOMPtr<nsIDOMKeyEvent> keyEvent = do_QueryInterface(aKeyEvent);
  uGlobalMenuObject *found = nsnull;
  PRUint32 keyCode = nsnull;

  if (keyEvent) {
    keyEvent->GetKeyCode(&keyCode);
    PRUint32 count = mMenuObjects.Length();
    PRUint32 modifiers = GetModifiersFromEvent(keyEvent);
    if ((modifiers & mAccessKeyMask) && ((modifiers & ~mAccessKeyMask) == 0)) {
      // The menu access modifier is pressed
      PRUint32 charCode;
      keyEvent->GetCharCode(&charCode);
      if (charCode != 0) {
        PRUnichar ch = PRUnichar(charCode);
        PRUnichar chl;
        PRUnichar chu;
        // XXX: I think we need to link against libxul.so to get ToLowerCase
        //      and ToUpperCase from nsUnicharUtils.h
        nsCOMPtr<nsICaseConversion> converter =
          do_GetService(NS_UNICHARUTIL_CONTRACTID);
        if (converter) {
          converter->ToUpper(ch, &chu);
          converter->ToLower(ch, &chl);
        } else {
          if (ch < 256) {
            chu = toupper(char(ch));
            chl = tolower(char(ch));
          } else {
            chu = ch;
            chl = ch;
          }
        }

        for (PRUint32 i = 0; i < count; i++) {
          nsCOMPtr<nsIContent> content;
          mMenuObjects[i]->GetContent(getter_AddRefs(content));
          if (content) {
            nsAutoString accessKey;
            content->GetAttr(kNameSpaceID_None, uWidgetAtoms::accesskey,
                             accessKey);
            const PRUnichar *key = accessKey.BeginReading();
            if (*key == chl || *key == chu) {
              found = mMenuObjects[i];
              break;
            }
          }
        }
      }
    } else if (keyCode == nsIDOMKeyEvent::DOM_VK_F10) {
      // Go through each mMenuObject, and find the first one
      // that is both visible and sensitive, and mark it found
      // for opening.
      for (PRUint32 i = 0; i < count; i++) {
        uGlobalMenu *menu = static_cast<uGlobalMenu *>((uGlobalMenuObject *)mMenuObjects[i]);
        if (menu->CanOpen()) {
          found = mMenuObjects[i];
          break;
        }
      }
    }
  }

  if (found) {
    mOpenedByKeyboard = PR_TRUE;
    uGlobalMenu *menu = static_cast<uGlobalMenu *>(found);
    menu->OpenMenu();
    aKeyEvent->StopPropagation();
    aKeyEvent->PreventDefault();
  }

  return NS_OK;
}

void
uGlobalMenuBar::SetXULMenuBarHidden(PRBool hidden)
{
  mXULMenuHidden = hidden;

  if (hidden) {
    if (mHiddenElement) {
      mHiddenElement->SetAttr(kNameSpaceID_None, uWidgetAtoms::hidden,
                              mRestoreHidden ? NS_LITERAL_STRING("true") :
                              NS_LITERAL_STRING("false"), PR_TRUE);
    }
    nsIContent *tmp = mContent;

    // Walk up the DOM tree until we find a node with siblings
    while (tmp) {
      if (ShouldParentStayVisible(tmp)) {
        break;
      }

      tmp = tmp->GetParent();
    }

    mHiddenElement = tmp;
    mRestoreHidden = mHiddenElement->AttrValueIs(kNameSpaceID_None,
                                                 uWidgetAtoms::hidden,
                                                 uWidgetAtoms::_true,
                                                 eCaseMatters);

    mHiddenElement->SetAttr(kNameSpaceID_None, uWidgetAtoms::hidden,
                            NS_LITERAL_STRING("true"), PR_TRUE);
  } else if (mHiddenElement) {
    mHiddenElement->SetAttr(kNameSpaceID_None, uWidgetAtoms::hidden,
                            mRestoreHidden ? NS_LITERAL_STRING("true") :
                            NS_LITERAL_STRING("false"), PR_TRUE);
    mHiddenElement = nsnull;
  }
}

PRUint32
uGlobalMenuBar::GetWindowID()
{
  return (PRUint32) GDK_WINDOW_XID(gtk_widget_get_window(mTopLevel));
}

const char*
uGlobalMenuBar::GetMenuPath()
{
  return mPath.get();
}

PRBool
uGlobalMenuBar::WidgetHasSameToplevelWindow(nsIWidget *aWidget)
{
  GtkWidget *topLevel = WidgetToGTKWindow(aWidget);
  return GDK_WINDOW_XID(gtk_widget_get_window(mTopLevel)) == GDK_WINDOW_XID(gtk_widget_get_window(topLevel));
}

void
uGlobalMenuBar::ObserveAttributeChanged(nsIDocument *aDocument,
                                        nsIContent *aContent,
                                        nsIAtom *aAttribute)
{

}

void
uGlobalMenuBar::ObserveContentRemoved(nsIDocument *aDocument,
                                      nsIContent *aContainer,
                                      nsIContent *aChild,
                                      PRInt32 aIndexInContainer)
{
  if (IsParentOfMenuBar(aContainer)) {
    SetXULMenuBarHidden(mXULMenuHidden);
    return;
  }

  if (aContainer != mContent) {
    return;
  }

  RemoveMenuObjectAt(aIndexInContainer);
}

void
uGlobalMenuBar::ObserveContentInserted(nsIDocument *aDocument,
                                       nsIContent *aContainer,
                                       nsIContent *aChild,
                                       PRInt32 aIndexInContainer)
{
  if (IsParentOfMenuBar(aContainer)) {
    SetXULMenuBarHidden(mXULMenuHidden);
    return;
  }

  if (aContainer != mContent) {
    return;
  }

  uGlobalMenuObject *newItem =
    NewGlobalMenuItem(static_cast<uGlobalMenuObject *>(this),
                      mListener, aChild, this);
  if (newItem) {
    InsertMenuObjectAt(newItem, aIndexInContainer);
  }
}
