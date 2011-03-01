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

#include <prtypes.h>
#include <nsStringAPI.h>
#include <nsIURI.h>
#include <imgILoader.h>
#include <nsILoadGroup.h>
#include <nsServiceManagerUtils.h>
#include <imgIContainer.h>
#include <nsNetError.h>
#include <nsNetUtil.h>
#include <nsIImageToPixbuf.h>
#include <nsIDOMNSElement.h>
#include <nsIDOMDOMTokenList.h>
#include <nsILookAndFeel.h>
#include <nsIDOMDocumentView.h>
#include <nsIDOMAbstractView.h>
#include <nsIDOMViewCSS.h>
#include <nsIDOMElement.h>
#include <nsIDOMCSSStyleDeclaration.h>
#include <nsIDOMCSSValue.h>
#include <nsIDOMCSSPrimitiveValue.h>
#include <nsIDOMRect.h>

#include <libdbusmenu-gtk/menuitem.h>

#include "uGlobalMenuObject.h"
#include "uGlobalMenuBar.h"
#include "uWidgetAtoms.h"

typedef nsresult (nsIDOMRect::*GetRectSideMethod)(nsIDOMCSSPrimitiveValue**);

NS_IMPL_ISUPPORTS2(uGlobalMenuIconLoader, imgIDecoderObserver, imgIContainerObserver)

PRInt32 uGlobalMenuIconLoader::sImagesInMenus = -1;

PRBool
uGlobalMenuIconLoader::ShouldShowIcon()
{
  // Ideally, we want to get the visibility of the XUL image in our menu item,
  // but that is anonymous content which is only created when the frame is drawn
  // (which obviously never happens here).
  // As an alternative, we get the user setting for menus-have-icons from
  // nsILookAndFeel. If menu icons are to be hidden, we hide everything except
  // for menuitems with the menuitem-with-favicon class. This is basically
  // how the visibility gets set anyway (see chrome://toolkit/content/xul.css),
  // which should work in most cases. But, I guess a theme could override this,
  // and then we ignore the users theme settings. Oh well......

  nsresult rv;

  if (sImagesInMenus == -1) {
    nsCOMPtr<nsILookAndFeel> laf =
      do_GetService("@mozilla.org/widget/lookandfeel;1");
    if (laf) {
      rv = laf->GetMetric(nsILookAndFeel::eMetric_ImagesInMenus,
                          sImagesInMenus);
      if (NS_FAILED(rv)) {
        NS_WARNING("Failed to get ImagesInMenus metric from nsILookAndFeel");
        sImagesInMenus = 0;
      }
    } else {
      NS_WARNING("No nsILookAndFeel service?");
      sImagesInMenus = 0;
    }
  }

  if (sImagesInMenus) {
    return PR_TRUE;
  }

  return mMenuItem->WithFavicon();
}

void
uGlobalMenuIconLoader::LoadIcon()
{
  NS_DispatchToCurrentThread(this);
}

static PRInt32
GetDOMRectSide(nsIDOMRect* aRect, GetRectSideMethod aMethod)
{
  nsCOMPtr<nsIDOMCSSPrimitiveValue> dimensionValue;
  (aRect->*aMethod)(getter_AddRefs(dimensionValue));
  if (!dimensionValue)
    return -1;

  PRUint16 primitiveType;
  nsresult rv = dimensionValue->GetPrimitiveType(&primitiveType);
  if (NS_FAILED(rv) || primitiveType != nsIDOMCSSPrimitiveValue::CSS_PX)
    return -1;

  float dimension = 0;
  rv = dimensionValue->GetFloatValue(nsIDOMCSSPrimitiveValue::CSS_PX,
                                     &dimension);
  if (NS_FAILED(rv))
    return -1;

  return NSToIntRound(dimension);
}

