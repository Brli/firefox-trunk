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

#include <nsCOMPtr.h>
#include <nsIContent.h>
#include <nsStringAPI.h>
#include <nsDebug.h>
#include <nsComponentManagerUtils.h>
#include <nsIInterfaceRequestorUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsIWidget.h>
#include <nsIBaseWindow.h>
#include <nsIXULWindow.h>
#include <nsICaseConversion.h>
#include <nsUnicharUtilCIID.h>
#include <imgILoader.h>
#include <nsIImageToPixbuf.h>
#include <nsIPrefService.h>
#include <nsIPrefBranch2.h>
#if MOZILLA_BRANCH_MAJOR_VERSION < 15
# include <nsIXBLService.h>
#endif
#include <nsIXPConnect.h>
#include <prenv.h>

#include <glib-object.h>

#include "uGlobalMenuService.h"
#include "uWidgetAtoms.h"

#include "uDebug.h"

class RegisterWindowCbData
{
public:
  RegisterWindowCbData(uGlobalMenuBar *aMenu,
                       uGlobalMenuRequestAutoCanceller *aCanceller):
                       mMenu(aMenu),
                       mCanceller(aCanceller)
  {
    mCancellable = mCanceller->GetCancellable();
    if (mCancellable) {
      g_object_ref(mCancellable);
      mID = g_cancellable_connect(mCancellable, G_CALLBACK(Cancelled), this, nsnull);
    }
  }

  static void Cancelled(GCancellable *aCancellable,
                        gpointer userdata)
  {
    RegisterWindowCbData *cbdata =
      static_cast<RegisterWindowCbData *>(userdata);

    // If the request was cancelled, then invalidate pointers to objects
    // that might not exist anymore, as we don't assume that GDBus
    // cancellation is reliable (see https://launchpad.net/bugs/953562)

    cbdata->mMenu = nsnull;
    cbdata->mCanceller = nsnull;
  }

  uGlobalMenuBar* GetMenuBar() { return mMenu; }

  ~RegisterWindowCbData()
  {
    if (mCancellable) {
      g_cancellable_disconnect(mCancellable, mID);
      g_object_unref(mCancellable);
    }
  }

private:
  uGlobalMenuBar *mMenu;
  GCancellable *mCancellable;
  uGlobalMenuRequestAutoCanceller *mCanceller;
  PRUint32 mID;
};

NS_IMPL_ISUPPORTS2(uGlobalMenuService, uIGlobalMenuService, nsIWindowMediatorListener)

uGlobalMenuService* uGlobalMenuService::sService = nsnull;
#define SERVICE(Name, Interface, CID) \
Interface* uGlobalMenuService::s##Name = nsnull;
#include "uGlobalMenuServiceList.h"
#undef SERVICE
bool uGlobalMenuService::sShutdown = false;

/*static*/ uGlobalMenuService*
uGlobalMenuService::GetInstanceForService()
{
  if (sService) {
    NS_ADDREF(sService);
    return sService;
  }

  if (sShutdown) {
    return nsnull;
  }

  sService = new uGlobalMenuService();
  NS_ADDREF(sService);

  if (NS_FAILED(sService->Init())) {
    NS_RELEASE(sService);
    sService = nsnull;
    return nsnull;
  }

  NS_ADDREF(sService);
  return sService;
}

