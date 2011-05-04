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
#include <nsIDOMDocumentEvent.h>
#include <nsIDOMEvent.h>
#include <nsIDOMMouseEvent.h>
#include <nsIDOMAbstractView.h>
#include <nsStringAPI.h>
#include <nsIDOMEventTarget.h>
#include <nsIPrivateDOMEvent.h>
#include <nsPIDOMWindow.h>
#include <nsServiceManagerUtils.h>
#include <nsIObserverService.h>
#include <nsIDOMDocumentView.h>
#include <nsIDOMXULCommandEvent.h>
#include <nsIXPConnect.h>
#include <nsIScriptGlobalObject.h>
#include <nsIScriptContext.h>
#include <jsapi.h>
#if MOZILLA_BRANCH_MAJOR_VERSION >= 2
#include <mozilla/dom/Element.h>
#endif

#include <glib-object.h>

#include "uGlobalMenu.h"
#include "uGlobalMenuBar.h"
#include "uGlobalMenuFactory.h"
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
    nsCOMPtr<nsIDOMDocumentEvent> docEvent = do_QueryInterface(doc);
    if (docEvent) {
      nsCOMPtr<nsIDOMEvent> event;
      docEvent->CreateEvent(NS_LITERAL_STRING("Events"),
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
    nsCOMPtr<nsIDOMDocumentEvent> docEvent = do_QueryInterface(doc);
    if (docEvent) {
      nsCOMPtr<nsIDOMEvent> event;
      docEvent->CreateEvent(NS_LITERAL_STRING("Events"),
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
    PRBool isHidden = mContent->AttrValueIs(kNameSpaceID_None,
                                           uWidgetAtoms::hidden,
                                           uWidgetAtoms::_true,
                                           eCaseMatters);
    PRBool isDisabled = mContent->AttrValueIs(kNameSpaceID_None,
                                             uWidgetAtoms::disabled,
                                             uWidgetAtoms::_true,
                                             eCaseMatters);

    return (!isHidden && !isDisabled);
}

void
uGlobalMenu::AboutToOpen()
{
  PRUint32 count = mMenuObjects.Length();
  for (PRUint32 i = 0; i < count; i++) {
    mMenuObjects[i]->UpdateVisibility();
  }

  // XXX: This should happen when the pointer hovers over the menu entry,
  //      but we don't have that information right now. We synthesize it for
  //      menus, but this doesn't work for menuitems at all
  Activate();

  nsIDocument *doc = mPopupContent->GetOwnerDoc();
  if (doc) {
     nsCOMPtr<nsIDOMDocumentEvent> docEvent = do_QueryInterface(doc);
    if (docEvent) {
      nsCOMPtr<nsIDOMEvent> event;
      docEvent->CreateEvent(NS_LITERAL_STRING("mouseevent"),
                            getter_AddRefs(event));
      if (event) {
        nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(event);
        if (mouseEvent) {
          nsCOMPtr<nsIDOMDocumentView> domDocView = do_QueryInterface(doc);
          nsCOMPtr<nsIDOMAbstractView> window;
          domDocView->GetDefaultView(getter_AddRefs(window));
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
    os->NotifyObservers(nsnull, "menuservice-popup-open", popupID.get());
  }
}

void
uGlobalMenu::OnOpen()
{
  mContent->SetAttr(kNameSpaceID_None, uWidgetAtoms::open, NS_LITERAL_STRING("true"), PR_TRUE);

  nsIDocument *doc = mPopupContent->GetOwnerDoc();
  if (doc) {
     nsCOMPtr<nsIDOMDocumentEvent> docEvent = do_QueryInterface(doc);
    if (docEvent) {
      nsCOMPtr<nsIDOMEvent> event;
      docEvent->CreateEvent(NS_LITERAL_STRING("mouseevent"),
                            getter_AddRefs(event));
      if (event) {
        nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(event);
        if (mouseEvent) {
          nsCOMPtr<nsIDOMDocumentView> domDocView = do_QueryInterface(doc);
          nsCOMPtr<nsIDOMAbstractView> window;
          domDocView->GetDefaultView(getter_AddRefs(window));
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
}

void
uGlobalMenu::OnClose()
{
  mContent->UnsetAttr(kNameSpaceID_None, uWidgetAtoms::open, PR_TRUE);

  nsIDocument *doc = mPopupContent->GetOwnerDoc();
  if (doc) {
     nsCOMPtr<nsIDOMDocumentEvent> docEvent = do_QueryInterface(doc);
    if (docEvent) {
      nsCOMPtr<nsIDOMEvent> event;
      docEvent->CreateEvent(NS_LITERAL_STRING("mouseevent"),
                            getter_AddRefs(event));
      if (event) {
        nsCOMPtr<nsIDOMMouseEvent> mouseEvent = do_QueryInterface(event);
        if (mouseEvent) {
          nsCOMPtr<nsIDOMDocumentView> domDocView = do_QueryInterface(doc);
          nsCOMPtr<nsIDOMAbstractView> window;
          domDocView->GetDefaultView(getter_AddRefs(window));
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

  SyncLabelFromContent(mCommandContent);
  SyncSensitivityFromContent(mCommandContent);
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

void
uGlobalMenu::InsertMenuObjectAt(uGlobalMenuObject *menuObj,
                                PRUint32 index)
{
  PRBool res = dbusmenu_menuitem_child_add_position(mDbusMenuItem,
                                                    menuObj->GetDbusMenuItem(),
                                                    index);
  NS_ASSERTION(res, "Failed to add child to menu. Everything will go horribly wrong now");

  mMenuObjects.InsertElementAt(index, menuObj);
}

void
uGlobalMenu::AppendMenuObject(uGlobalMenuObject *menuObj)
{
  PRBool res = dbusmenu_menuitem_child_append(mDbusMenuItem,
                                              menuObj->GetDbusMenuItem());
  NS_ASSERTION(res, "Failed to append child to menu. Everything will go horribly wrong now");

  mMenuObjects.AppendElement(menuObj);
}

void
uGlobalMenu::RemoveMenuObjectAt(PRUint32 index)
{
  PRBool res = dbusmenu_menuitem_child_delete(mDbusMenuItem,
                                       mMenuObjects[index]->GetDbusMenuItem());
  NS_ASSERTION(res, "Failed to delete child from menu. Everything will go horribly wrong now");

  mMenuObjects.RemoveElementAt(index);
}

nsresult
uGlobalMenu::Build()
{
  GetMenuPopupFromMenu(getter_AddRefs(mPopupContent));

  if (!mPopupContent) {
    // The menu has no popup, so there are no menuitems here
    return NS_OK;
  }

  // Manually wrap the menupopup node to make sure it's bounded
  // Borrowed from widget/src/cocoa/nsMenuX.mm, we need this to make
  // some menus in Thunderbird work
  if (!mPopupBound) {
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
            mPopupBound = PR_TRUE;
          }
        } 
      }
    }
  }

  if (mContent != mPopupContent) {
    mListener->RegisterForContentChanges(mPopupContent, this);
  }

  PRUint32 count = mPopupContent->GetChildCount();

  for (PRUint32 i = 0; i < count; i++) {
    nsIContent *child = mPopupContent->GetChildAt(i);
    uGlobalMenuObject *menuObject =
      NewGlobalMenuItem(static_cast<uGlobalMenuObject *>(this),
                        mListener, child, mMenuBar);
    if (!menuObject) {
      return NS_ERROR_FAILURE;
    }

    AppendMenuObject(menuObject);
  }

  return NS_OK;
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

  nsIDocument *doc = mContent->GetCurrentDoc();
  if (doc) {
    nsAutoString attr;
    mContent->GetAttr(kNameSpaceID_None, uWidgetAtoms::command, attr);
#if MOZILLA_BRANCH_MAJOR_VERSION < 2
    nsCOMPtr<nsIDOMDocument> domDoc(do_QueryInterface(doc));
    nsCOMPtr<nsIDOMElement> domElmt;
#endif
    if (!attr.IsEmpty()) {
#if MOZILLA_BRANCH_MAJOR_VERSION < 2
      if (domDoc) {
        domDoc->GetElementById(attr, getter_AddRefs(domElmt));
      }

      mCommandContent = do_QueryInterface(domElmt);
#else
      mCommandContent = doc->GetElementById(attr);
#endif
    }
  }

  nsresult rv = ConstructDbusMenuItem();
  NS_ENSURE_SUCCESS(rv, rv);

  mListener->RegisterForContentChanges(mContent, this);
  if (mCommandContent) {
    mListener->RegisterForContentChanges(mCommandContent, this);
  }

  return Build();
}

void
uGlobalMenu::Rebuild()
{
  PRUint32 count = mMenuObjects.Length();
  for (PRUint32 i = 0; i < count; i++) {
    RemoveMenuObjectAt(0);
  }

  if (mContent != mPopupContent) {
    mListener->UnregisterForContentChanges(mPopupContent);
  }

  mPopupBound = PR_FALSE;

  Build();
}

uGlobalMenu::uGlobalMenu():
  uGlobalMenuObject(Menu), mPopupBound(PR_FALSE)
{
  MOZ_COUNT_CTOR(uGlobalMenu);
}

uGlobalMenu::~uGlobalMenu()
{
  mListener->UnregisterForContentChanges(mContent);
  if (mContent != mPopupContent) {
    mListener->UnregisterForContentChanges(mPopupContent);
  }
  if (mCommandContent) {
    mListener->UnregisterForContentChanges(mCommandContent);
  }

  DestroyIconLoader();

  if (mDbusMenuItem) {
    g_signal_handler_disconnect(mDbusMenuItem, mOpenHandlerID);
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

  if (aAttribute == uWidgetAtoms::open)
    return;

  if (aAttribute == uWidgetAtoms::disabled) {
    SyncSensitivityFromContent(mCommandContent);
  } else if (aAttribute == uWidgetAtoms::hidden) {
    SyncVisibilityFromContent();
  } else if (aAttribute == uWidgetAtoms::label || 
             aAttribute == uWidgetAtoms::accesskey) {
    SyncLabelFromContent(mCommandContent);
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

  if (aContainer == mContent) {
    // Should only get this when a menupopup is removed, in
    // which case, we need to rebuild the whole menu
    Rebuild();
  } else if (aContainer == mPopupContent) {
    RemoveMenuObjectAt(aIndexInContainer);
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

  if (aContainer == mContent) {
    // Should only get this when a menupopup is inserted, in
    // which case, we need to rebuild the whole menu
    Rebuild();
  } else if (aContainer == mPopupContent) {
    uGlobalMenuObject *newItem =
      NewGlobalMenuItem(static_cast<uGlobalMenuObject *>(this),
                        mListener, aChild, mMenuBar);
    if (newItem) {
      InsertMenuObjectAt(newItem, aIndexInContainer);
    }
  }
}