NS_IMETHODIMP
uGlobalMenuIconLoader::Run()
{
  // Some of this is borrowed from widget/src/cocoa/nsMenuItemIconX.mm
  if (mIconRequest) {
    mIconRequest->Cancel(NS_BINDING_ABORTED);
    mIconRequest = nsnull;
  }

  if (!mMenuItem) {
    // Our menu item got destroyed already
    return NS_OK;
  }

  mMenuItem->GetContent(getter_AddRefs(mContent));

  if (!ShouldShowIcon()) {
    ClearIcon();
    return NS_OK;
  }

  mIconLoaded = PR_FALSE;

  nsAutoString uriString;
  PRBool hasImage = mContent->GetAttr(kNameSpaceID_None, uWidgetAtoms::image,
                                      uriString);

  nsresult rv;
  nsCOMPtr<nsIDOMRect> domRect;

  if (!hasImage) {
    nsCOMPtr<nsIDOMDocumentView> domDocView =
      do_QueryInterface(mContent->GetDocument());
    if (!domDocView) {
      return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsIDOMAbstractView> domAbstractView;
    domDocView->GetDefaultView(getter_AddRefs(domAbstractView));
    if (!domAbstractView) {
      return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsIDOMViewCSS> domViewCSS =
      do_QueryInterface(domAbstractView);
    if (!domViewCSS) {
      return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsIDOMElement> domElement = do_QueryInterface(mContent);
    if (!domElement) {
      return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsIDOMCSSStyleDeclaration> cssStyleDecl;
    domViewCSS->GetComputedStyle(domElement, EmptyString(),
                                 getter_AddRefs(cssStyleDecl));
    if (!cssStyleDecl) {
      return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsIDOMCSSValue> cssValue;
    cssStyleDecl->GetPropertyCSSValue(NS_LITERAL_STRING("list-style-image"),
                                      getter_AddRefs(cssValue));
    if (!cssValue) {
      return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsIDOMCSSPrimitiveValue> primitiveValue =
      do_QueryInterface(cssValue);
    if (!primitiveValue) {
      return NS_ERROR_FAILURE;
    }

    PRUint16 primitiveType;
    rv = primitiveValue->GetPrimitiveType(&primitiveType);
    if (NS_FAILED(rv)) {
      return rv;
    }

    if (primitiveType != nsIDOMCSSPrimitiveValue::CSS_URI) {
      return NS_ERROR_FAILURE;
    }

    rv = primitiveValue->GetStringValue(uriString);
    if (NS_FAILED(rv)) {
      return rv;
    }

    cssStyleDecl->GetPropertyCSSValue(NS_LITERAL_STRING("-moz-image-region"),
                                      getter_AddRefs(cssValue));
    if (cssValue) {
      primitiveValue = do_QueryInterface(cssValue);
      if (primitiveValue) {
        rv = primitiveValue->GetPrimitiveType(&primitiveType);
        if (NS_SUCCEEDED(rv) && primitiveType ==
            nsIDOMCSSPrimitiveValue::CSS_RECT) {
          primitiveValue->GetRectValue(getter_AddRefs(domRect));
        }
      }
    }
  }

  nsCOMPtr<nsIURI> uri;
  rv = NS_NewURI(getter_AddRefs(uri), uriString);
  if (NS_FAILED(rv)) {
    ClearIcon();
    return NS_OK;
  }

  nsCOMPtr<imgILoader> loader = do_GetService("@mozilla.org/image/loader;1");
  if (!loader) {
    return NS_OK;
  }

  nsIDocument *doc = mContent->GetDocument();
  nsCOMPtr<nsILoadGroup> loadGroup = doc->GetDocumentLoadGroup();

#ifdef MOZILLA_1_9_2_BRANCH
  loader->LoadImage(uri, nsnull, nsnull, loadGroup, this,
                    nsnull, nsIRequest::LOAD_NORMAL, nsnull,
                    nsnull, getter_AddRefs(mIconRequest));
#else
  loader->LoadImage(uri, nsnull, nsnull, loadGroup, this,
                    nsnull, nsIRequest::LOAD_NORMAL, nsnull,
                    nsnull, nsnull, getter_AddRefs(mIconRequest));
#endif

  mImageRect.Empty();

  if (domRect) {
    PRInt32 bottom = GetDOMRectSide(domRect, &nsIDOMRect::GetBottom);
    PRInt32 right = GetDOMRectSide(domRect, &nsIDOMRect::GetRight);
    PRInt32 top = GetDOMRectSide(domRect, &nsIDOMRect::GetTop);
    PRInt32 left = GetDOMRectSide(domRect, &nsIDOMRect::GetLeft);

    if (top < 0 || left < 0 || bottom <= top || right <= left) {
      return NS_ERROR_FAILURE;
    }

    mImageRect.SetRect(left, top, right - left, bottom - top);
  }

  return NS_OK;
}

void
uGlobalMenuIconLoader::ClearIcon()
{
  dbusmenu_menuitem_property_remove(mMenuItem->GetDbusMenuItem(),
                                    DBUSMENU_MENUITEM_PROP_ICON_DATA);
}

NS_IMETHODIMP
uGlobalMenuIconLoader::OnStartRequest(imgIRequest *aRequest)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
uGlobalMenuIconLoader::OnStartDecode(imgIRequest *aRequest)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
uGlobalMenuIconLoader::OnStartContainer(imgIRequest *aRequest, imgIContainer *aContainer)
{
#ifndef MOZILLA_1_9_2_BRANCH
  aContainer->RequestDecode();
#endif
  return NS_OK;
}

NS_IMETHODIMP
uGlobalMenuIconLoader::OnStartFrame(imgIRequest *aRequest, PRUint32 aFrame)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
uGlobalMenuIconLoader::OnDataAvailable(imgIRequest *aRequest,
                                       PRBool aCurrentFrame,
                                       const nsIntRect *aRect)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
uGlobalMenuIconLoader::OnStopFrame(imgIRequest *aRequest, PRUint32 aFrame)
{
  if (aRequest != mIconRequest) {
    return NS_ERROR_FAILURE;
  }

  if (mIconLoaded) {
    return NS_OK;
  }

  mIconLoaded = PR_TRUE;

  nsCOMPtr<imgIContainer> img;
  aRequest->GetImage(getter_AddRefs(img));
  if (!img) {
    return NS_ERROR_FAILURE;
  }

  PRInt32 origWidth;
  PRInt32 origHeight;
  img->GetWidth(&origWidth);
  img->GetHeight(&origHeight);

  PRBool needsClip = PR_FALSE;

  if (!mImageRect.IsEmpty()) {
    if (mImageRect.XMost() > origWidth || mImageRect.YMost() > origHeight) {
      return NS_ERROR_FAILURE;
    }

    if (!(mImageRect.x == 0 && mImageRect.y == 0 &&
         mImageRect.width == origWidth && mImageRect.height == origHeight)) {
      needsClip = PR_TRUE;
    }
  }

  nsCOMPtr<imgIContainer> clippedImg;
  if (needsClip) {
#ifdef MOZILLA_1_9_2_BRANCH
    nsresult rv = img->ExtractCurrentFrame(mImageRect,
                                           getter_AddRefs(clippedImg));
#else
    nsresult rv = img->ExtractFrame(0, mImageRect, 0,
                                    getter_AddRefs(clippedImg));
#endif
    if (NS_FAILED(rv)) {
      return NS_ERROR_FAILURE;
    }
  }

  nsCOMPtr<nsIImageToPixbuf> converter =
    do_GetService("@mozilla.org/widget/image-to-gdk-pixbuf;1");
  if (!converter) {
    return NS_ERROR_FAILURE;
  }

  GdkPixbuf *pixbuf =
    converter->ConvertImageToPixbuf(clippedImg ? clippedImg : img);
  if (pixbuf) {
    dbusmenu_menuitem_property_set_image(mMenuItem->GetDbusMenuItem(),
                                         DBUSMENU_MENUITEM_PROP_ICON_DATA,
                                         pixbuf);
    g_object_unref(pixbuf);
  } else {
    ClearIcon();
  }

  return NS_OK;
}

NS_IMETHODIMP
uGlobalMenuIconLoader::OnStopContainer(imgIRequest *aRequest,
                                       imgIContainer *aContainer)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
uGlobalMenuIconLoader::OnStopDecode(imgIRequest *aRequest, nsresult status,
                                    const PRUnichar *statusArg)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
uGlobalMenuIconLoader::OnStopRequest(imgIRequest *aRequest, PRBool aIsLastPart)
{
  if (mIconRequest) {
    mIconRequest->Cancel(NS_BINDING_ABORTED);
    mIconRequest = nsnull;
  }

  return NS_OK;
}

#ifndef MOZILLA_1_9_2_BRANCH
NS_IMETHODIMP
uGlobalMenuIconLoader::OnDiscard(imgIRequest *aRequest)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
#endif

NS_IMETHODIMP
uGlobalMenuIconLoader::FrameChanged(imgIContainer *aContainer,
#ifdef MOZILLA_1_9_2_BRANCH
                                    nsIntRect *aDirtyRect)
#else
                                    const nsIntRect *aDirtyRect)
#endif
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

void
uGlobalMenuIconLoader::Destroy()
{
  if (mIconRequest) {
    mIconRequest->Cancel(NS_BINDING_ABORTED);
    mIconRequest = nsnull;
  }

  mMenuItem = nsnull;
}

void
uGlobalMenuObject::GetContent(nsIContent **_retval)
{
  if (!_retval) {
    return;
  }
  *_retval = mContent;
  NS_IF_ADDREF(*_retval);
}

// Synchronize the 'label' and 'accesskey' attributes on the DOM node
// with the 'label' property on the dbusmenu node
void
uGlobalMenuObject::SyncLabelFromContent()
{
  // Gecko stores the label and access key in separate attributes
  // so we need to convert label="Foo"/accesskey="F" in to
  // label="_Foo" for dbusmenu

  nsAutoString label;
  mContent->GetAttr(kNameSpaceID_None, uWidgetAtoms::label, label);

  nsAutoString accesskey;
  mContent->GetAttr(kNameSpaceID_None, uWidgetAtoms::accesskey, accesskey);

  const PRUnichar *key = accesskey.BeginReading();
  PRUnichar *cur = label.BeginWriting();
  PRUnichar *end = label.EndWriting();
  int length = label.Length();
  int pos = 0;
  PRBool foundAccessKey = false;

  while(cur < end) {
    if((*cur == *key && !foundAccessKey) || *cur == PRUnichar('_')) {
      
      length += 1;
      label.SetLength(length);
      int newLength = label.Length();
      if(length != newLength)
        break; 
     
      cur = label.BeginWriting() + pos;
      end = label.EndWriting();
      memmove(cur + 1, cur, (length - 1 - pos) * sizeof(PRUnichar));
//                     \^/
      *cur = PRUnichar('_'); // Yeah!
//                      v

      if(*cur == *key)
        foundAccessKey = true;

      cur += 2;
      pos += 2; 

    } else {
      cur++;
      pos++;
    } 
  }

  // Ellipsize long labels. I've picked an arbitrary length here
  if(length > 36) {
    cur = label.BeginWriting();
    *(cur + 33) = PRUnichar('.');
    *(cur + 34) = PRUnichar('.');
    *(cur + 35) = PRUnichar('.');
    *(cur + 36) = nsnull;
    label.SetLength(36);
  }

  nsCAutoString clabel;
  CopyUTF16toUTF8(label, clabel);
  dbusmenu_menuitem_property_set(mDbusMenuItem,
                                 DBUSMENU_MENUITEM_PROP_LABEL,
                                 clabel.get());
}

// Synchronize the 'hidden' attribute on the DOM node with the
// 'visible' property on the dbusmenu node
void
uGlobalMenuObject::SyncVisibilityFromContent()
{
  mContentVisible = !mContent->AttrValueIs(kNameSpaceID_None,
                                           uWidgetAtoms::hidden,
                                           uWidgetAtoms::_true,
                                           eCaseMatters);
  dbusmenu_menuitem_property_set_bool(mDbusMenuItem,
                                      DBUSMENU_MENUITEM_PROP_VISIBLE,
                                      mContentVisible);
}

// Synchronize the 'disabled' attribute on the DOM node with the
// 'sensitivity' property on the dbusmenu node
void
uGlobalMenuObject::SyncSensitivityFromContent()
{
  dbusmenu_menuitem_property_set_bool(mDbusMenuItem,
                                      DBUSMENU_MENUITEM_PROP_ENABLED,
                                      !mContent->AttrValueIs(kNameSpaceID_None,
                                                             uWidgetAtoms::disabled,
                                                             uWidgetAtoms::_true,
                                                             eCaseMatters));
}

// Synchronize the 'disabled' attribute on the specified DOM node with the
// 'sensitivity' property on the dbusmenu node
void
uGlobalMenuObject::SyncSensitivityFromContent(nsIContent *aContent)
{
  dbusmenu_menuitem_property_set_bool(mDbusMenuItem,
                                      DBUSMENU_MENUITEM_PROP_ENABLED,
                                      !aContent->AttrValueIs(kNameSpaceID_None,
                                                             uWidgetAtoms::disabled,
                                                             uWidgetAtoms::_true,
                                                             eCaseMatters));
}

void
uGlobalMenuObject::UpdateInfoFromContentClass()
{
  nsCOMPtr<nsIDOMNSElement> element(do_QueryInterface(mContent));
  if (!element) {
    return;
  }

  nsCOMPtr<nsIDOMDOMTokenList> classes;
  element->GetClassList(getter_AddRefs(classes));
  if (!classes) {
    return;
  }

  classes->Contains(NS_LITERAL_STRING("show-only-for-keyboard"),
                    &mShowOnlyForKb);
  classes->Contains(NS_LITERAL_STRING("menuitem-with-favicon"),
                    &mWithFavicon);
}

void
uGlobalMenuObject::UpdateVisibility()
{
  if (!mMenuBar) {
    return;
  }

  PRBool newVis = (mShowOnlyForKb == PR_FALSE || mMenuBar->OpenedByKeyboard()) ?
                   mContentVisible : PR_FALSE;

  dbusmenu_menuitem_property_set_bool(mDbusMenuItem,
                                      DBUSMENU_MENUITEM_PROP_VISIBLE,
                                      newVis);
}

void
uGlobalMenuObject::SyncIconFromContent()
{
  if (!mIconLoader) {
    mIconLoader = new uGlobalMenuIconLoader(this);
  }

  mIconLoader->LoadIcon();
}

void
uGlobalMenuObject::DestroyIconLoader()
{
  if (mIconLoader) {
    mIconLoader->Destroy();
  }
}
