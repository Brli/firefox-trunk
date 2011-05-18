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
#include <nsILoadGroup.h>
#include <nsServiceManagerUtils.h>
#include <imgIContainer.h>
#include <nsNetError.h>
#include <nsNetUtil.h>
#include <nsIImageToPixbuf.h>
#include <nsIDOMNSElement.h>
#include <nsIDOMDOMTokenList.h>
#include <nsILookAndFeel.h>
#if MOZILLA_BRANCH_MAJOR_VERSION >= 6
# include <nsIDOMDocument.h>
# include <nsIDOMWindow.h>
#else
# include <nsIDOMDocumentView.h>
# include <nsIDOMAbstractView.h>
# include <nsIDOMViewCSS.h>
#endif
#include <nsIDOMElement.h>
#include <nsIDOMCSSStyleDeclaration.h>
#include <nsIDOMCSSValue.h>
#include <nsIDOMCSSPrimitiveValue.h>
#include <nsIDOMRect.h>
#include <nsICaseConversion.h>
#include <nsUnicharUtilCIID.h>

#include <libdbusmenu-gtk/menuitem.h>

#include "uGlobalMenuObject.h"
#include "uGlobalMenuBar.h"
#include "uWidgetAtoms.h"

#define MAX_LABEL_NCHARS 40

typedef nsresult (nsIDOMRect::*GetRectSideMethod)(nsIDOMCSSPrimitiveValue**);

NS_IMPL_ISUPPORTS3(uGlobalMenuIconLoader, imgIDecoderObserver, imgIContainerObserver, nsIRunnable)

PRInt32 uGlobalMenuIconLoader::sImagesInMenus = -1;
nsCOMPtr<imgILoader> uGlobalMenuIconLoader::sLoader = 0;

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
    nsCOMPtr<nsIDOMCSSStyleDeclaration> cssStyleDecl;
#if MOZILLA_BRANCH_MAJOR_VERSION >= 6
    nsCOMPtr<nsIDOMDocument> domDoc =
      do_QueryInterface(mContent->GetDocument());
    if (domDoc) {
      nsCOMPtr<nsIDOMWindow> domWin;
      domDoc->GetDefaultView(getter_AddRefs(domWin));
      if (domWin) {
        nsCOMPtr<nsIDOMElement> domElement = do_QueryInterface(mContent);
        if (domElement) {
          domWin->GetComputedStyle(domElement, EmptyString(),
                                   getter_AddRefs(cssStyleDecl));
        }
      }
    }
#else
    nsCOMPtr<nsIDOMDocumentView> domDocView =
      do_QueryInterface(mContent->GetDocument());
    if (domDocView) {
      nsCOMPtr<nsIDOMAbstractView> domWin;
      domDocView->GetDefaultView(getter_AddRefs(domWin));
      if (domWin) {
        nsCOMPtr<nsIDOMViewCSS> domViewCSS;
        domViewCSS = do_QueryInterface(domWin);
        if (domViewCSS) {
          nsCOMPtr<nsIDOMElement> domElement = do_QueryInterface(mContent);
          if (domElement) {
            domViewCSS->GetComputedStyle(domElement, EmptyString(),
                                         getter_AddRefs(cssStyleDecl));
          }
        }
      }
    }
