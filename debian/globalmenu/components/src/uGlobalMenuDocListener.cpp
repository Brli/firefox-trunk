/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Josh Aas <josh@mozilla.com>
 *   Thomas K. Dyas <tom.dyas@gmail.com>
 *   Chris Coulson <chris.coulson@canonical.com>
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
#include <nsIDocument.h>
#include <nsIAtom.h>
#include <nsINode.h>
#if MOZILLA_BRANCH_MAJOR_VERSION >= 2
#include <mozilla/dom/Element.h>
#endif
#include <nsIContent.h>
#include <nsIDocument.h>

#include "uGlobalMenuDocListener.h"

NS_IMPL_ISUPPORTS1(uGlobalMenuDocListener, nsIMutationObserver)

nsresult
uGlobalMenuDocListener::Init(nsIContent *rootNode)
{
  NS_ENSURE_ARG(rootNode);

  mDocument = rootNode->GetOwnerDoc();
  if (!mDocument)
    return NS_ERROR_FAILURE;
  mDocument->AddMutationObserver(this);

  return NS_OK;
}

void
uGlobalMenuDocListener::Destroy()
{
  if (mDocument) {
    mDocument->RemoveMutationObserver(this);
  }
}

uGlobalMenuDocListener::uGlobalMenuDocListener() :
  mDocument(nsnull)
{
  mContentToObserverTable.Init();
}

void
uGlobalMenuDocListener::CharacterDataWillChange(nsIDocument *aDocument,
                                                nsIContent *aContent,
              	                                CharacterDataChangeInfo *aInfo)
{

}

void
uGlobalMenuDocListener::CharacterDataChanged(nsIDocument *aDocument,
                                             nsIContent *aContent,
                                             CharacterDataChangeInfo *aInfo)
{

}

void
uGlobalMenuDocListener::AttributeWillChange(nsIDocument *aDocument,
#if MOZILLA_BRANCH_MAJOR_VERSION < 2
                                            nsIContent *aContent,
#else
                                            mozilla::dom::Element *aElement,
#endif
                                            PRInt32 aNameSpaceID,
                                            nsIAtom *aAttribute,
                                            PRInt32 aModType)
{

}

void
uGlobalMenuDocListener::AttributeChanged(nsIDocument *aDocument,
#if MOZILLA_BRANCH_MAJOR_VERSION < 2
                                         nsIContent *aElement,
#else
                                         mozilla::dom::Element *aElement,
#endif
                                         PRInt32 aNameSpaceID,
                                         nsIAtom *aAttribute,
#if MOZILLA_BRANCH_MAJOR_VERSION < 2
                                         PRInt32 aModType,
                                         PRUint32 aStateMask)
#else
                                         PRInt32 aModType)
#endif
{
  if (!aElement)
    return;

  uMenuChangeObserver *obs = LookupContentChangeObserver(aElement);
  if (obs)
    obs->ObserveAttributeChanged(aDocument, aElement, aAttribute);
}

void
uGlobalMenuDocListener::ContentAppended(nsIDocument *aDocument,
                                        nsIContent *aContainer,
#if MOZILLA_BRANCH_MAJOR_VERSION >= 2
                                        nsIContent *aFirstNewContent,
#endif
                                        PRInt32 aNewIndexInContainer)
{
#if MOZILLA_BRANCH_MAJOR_VERSION < 2
  PRUint32 count = aContainer->GetChildCount();
  while ((PRUint32)aNewIndexInContainer < count) {
    nsIContent *cur = aContainer->GetChildAt(aNewIndexInContainer);
#else
  for (nsIContent* cur = aFirstNewContent; cur; cur = cur->GetNextSibling()) {
#endif
    ContentInserted(aDocument, aContainer, cur, aNewIndexInContainer);
    aNewIndexInContainer++;
  }
}

void
uGlobalMenuDocListener::ContentInserted(nsIDocument *aDocument,
                                        nsIContent *aContainer,
                                        nsIContent *aChild,
                                        PRInt32 aIndexInContainer)
{
  if (!aContainer)
    return;

  uMenuChangeObserver *obs = LookupContentChangeObserver(aContainer);
  if (obs)
    obs->ObserveContentInserted(aDocument, aContainer, aChild,
                                aIndexInContainer);

  PRUint32 count = mGlobalObservers.Length();
  for (PRUint32 i = 0; i < count; i++) {
    if (obs && mGlobalObservers[i] != obs) {
      mGlobalObservers[i]->ObserveContentInserted(aDocument, aContainer,
                                                  aChild, aIndexInContainer);
    }
  }
}

void
uGlobalMenuDocListener::ContentRemoved(nsIDocument *aDocument,
                                       nsIContent *aContainer,
                                       nsIContent *aChild,
#if MOZILLA_BRANCH_MAJOR_VERSION < 2
                                       PRInt32 aIndexInContainer)
#else
                                       PRInt32 aIndexInContainer,
                                       nsIContent *aPreviousSibling)
#endif
{
  if (!aContainer)
    return;

  uMenuChangeObserver *obs = LookupContentChangeObserver(aContainer);
  if (obs)
    obs->ObserveContentRemoved(aDocument, aContainer, aChild,
                               aIndexInContainer);

  PRUint32 count = mGlobalObservers.Length();
  for (PRUint32 i = 0; i < count; i++) {
    if (obs && mGlobalObservers[i] != obs) {
      mGlobalObservers[i]->ObserveContentRemoved(aDocument, aContainer, aChild,
                                                 aIndexInContainer);
    }
  }
}

void
uGlobalMenuDocListener::NodeWillBeDestroyed(const nsINode *aNode)
{
  mDocument = nsnull;
}

void
uGlobalMenuDocListener::ParentChainChanged(nsIContent *aContent)
{

}

void
uGlobalMenuDocListener::RegisterForContentChanges(nsIContent *aContent,
                                                  uMenuChangeObserver *aMenuObject)
{
  mContentToObserverTable.Put(aContent, aMenuObject);
}

void
uGlobalMenuDocListener::UnregisterForContentChanges(nsIContent *aContent)
{
  mContentToObserverTable.Remove(aContent);
}

void
uGlobalMenuDocListener::RegisterForAllChanges(uMenuChangeObserver *aMenuObject)
{
  if (!aMenuObject) {
    return;
  }

  PRUint32 count = mGlobalObservers.Length();
  for (PRUint32 i = 0; i < count; i ++) {
    if (mGlobalObservers[i] == aMenuObject) {
      // Don't add more than once
      return;
    }
  }
  mGlobalObservers.AppendElement(aMenuObject);
}

void
uGlobalMenuDocListener::UnregisterForAllChanges(uMenuChangeObserver *aMenuObject)
{
  if (!aMenuObject) {
    return;
  }

  PRUint32 count = mGlobalObservers.Length();
  for (PRUint32 i = 0; i < count; i++) {
    if (mGlobalObservers[i] == aMenuObject) {
      mGlobalObservers.RemoveElementAt(i);
      return;
    }
  }
}

uMenuChangeObserver*
uGlobalMenuDocListener::LookupContentChangeObserver(nsIContent *aContent)
{
  uMenuChangeObserver *result;
  if (mContentToObserverTable.Get(aContent, &result))
    return result;
  else
    return nsnull;
}