#define SERVICE(Name, Interface, CID) \
Interface* \
uGlobalMenuService::Get##Name() \
{ \
  if (s##Name) { \
    return s##Name; \
  } \
  if (sShutdown) { \
    return nsnull; \
  } \
  nsCOMPtr<Interface> tmp = do_GetService(CID); \
  if (!tmp) { \
    return nsnull; \
  } \
  s##Name = tmp; \
  NS_ADDREF(s##Name); \
  return s##Name; \
}
#include "uGlobalMenuServiceList.h"
#undef SERVICE

/*static*/ void
uGlobalMenuService::Shutdown()
{
  if (!sShutdown) {
    sShutdown = true;

    if (sService) {
      if (sService->mCancellable) {
        g_cancellable_cancel(sService->mCancellable);
      }
      NS_RELEASE(sService);
      sService = nsnull;
    }

#define SERVICE(Name, Interface, CID) \
    if (s##Name) { \
      NS_RELEASE(s##Name); \
      s##Name = nsnull; \
    }
#include "uGlobalMenuServiceList.h"
#undef SERVICE
  }
}

/*static*/ bool
uGlobalMenuService::InitService()
{
  if (sShutdown) {
    return false;
  }

  if (!sService) {
    nsCOMPtr<uIGlobalMenuService> service =
      do_GetService(U_GLOBALMENUSERVICE_CONTRACTID);
  }

  return sService != nsnull;
}

/*static*/ void
uGlobalMenuService::ProxyCreatedCallback(GObject *object,
                                         GAsyncResult *res,
                                         gpointer userdata)
{
  GError *error = NULL;
  GDBusProxy *proxy = g_dbus_proxy_new_for_bus_finish(res, &error);
  if (error && g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
    // If the request was cancelled, then the service is definitely not
    // around already
    g_error_free(error);
    return;
  }

  if (error) {
    g_error_free(error);
  }

  if (!sService) {
    // We don't assume that GDbus cancellation is reliable
    // see https://launchpad.net/bugs/953562
    return;
  }

  g_object_unref(sService->mCancellable);
  sService->mCancellable = nsnull;

  sService->mDbusProxy = proxy;

  if (!proxy) {
    NS_WARNING("Failed to create proxy for AppMenu registrar");
    sService->SetOnline(false);
    return;
  }

  g_signal_connect(sService->mDbusProxy, "notify::g-name-owner",
                   G_CALLBACK(NameOwnerChangedCallback), NULL);

  char *owner = g_dbus_proxy_get_name_owner(sService->mDbusProxy);
  sService->SetOnline(owner ? true : false);
  g_free(owner);
}

/*static*/ void
uGlobalMenuService::NameOwnerChangedCallback(GObject *object,
                                             GParamSpec *pspec,
                                             gpointer userdata)
{
  if (!sService) {
    return;
  }

  char *owner = g_dbus_proxy_get_name_owner(sService->mDbusProxy);
  sService->SetOnline(owner ? true : false);
  g_free(owner);
}

/*static*/ void
uGlobalMenuService::RegisterWindowCallback(GObject *object,
                                           GAsyncResult *res,
                                           gpointer userdata)
{
  GError *error = NULL;
  GVariant *result = g_dbus_proxy_call_finish(G_DBUS_PROXY(object), res, &error);
  if (result) {
    g_variant_unref(result);
  }

  RegisterWindowCbData *data = static_cast<RegisterWindowCbData *>(userdata);

  if (error && g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
    // If the request was cancelled, then the menubar has definitely been
    // deleted already
    g_error_free(error);
    delete data;
    return;
  }

  uGlobalMenuBar *menu = data->GetMenuBar();

  // We don't assume that GDbus cancellation is reliable
  // see https://launchpad.net/bugs/953562
  if (menu && !error) {
    menu->NotifyMenuBarRegistered();
  } else if (menu && sService) {
    sService->mMenus.RemoveElement(menu);
  }

  if (error) {
    g_error_free(error);
  }

  delete data;
}

void
uGlobalMenuService::DestroyMenus()
{
  PRUint32 count = mMenus.Length();
  for (PRUint32 j = 0; j < count; j++) {
    mMenus.RemoveElementAt(0);
  }
}

void
uGlobalMenuService::DestroyMenuForWidget(nsIWidget *aWidget)
{
  for (PRUint32 i = 0; i < mMenus.Length(); i++) {
    if (mMenus[i]->WidgetHasSameToplevelWindow(aWidget)) {
      mMenus.RemoveElementAt(i);
      return;
    }
  }
}

void
uGlobalMenuService::SetOnline(bool aOnline)
{
  if (mOnline != !!aOnline) {
    mOnline = !!aOnline;

    for (PRUint32 i = mListeners.Length(); i > 0; --i) {
      mListeners[i - 1]->Observe(nsnull,
                                 mOnline ? "native-menu-service:online" :
                                           "native-menu-service:offline",
                                 nsnull);
    }


    if (!mOnline) {
      DestroyMenus();
    }
  }
}

bool
uGlobalMenuService::WidgetHasGlobalMenu(nsIWidget *aWidget)
{
  for (PRUint32 i = 0; i < mMenus.Length(); i++) {
    if (mMenus[i]->WidgetHasSameToplevelWindow(aWidget))
      return true;
  }
  return false;
}

nsresult
uGlobalMenuService::Init()
{
  nsresult rv;
  rv = uWidgetAtoms::RegisterAtoms();
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to register atoms");
    return rv;
  }

  mCancellable = g_cancellable_new();

  g_dbus_proxy_new_for_bus(G_BUS_TYPE_SESSION,
                           static_cast<GDBusProxyFlags>(
                           G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES |
                           G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS | 
                           G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START),
                           NULL,
                           "com.canonical.AppMenu.Registrar",
                           "/com/canonical/AppMenu/Registrar",
                           "com.canonical.AppMenu.Registrar",
                           mCancellable,
                           ProxyCreatedCallback,
                           NULL);

  mWindowMediator = do_GetService("@mozilla.org/appshell/window-mediator;1");
  if (!mWindowMediator) {
    NS_WARNING("No window mediator, which we need for close events");
    return NS_ERROR_FAILURE;
  }

  mWindowMediator->AddListener(this);
  return rv;
}

uGlobalMenuService::~uGlobalMenuService()
{
  if (mWindowMediator) {
    mWindowMediator->RemoveListener(this);
  }

  if (mCancellable) {
    g_object_unref(mCancellable);
  }

  if (mDbusProxy) {
    g_signal_handlers_disconnect_by_func(mDbusProxy,
                                         reinterpret_cast<gpointer>(NameOwnerChangedCallback),
                                         NULL);
    g_object_unref(mDbusProxy);
  }
}

NS_IMETHODIMP
uGlobalMenuService::CreateGlobalMenuBar(nsIWidget  *aParent, 
                                        nsIContent *aMenuBarNode)
{
  NS_ENSURE_ARG(aParent);
  NS_ENSURE_ARG(aMenuBarNode);

  if (!mOnline) {
    NS_WARNING("Can't create a menubar when the service is not online");
    return NS_ERROR_FAILURE;
  }

  // Sanity check to make sure we don't register more than one menu
  // for each top-level window
  if (WidgetHasGlobalMenu(aParent))
    return NS_ERROR_FAILURE;

  uGlobalMenuBar *menu = uGlobalMenuBar::Create(aParent, aMenuBarNode);
  if (!menu) {
    NS_WARNING("Failed to create menubar");
    return NS_ERROR_FAILURE;
  }

  mMenus.AppendElement(menu);

  return NS_OK;
}

/*static*/ bool
uGlobalMenuService::RegisterGlobalMenuBar(uGlobalMenuBar *aMenuBar,
                                          uGlobalMenuRequestAutoCanceller *aCanceller,
                                          PRUint32 aXID, nsACString& aPath)
{
  if (!InitService()) {
    NS_ERROR("Failed to register menubar - service not initialized");
    return false;
  }

  NS_ASSERTION(sService->mOnline, "Trying to register menubar when service is offline");
  if (sService->mOnline != true) {
    return false;
  }

  if (aXID == 0 || aPath.IsEmpty()) {
    return false;
  }

  RegisterWindowCbData *data = new RegisterWindowCbData(aMenuBar, aCanceller);

  g_dbus_proxy_call(sService->mDbusProxy,
                    "RegisterWindow",
                    g_variant_new("(uo)", aXID, PromiseFlatCString(aPath).get()),
                    G_DBUS_CALL_FLAGS_NONE,
                    -1,
                    aCanceller->GetCancellable(),
                    RegisterWindowCallback,
                    data);

  return true;
}

/* void registerNotification (in nsIObserver observer); */
NS_IMETHODIMP
uGlobalMenuService::RegisterNotification(nsIObserver *aObserver)
{
  NS_ENSURE_ARG(aObserver);

  return mListeners.AppendElement(aObserver) ? NS_OK : NS_ERROR_FAILURE;
}

/* void unregisterNotification (in nsIObserver observer); */
NS_IMETHODIMP
uGlobalMenuService::UnregisterNotification(nsIObserver *aObserver)
{
  NS_ENSURE_ARG(aObserver);

  return mListeners.RemoveElement(aObserver) ? NS_OK : NS_ERROR_FAILURE;
}

/* readonly attribute boolean online; */
NS_IMETHODIMP
uGlobalMenuService::GetOnline(bool *online)
{
  NS_ENSURE_ARG_POINTER(online);
  *online = mOnline;
  return NS_OK;
}

NS_IMETHODIMP
uGlobalMenuService::OnWindowTitleChange(nsIXULWindow *window,
                                        const PRUnichar *newTitle)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
uGlobalMenuService::OnOpenWindow(nsIXULWindow *window)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
uGlobalMenuService::OnCloseWindow(nsIXULWindow *window)
{
  nsCOMPtr<nsIBaseWindow> baseWindow = do_QueryInterface(window);
  NS_ASSERTION(baseWindow, "nsIXULWindow passed to OnCloseWindow is not a nsIBaseWindow");
  if (!baseWindow) {
    return NS_ERROR_INVALID_ARG;
  }

  nsCOMPtr<nsIWidget> widget;
  baseWindow->GetMainWidget(getter_AddRefs(widget));
  if (!widget) {
    return NS_ERROR_FAILURE;
  }

  DestroyMenuForWidget(widget);

  return NS_OK;
}