#endif

    if (!cssStyleDecl) {
      return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsIDOMCSSValue> cssValue;
    nsCOMPtr<nsIDOMCSSPrimitiveValue> primitiveValue;
    PRUint16 primitiveType;
    cssStyleDecl->GetPropertyCSSValue(NS_LITERAL_STRING("list-style-image"),
                                      getter_AddRefs(cssValue));
    if (cssValue) {
      primitiveValue = do_QueryInterface(cssValue);
      if (primitiveValue) {
        primitiveValue->GetPrimitiveType(&primitiveType);
        if (primitiveType == nsIDOMCSSPrimitiveValue::CSS_URI) {
          rv = primitiveValue->GetStringValue(uriString);
          if (NS_SUCCEEDED(rv)) {
            hasImage = PR_TRUE;
          }
        } else {
          NS_WARNING("list-style-image has wrong primitive type");
        }
      }
    }

    if (!hasImage) {
      return NS_OK;
    }

    cssStyleDecl->GetPropertyCSSValue(NS_LITERAL_STRING("-moz-image-region"),
                                      getter_AddRefs(cssValue));
    if (cssValue) {
      primitiveValue = do_QueryInterface(cssValue);
      if (primitiveValue) {
        primitiveValue->GetPrimitiveType(&primitiveType);
        if (primitiveType == nsIDOMCSSPrimitiveValue::CSS_RECT) {
          primitiveValue->GetRectValue(getter_AddRefs(domRect));
        } else {
          NS_WARNING("-moz-image-region has wrong primitive type");
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

  if (!sLoader) {
    sLoader = do_GetService("@mozilla.org/image/loader;1");
  }

  NS_ASSERTION(sLoader, "No icon loader");
  if (!sLoader) {
    return NS_OK;
  }

  nsIDocument *doc = mContent->GetDocument();
  nsCOMPtr<nsILoadGroup> loadGroup = doc->GetDocumentLoadGroup();

#if MOZILLA_BRANCH_MAJOR_VERSION < 2
  sLoader->LoadImage(uri, nsnull, nsnull, loadGroup, this,
                    nsnull, nsIRequest::LOAD_NORMAL, nsnull,
                    nsnull, getter_AddRefs(mIconRequest));
#else
  sLoader->LoadImage(uri, nsnull, nsnull, loadGroup, this,
                    nsnull, nsIRequest::LOAD_NORMAL, nsnull,
                    nsnull, nsnull, getter_AddRefs(mIconRequest));
#endif

#if MOZILLA_BRANCH_MAJOR_VERSION >= 6
  mImageRect.SetEmpty();
#else
  mImageRect.Empty();
#endif

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
#if MOZILLA_BRANCH_MAJOR_VERSION >= 2
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

  if ((!needsClip && (origWidth > 100 || origHeight > 100)) ||
     (needsClip && (mImageRect.width > 100 || mImageRect.height > 100))) {
    /* The icon data needs to go across DBus. Make sure the icon
     * data isn't too large, else our connection gets terminated and
     * GDbus helpfully aborts the application. Thank you :)
     */
    NS_WARNING("Icon data too large");
    ClearIcon();
    return NS_OK;
  }

  nsCOMPtr<imgIContainer> clippedImg;
  if (needsClip) {
#if MOZILLA_BRANCH_MAJOR_VERSION < 2
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

#if MOZILLA_BRANCH_MAJOR_VERSION >= 2
NS_IMETHODIMP
uGlobalMenuIconLoader::OnDiscard(imgIRequest *aRequest)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
#endif

NS_IMETHODIMP
uGlobalMenuIconLoader::FrameChanged(imgIContainer *aContainer,
#if MOZILLA_BRANCH_MAJOR_VERSION < 2
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

  const PRUnichar *tmp = accesskey.BeginReading();
  PRUnichar keyUpper;
  PRUnichar keyLower;
  // XXX: I think we need to link against libxul.so to get ToLowerCase
  //      and ToUpperCase from nsUnicharUtils.h
  nsCOMPtr<nsICaseConversion> converter =
    do_GetService(NS_UNICHARUTIL_CONTRACTID);
  if (converter) {
    converter->ToUpper(*tmp, &keyUpper);
    converter->ToLower(*tmp, &keyLower);
  } else {
    if (*tmp < 256) {
      keyUpper = toupper(char(*tmp));
      keyLower = tolower(char(*tmp));
    } else {
      NS_WARNING("accesskey matching is case-sensitive when it shouldn't be");
      keyUpper = *tmp;
      keyLower = *tmp;
    }
  }

  PRUnichar *cur = label.BeginWriting();
  PRUnichar *end = label.EndWriting();
  int length = label.Length();
  int pos = 0;
  PRBool foundAccessKey = PR_FALSE;

  while (cur < end) {
    if (*cur != PRUnichar('_')) {
      if ((*cur != keyLower && *cur != keyUpper) || foundAccessKey) {
        cur++;
        pos++;
        continue;
      }
      foundAccessKey = PR_TRUE;
    }

    length += 1;
    label.SetLength(length);
    int newLength = label.Length();
    if (length != newLength)
      break; 
     
    cur = label.BeginWriting() + pos;
    end = label.EndWriting();
    memmove(cur + 1, cur, (length - 1 - pos) * sizeof(PRUnichar));
//                   \^/
    *cur = PRUnichar('_'); // Yeah!
//                    v

    cur += 2;
    pos += 2;
  }

  // Ellipsize long labels. I've picked an arbitrary length here
  if (length > MAX_LABEL_NCHARS) {
    cur = label.BeginWriting();
    for (PRUint32 i = 1; i < 4; i++) {
      *(cur + (MAX_LABEL_NCHARS - i)) = PRUnichar('.');
    }
    *(cur + MAX_LABEL_NCHARS) = nsnull;
    label.SetLength(MAX_LABEL_NCHARS);
  }

  nsCAutoString clabel;
  CopyUTF16toUTF8(label, clabel);
  dbusmenu_menuitem_property_set(mDbusMenuItem,
                                 DBUSMENU_MENUITEM_PROP_LABEL,
                                 clabel.get());
}

void
uGlobalMenuObject::SyncLabelFromContent(nsIContent *aCommandContent)
{
  if (mLabelSyncGuard) {
    return;
  }

  mLabelSyncGuard = PR_TRUE;

  if (aCommandContent) {
    nsAutoString label;
    aCommandContent->GetAttr(kNameSpaceID_None, uWidgetAtoms::label, label);
    if (!label.IsEmpty() || aCommandContent == mLabelContent) {
      // If the command content node has a label, or we previously mirrored a
      // label from it, then mirror its label again to the menuitem content node.
      // If it doesn't have a label and never did, then we just fall back to
      // the label from the menuitem content node
      mContent->SetAttr(kNameSpaceID_None, uWidgetAtoms::label,
                        label, PR_TRUE);
      mLabelContent = aCommandContent;
    }
  }

  SyncLabelFromContent();

  mLabelSyncGuard = PR_FALSE;
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

// Synchronize the 'disabled' attribute on the specified command content
// node with the menuitem content node and the 'sensitivity' property on 
// the dbusmenu node
void
uGlobalMenuObject::SyncSensitivityFromContent(nsIContent *aCommandContent)
{
  if (mSensitivitySyncGuard) {
    return;
  }

  mSensitivitySyncGuard = PR_TRUE;

  if (aCommandContent) {
    PRBool disabled = aCommandContent->AttrValueIs(kNameSpaceID_None,
                                                   uWidgetAtoms::disabled,
                                                   uWidgetAtoms::_true,
                                                   eCaseMatters);
    if (disabled) {
      mContent->SetAttr(kNameSpaceID_None, uWidgetAtoms::disabled,
                        NS_LITERAL_STRING("true"), PR_TRUE);
    } else {
      mContent->UnsetAttr(kNameSpaceID_None, uWidgetAtoms::disabled, PR_TRUE);
    }
  }

  SyncSensitivityFromContent();

  mSensitivitySyncGuard = PR_FALSE;
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
