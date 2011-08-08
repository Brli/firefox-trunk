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
#include <nsIXBLService.h>
#include <nsIAtom.h>
#include <nsIDOMEvent.h>
#include <nsIDOMMouseEvent.h>
#include <nsIDOMWindow.h>
#include <nsIDOMDocument.h>
#include <nsStringAPI.h>
#include <nsIDOMEventTarget.h>
#include <nsIPrivateDOMEvent.h>
#include <nsPIDOMWindow.h>
#include <nsServiceManagerUtils.h>
#include <nsIObserverService.h>
#include <nsIDOMXULCommandEvent.h>
#include <nsIXPConnect.h>
#include <nsIScriptGlobalObject.h>
#include <nsIScriptContext.h>
#include <jsapi.h>
#include <mozilla/dom/Element.h>

#include <glib-object.h>

#include "uGlobalMenu.h"
#include "uGlobalMenuBar.h"
#include "uGlobalMenuUtils.h"
#include "uWidgetAtoms.h"

/*static*/ PRBool
uGlobalMenu::MenuEventCallback(DbusmenuMenuitem *menu,
                               const gchar *name,
                               GVariant *value,
                               guint timestamp,
                               void *data)
{
  uGlobalMenu *self = static_cast<uGlobalMenu *>(data);
  if (!g_strcmp0("closed", name)) {
    self->OnClose();
    return PR_TRUE;
  }

  if (!g_strcmp0("opened", name)) {
    self->OnOpen();
    return PR_TRUE;
  }

  return PR_FALSE;
}

/*static*/ PRBool
uGlobalMenu::MenuAboutToOpenCallback(DbusmenuMenuitem *menu,
                                     void *data)
{
  uGlobalMenu *self = static_cast<uGlobalMenu *>(data);
  self->AboutToOpen();

  // We return false here for "needsUpdate", as we have no way of
  // knowing in advance if the menu structure is going to be updated.
  // The menu layout will still update on the client, but we won't block
  // opening the menu until it's happened
  return PR_FALSE;
}

void
uGlobalMenu::Activate()
{
  mContent->SetAttr(kNameSpaceID_None, uWidgetAtoms::menuactive,
                    NS_LITERAL_STRING("true"), PR_TRUE);

  nsIDocument *doc = mContent->GetOwnerDoc();
  nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(mContent);
  if (doc && target) {
    nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(doc);
    if (domDoc) {
      nsCOMPtr<nsIDOMEvent> event;
      domDoc->CreateEvent(NS_LITERAL_STRING("Events"),
                          getter_AddRefs(event));
      if (event) {
        event->InitEvent(NS_LITERAL_STRING("DOMMenuItemActive"),
                         PR_TRUE, PR_TRUE);
        nsCOMPtr<nsIPrivateDOMEvent> priv = do_QueryInterface(event);
        if (priv) {
          priv->SetTrusted(PR_TRUE);
        }
        PRBool dummy;
        target->DispatchEvent(event, &dummy);
      }
    }
  }
}

void
uGlobalMenu::Deactivate()
{
  mContent->UnsetAttr(kNameSpaceID_None, uWidgetAtoms::menuactive, PR_TRUE);

  nsIDocument *doc = mContent->GetOwnerDoc();
  if (doc) {
    nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(doc);
    if (domDoc) {
      nsCOMPtr<nsIDOMEvent> event;
      domDoc->CreateEvent(NS_LITERAL_STRING("Events"),
                          getter_AddRefs(event));
      if (event) {
        event->InitEvent(NS_LITERAL_STRING("DOMMenuItemInactive"),
                         PR_TRUE, PR_TRUE);
        nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(mContent);
        if (target) {
          nsCOMPtr<nsIPrivateDOMEvent> priv = do_QueryInterface(event);
          if (priv) {
            priv->SetTrusted(PR_TRUE);
          }
          PRBool dummy;
          target->DispatchEvent(event, &dummy);
        }
      }
    }
  }
}

PRBool
uGlobalMenu::CanOpen()
{
    PRBool isHidden = IsHidden();
    PRBool isDisabled = mContent->AttrValueIs(kNameSpaceID_None,
                                             uWidgetAtoms::disabled,
                                             uWidgetAtoms::_true,
                                             eCaseMatters);

    return (!isHidden && !isDisabled);
}

