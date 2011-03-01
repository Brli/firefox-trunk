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

#ifndef _U_GLOBALMENUBASE_H
#define _U_GLOBALMENUBASE_H

#include <nsIContent.h>
#include <nsCOMPtr.h>
#include <nsAutoPtr.h>
#include <imgIDecoderObserver.h>
#include <imgIRequest.h>
#include <imgIContainerObserver.h>
#include <nsThreadUtils.h>

#include <libdbusmenu-glib/server.h>

#include "uGlobalMenuDocListener.h"

enum uMenuObjectType {
  MenuBar,
  Menu,
  MenuItem,
  MenuSeparator,
  MenuDummy
};

class uGlobalMenuObject;
class uGlobalMenuBar;

class uGlobalMenuIconLoader: public imgIDecoderObserver,
                             public nsRunnable
{
  friend class uGlobalMenuObject;
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IMGIDECODEROBSERVER
  NS_DECL_IMGICONTAINEROBSERVER
  NS_DECL_NSIRUNNABLE

  void LoadIcon ();

  uGlobalMenuIconLoader (uGlobalMenuObject *aMenuItem):
                         mMenuItem(aMenuItem) { };
  ~uGlobalMenuIconLoader () { };

protected:
  void Destroy ();

private:
  void ClearIcon ();
  PRBool ShouldShowIcon ();

  PRBool mIconLoaded;
  uGlobalMenuObject *mMenuItem;
  nsCOMPtr<nsIContent> mContent;
  nsCOMPtr<imgIRequest> mIconRequest;
  nsIntRect mImageRect;
  static PRInt32 sImagesInMenus;
};

class uGlobalMenuObject
{
  friend class uGlobalMenuIconLoader;
public:
  uGlobalMenuObject (uMenuObjectType aType): mDbusMenuItem(nsnull),
                                             mListener(nsnull),
                                             mParent(nsnull),
                                             mType(aType) { };
  DbusmenuMenuitem* GetDbusMenuItem () { return mDbusMenuItem; }
  uGlobalMenuObject* GetParent() { return mParent; }
  uMenuObjectType GetType() { return mType; }
  void GetContent (nsIContent **_retval);
  void UpdateVisibility();
  virtual ~uGlobalMenuObject () { };

protected:
  void SyncLabelFromContent ();
  void SyncVisibilityFromContent ();
  void SyncSensitivityFromContent ();
  void SyncSensitivityFromContent (nsIContent *aContent);
  void SyncIconFromContent ();
  void UpdateInfoFromContentClass();
  void DestroyIconLoader ();
  PRBool WithFavicon() { return mWithFavicon; }

  nsCOMPtr<nsIContent> mContent;
  DbusmenuMenuitem *mDbusMenuItem;
  nsRefPtr<uGlobalMenuDocListener> mListener;
  uGlobalMenuObject *mParent;
  uMenuObjectType mType;
  PRBool mContentVisible;
  uGlobalMenuBar *mMenuBar;
  PRBool mWithFavicon;
  PRBool mShowOnlyForKb;

private:
  nsRefPtr<uGlobalMenuIconLoader> mIconLoader;
};

#endif