void
uGlobalMenu::AboutToOpen()
{
  NS_WARN_IF_FALSE(!mHalted, "Showing a menu that should have been destroyed already");

  if (mDirty) {
    Build();
  }

  // If there is no popup content, then there is nothing to do, and it's
  // unsafe to proceed anyway
  if (!mPopupContent) {
    return;
  }

  mOpening = PR_TRUE;

  PRUint32 count = mMenuObjects.Length();
  for (PRUint32 i = 0; i < count; i++) {
    mMenuObjects[i]->AboutToShowNotify();
  }

  // XXX: This should happen when the pointer hovers over the menu entry,
  //      but we don't have that information right now. We synthesize it for
  //      menus, but this doesn't work for menuitems at all
  Activate();

  nsIDocument *doc = mPopupContent->GetOwnerDoc();
  if (doc) {
    nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(doc);
    if (domDoc) {
      nsCOMPtr<nsIDOMEvent> event;
      domDoc->CreateEvent(NS_LITERAL_STRING("mouseevent"),
                          getter_AddRefs(event));
      if (event) {
        nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(event);
        if (mouseEvent) {
          nsCOMPtr<nsIDOMWindow> window;
          domDoc->GetDefaultView(getter_AddRefs(window));
          if (window) {
            mouseEvent->InitMouseEvent(NS_LITERAL_STRING("popupshowing"),
                                       PR_TRUE, PR_TRUE, window, nsnull,
                                       0, 0, 0, 0, PR_FALSE, PR_FALSE,
                                       PR_FALSE, PR_FALSE, 0, nsnull);
            nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(mPopupContent);
            if (target) {
              nsCOMPtr<nsIPrivateDOMEvent> priv = do_QueryInterface(event);
              if (priv) {
                priv->SetTrusted(PR_TRUE);
              }
              PRBool dummy;
              // XXX: dummy == PR_FALSE means that we should prevent the
              //      the menu from opening, but there's no way to do this
              target->DispatchEvent(event, &dummy);
            }
          }
        }
      }
    }
  }

  nsCOMPtr<nsIObserverService> os =
    do_GetService("@mozilla.org/observer-service;1");
  if (os) {
    nsAutoString popupID;
    mPopupContent->GetAttr(kNameSpaceID_None, uWidgetAtoms::id, popupID);
    os->NotifyObservers(nsnull, "native-menu-service:popup-open", popupID.get());
  }
}

void
uGlobalMenu::OnOpen()
{
  // If there is no popup content, then there is nothing to do, and it's
  // unsafe to proceed anyway
  if (!mPopupContent) {
    mOpening = PR_FALSE;
    return;
  }

  mContent->SetAttr(kNameSpaceID_None, uWidgetAtoms::open, NS_LITERAL_STRING("true"), PR_TRUE);

  nsIDocument *doc = mPopupContent->GetOwnerDoc();
  if (doc) {
    nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(doc);
    if (domDoc) {
      nsCOMPtr<nsIDOMEvent> event;
      domDoc->CreateEvent(NS_LITERAL_STRING("mouseevent"),
                          getter_AddRefs(event));
      if (event) {
        nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(event);
        if (mouseEvent) {
          nsCOMPtr<nsIDOMWindow> window;
          domDoc->GetDefaultView(getter_AddRefs(window));
          if (window) {
            mouseEvent->InitMouseEvent(NS_LITERAL_STRING("popupshown"),
                                       PR_TRUE, PR_TRUE, window, nsnull,
                                       0, 0, 0, 0, PR_FALSE, PR_FALSE,
                                       PR_FALSE, PR_FALSE, 0, nsnull);
            nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(mPopupContent);
            if (target) {
              nsCOMPtr<nsIPrivateDOMEvent> priv = do_QueryInterface(event);
              if (priv) {
                priv->SetTrusted(PR_TRUE);
              }
              PRBool dummy;
              target->DispatchEvent(event, &dummy);
            }
          }
        }
      }
    }
  }

  mOpening = PR_FALSE;
}

void
uGlobalMenu::OnClose()
{
  mOpening = PR_FALSE;
  // If there is no popup content, then there is nothing to do, and it's
  // unsafe to proceed anyway
  if (!mPopupContent) {
    return;
  }

  mContent->UnsetAttr(kNameSpaceID_None, uWidgetAtoms::open, PR_TRUE);

  nsIDocument *doc = mPopupContent->GetOwnerDoc();
  if (doc) {
    nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(doc);
    if (domDoc) {
      nsCOMPtr<nsIDOMEvent> event;
      domDoc->CreateEvent(NS_LITERAL_STRING("mouseevent"),
                          getter_AddRefs(event));
      if (event) {
        nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(event);
        if (mouseEvent) {
          nsCOMPtr<nsIDOMWindow> window;
          domDoc->GetDefaultView(getter_AddRefs(window));
          if (window) {
            mouseEvent->InitMouseEvent(NS_LITERAL_STRING("popuphiding"),
                                       PR_TRUE, PR_TRUE, window, nsnull,
                                       0, 0, 0, 0, PR_FALSE, PR_FALSE,
                                       PR_FALSE, PR_FALSE, 0, nsnull);
            nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(mPopupContent);
            if (target) {
              nsCOMPtr<nsIPrivateDOMEvent> priv = do_QueryInterface(event);
              if (priv) {
                priv->SetTrusted(PR_TRUE);
              }
              PRBool dummy;
              target->DispatchEvent(event, &dummy);

              mouseEvent->InitMouseEvent(NS_LITERAL_STRING("popuphidden"),
                                         PR_TRUE, PR_TRUE, window, nsnull,
                                         0, 0, 0, 0, PR_FALSE, PR_FALSE,
                                         PR_FALSE, PR_FALSE, 0, nsnull);
              target->DispatchEvent(event, &dummy);
            }
          }
        }
      }
    }
  }

  Deactivate();
}

nsresult
uGlobalMenu::ConstructDbusMenuItem()
{
  mDbusMenuItem = dbusmenu_menuitem_new();
  if (!mDbusMenuItem)
    return NS_ERROR_OUT_OF_MEMORY;

  // This happens automatically when we add children, but we have to
  // do this manually for menus which don't initially have children,
  // so we can receive about-to-show which triggers a build of the menu
  dbusmenu_menuitem_property_set(mDbusMenuItem,
                                 DBUSMENU_MENUITEM_PROP_CHILD_DISPLAY,
                                 DBUSMENU_MENUITEM_CHILD_DISPLAY_SUBMENU);

  mOpenHandlerID = g_signal_connect(G_OBJECT(mDbusMenuItem),
                                    "about-to-show",
                                    G_CALLBACK(MenuAboutToOpenCallback),
                                    this);
  mEventHandlerID = g_signal_connect(G_OBJECT(mDbusMenuItem),
                                     "event",
                                     G_CALLBACK(MenuEventCallback),
                                     this);

  SyncLabelFromContent();
  SyncSensitivityFromContent();
  SyncVisibilityFromContent();
  SyncIconFromContent();
  UpdateInfoFromContentClass();

  return NS_OK;
}

void
uGlobalMenu::GetMenuPopupFromMenu(nsIContent **aResult)
{
  if (!aResult)
    return;

  *aResult = nsnull;

  // Taken from widget/src/cocoa/nsMenuX.mm. Not sure if we need this
  nsresult rv;
  nsCOMPtr<nsIXBLService> xblService = do_GetService("@mozilla.org/xbl;1", &rv);
  if (!xblService)
    return;

  PRInt32 dummy;
  nsCOMPtr<nsIAtom> tag;
  xblService->ResolveTag(mContent, &dummy, getter_AddRefs(tag));
  if (tag == uWidgetAtoms::menupopup) {
    *aResult = mContent;
    NS_ADDREF(*aResult);
    return;
  }

  PRUint32 count = mContent->GetChildCount();

  for (PRUint32 i = 0; i < count; i++) {
    PRInt32 dummy;
    nsIContent *child = mContent->GetChildAt(i);
    nsCOMPtr<nsIAtom> tag;
    xblService->ResolveTag(child, &dummy, getter_AddRefs(tag));
    if (tag == uWidgetAtoms::menupopup) {
      *aResult = child;
      NS_ADDREF(*aResult);
      return;
    }
  }
}

PRBool
uGlobalMenu::InsertMenuObjectAt(uGlobalMenuObject *menuObj,
                                PRUint32 index)
{
  gboolean res = dbusmenu_menuitem_child_add_position(mDbusMenuItem,
                                                    menuObj->GetDbusMenuItem(),
                                                    index);
  return res && mMenuObjects.InsertElementAt(index, menuObj);
}

PRBool
uGlobalMenu::AppendMenuObject(uGlobalMenuObject *menuObj)
{
  gboolean res = dbusmenu_menuitem_child_append(mDbusMenuItem,
                                                menuObj->GetDbusMenuItem());
  return res && mMenuObjects.AppendElement(menuObj);
}

PRBool
uGlobalMenu::RemoveMenuObjectAt(PRUint32 index)
{
  gboolean res = dbusmenu_menuitem_child_delete(mDbusMenuItem,
                                       mMenuObjects[index]->GetDbusMenuItem());
  mMenuObjects.RemoveElementAt(index);

  return !!res;
}

nsresult
uGlobalMenu::Build()
{
  PRUint32 count = mMenuObjects.Length();
  for (PRUint32 i = 0; i < count; i++) {
    RemoveMenuObjectAt(0);
  }

  GetMenuPopupFromMenu(getter_AddRefs(mPopupContent));

  if (!mPopupContent) {
    // The menu has no popup, so there are no menuitems here
    return NS_OK;
  }

  // Manually wrap the menupopup node to make sure it's bounded
  // Borrowed from widget/src/cocoa/nsMenuX.mm, we need this to make
  // some menus in Thunderbird work
  nsIDocument *doc = mPopupContent->GetCurrentDoc();
  if (doc) {
    nsresult rv;
    nsCOMPtr<nsIXPConnect> xpconnect =
      do_GetService(nsIXPConnect::GetCID(), &rv);
    if (NS_SUCCEEDED(rv)) {
      nsIScriptGlobalObject *sgo = doc->GetScriptGlobalObject();
      nsCOMPtr<nsIScriptContext> scriptContext = sgo->GetContext();
      JSObject *global = sgo->GetGlobalJSObject();
      if (scriptContext && global) {
        JSContext *cx = (JSContext *)scriptContext->GetNativeContext();
        if (cx) {
          nsCOMPtr<nsIXPConnectJSObjectHolder> wrapper;
          xpconnect->WrapNative(cx, global,
                                mPopupContent, NS_GET_IID(nsISupports),
                                getter_AddRefs(wrapper));
        }
      }
    }
  }

  if (mContent != mPopupContent) {
    nsresult rv;
    rv = mListener->RegisterForContentChanges(mPopupContent, this);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mDirty = PR_FALSE;

  count = mPopupContent->GetChildCount();

  for (PRUint32 i = 0; i < count; i++) {
    nsIContent *child = mPopupContent->GetChildAt(i);
    uGlobalMenuObject *menuObject =
      NewGlobalMenuItem(static_cast<uGlobalMenuObject *>(this),
                        mListener, child, mMenuBar);
    PRBool res = PR_FALSE;
    if (menuObject) {
      res = AppendMenuObject(menuObject);
    }
    NS_WARN_IF_FALSE(res, "Failed to append menuitem. Marking menu invalid");
    if (!res) {
      Invalidate();
      return NS_ERROR_FAILURE;
    }
  }

  return NS_OK;
}

void
uGlobalMenu::Invalidate()
{
  if (!mDirty) {
    mDirty = PR_TRUE;

    if (mListener && mContent != mPopupContent) {
      mListener->UnregisterForContentChanges(mPopupContent, this);
    }

    PRUint32 count = mMenuObjects.Length();
    for (PRUint32 i = 0; i < count; i++) {
      mMenuObjects[i]->Halt();
    }
  }
}

nsresult
uGlobalMenu::Init(uGlobalMenuObject *aParent,
                  uGlobalMenuDocListener *aListener,
                  nsIContent *aContent,
                  uGlobalMenuBar *aMenuBar)
{
  NS_ENSURE_ARG(aParent);
  NS_ENSURE_ARG(aListener);
  NS_ENSURE_ARG(aContent);
  NS_ENSURE_ARG(aMenuBar);

  mParent = aParent;
  mListener = aListener;
  mContent = aContent;
  mMenuBar = aMenuBar;

  nsresult rv = mListener->RegisterForContentChanges(mContent, this);
  NS_ENSURE_SUCCESS(rv, rv);

  return ConstructDbusMenuItem();
}

uGlobalMenu::uGlobalMenu():
  uGlobalMenuObject(Menu), mOpening(PR_FALSE), mDirty(PR_TRUE)
{
  MOZ_COUNT_CTOR(uGlobalMenu);
}

uGlobalMenu::~uGlobalMenu()
{
  if (mListener) {
    if (!mHalted) {
      mListener->UnregisterForContentChanges(mContent, this);
    }
    if (mContent != mPopupContent && !mDirty) {
      mListener->UnregisterForContentChanges(mPopupContent, this);
    }
  }

  DestroyIconLoader();

  if (mDbusMenuItem) {
    g_signal_handler_disconnect(mDbusMenuItem, mOpenHandlerID);
    g_signal_handler_disconnect(mDbusMenuItem, mEventHandlerID);
    g_object_unref(mDbusMenuItem);
  }

  MOZ_COUNT_DTOR(uGlobalMenu);
}

/*static*/ uGlobalMenuObject*
uGlobalMenu::Create(uGlobalMenuObject *aParent,
                    uGlobalMenuDocListener *aListener,
                    nsIContent *aContent,
                    uGlobalMenuBar *aMenuBar)
{
  uGlobalMenu *menu = new uGlobalMenu();
  if (!menu) {
    return nsnull;
  }

  if (NS_FAILED(menu->Init(aParent, aListener, aContent, aMenuBar))) {
    delete menu;
    return nsnull;
  }

  return static_cast<uGlobalMenuObject *>(menu);
}

void
uGlobalMenu::Halt()
{
  if (!mHalted) {
    mHalted = PR_TRUE;

    if (mListener) {
      mListener->UnregisterForContentChanges(mContent, this);
    }

    Invalidate();
  }
}

void
uGlobalMenu::OpenMenu()
{
  if (!CanOpen()) {
    return;
  }

  dbusmenu_menuitem_show_to_user(mDbusMenuItem, 0);
}

void
uGlobalMenu::ObserveAttributeChanged(nsIDocument *aDocument,
                                     nsIContent *aContent,
                                     nsIAtom *aAttribute)
{
  NS_ASSERTION(aContent == mContent || aContent == mPopupContent,
               "Received an event that wasn't meant for us!");
  NS_WARN_IF_FALSE(mHalted, "Received an event after we disconnected");

  if (aAttribute == uWidgetAtoms::open) {
    return;
  }

  if (aAttribute == uWidgetAtoms::disabled) {
    SyncSensitivityFromContent();
  } else if (aAttribute == uWidgetAtoms::hidden ||
             aAttribute == uWidgetAtoms::collapsed) {
    SyncVisibilityFromContent();
  } else if (aAttribute == uWidgetAtoms::label || 
             aAttribute == uWidgetAtoms::accesskey) {
    SyncLabelFromContent();
  } else if (aAttribute == uWidgetAtoms::image) {
    SyncIconFromContent();
  } else if (aAttribute == uWidgetAtoms::_class) {
    UpdateInfoFromContentClass();
  }
}

void
uGlobalMenu::ObserveContentRemoved(nsIDocument *aDocument,
                                   nsIContent *aContainer,
                                   nsIContent *aChild,
                                   PRInt32 aIndexInContainer)
{
  NS_ASSERTION(aContainer == mContent || aContainer == mPopupContent,
               "Received an event that wasn't meant for us!");
  NS_WARN_IF_FALSE(mHalted, "Received an event after we disconnected");

  if (mDirty) {
    return;
  }

  if (mOpening && aContainer == mPopupContent) {
    PRBool res = RemoveMenuObjectAt(aIndexInContainer);
    NS_WARN_IF_FALSE(res, "Failed to remove menuitem. Marking menu invalid");
    if (!res) {
      Invalidate();
    }
  } else {
    Invalidate();
  }
}

void
uGlobalMenu::ObserveContentInserted(nsIDocument *aDocument,
                                    nsIContent *aContainer,
                                    nsIContent *aChild,
                                    PRInt32 aIndexInContainer)
{
  NS_ASSERTION(aContainer == mContent || aContainer == mPopupContent,
               "Received an event that wasn't meant for us!");
  NS_WARN_IF_FALSE(mHalted, "Received an event after we disconnected");

  if (mDirty) {
    return;
  }

  if (mOpening && aContainer == mPopupContent) {
    uGlobalMenuObject *newItem =
      NewGlobalMenuItem(static_cast<uGlobalMenuObject *>(this),
                        mListener, aChild, mMenuBar);
    PRBool res = PR_FALSE;
    if (newItem) {
      res = InsertMenuObjectAt(newItem, aIndexInContainer);
    }
    NS_WARN_IF_FALSE(res, "Failed to insert menuitem. Marking menu invalid");
    if (!res) {
      Invalidate();
    }    
  } else {
    Invalidate();
  }
}
