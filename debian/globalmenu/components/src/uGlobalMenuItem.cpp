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
#include <nsIDocument.h>
#include <nsIAtom.h>
#include <nsIPrefService.h>
#include <nsIPrefBranch2.h>
#include <nsIDOMKeyEvent.h>
#include <nsCOMPtr.h>
#include <nsServiceManagerUtils.h>
#include <nsIDOMDocumentEvent.h>
#include <nsIDOMXULCommandEvent.h>
#include <nsPIDOMWindow.h>
#include <nsIDOMAbstractView.h>
#include <nsIPrivateDOMEvent.h>
#include <nsIDOMEventTarget.h>
#ifndef MOZILLA_1_9_2_BRANCH
# include <mozilla/dom/Element.h>
#endif
#include <nsIContent.h>
#include <nsIDOMDocumentView.h>
#ifdef MOZILLA_1_9_2_BRANCH
# include <nsIDOMDocument.h>
# include <nsIDOMElement.h>
#endif

#include <glib-object.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <libdbusmenu-gtk/menuitem.h>

#include "uGlobalMenuItem.h"
#include "uGlobalMenuBar.h"
#include "uGlobalMenu.h"
#include "uWidgetAtoms.h"

// XXX: Borrowed from content/xbl/src/nsXBLPrototypeHandler.cpp. This doesn't
// seem to be publicly available, and we need a way to map key names
// to key codes (so we can then map them to GDK key codes)

struct keyCodeData {
  const char* str;
  size_t strlength;
  PRUint32 keycode;
};

// All of these must be uppercase, since the function below does
// case-insensitive comparison by converting to uppercase.
// XXX: be sure to check this periodically for new symbol additions!
static struct keyCodeData gKeyCodes[] = {

#define KEYCODE_ENTRY(str) {#str, sizeof(#str) - 1, nsIDOMKeyEvent::DOM_##str}

  KEYCODE_ENTRY(VK_CANCEL),
  KEYCODE_ENTRY(VK_BACK_SPACE),
  KEYCODE_ENTRY(VK_TAB),
  KEYCODE_ENTRY(VK_CLEAR),
  KEYCODE_ENTRY(VK_RETURN),
  KEYCODE_ENTRY(VK_ENTER),
  KEYCODE_ENTRY(VK_SHIFT),
  KEYCODE_ENTRY(VK_CONTROL),
  KEYCODE_ENTRY(VK_ALT),
  KEYCODE_ENTRY(VK_PAUSE),
  KEYCODE_ENTRY(VK_CAPS_LOCK),
  KEYCODE_ENTRY(VK_ESCAPE),
  KEYCODE_ENTRY(VK_SPACE),
  KEYCODE_ENTRY(VK_PAGE_UP),
  KEYCODE_ENTRY(VK_PAGE_DOWN),
  KEYCODE_ENTRY(VK_END),
  KEYCODE_ENTRY(VK_HOME),
  KEYCODE_ENTRY(VK_LEFT),
  KEYCODE_ENTRY(VK_UP),
  KEYCODE_ENTRY(VK_RIGHT),
  KEYCODE_ENTRY(VK_DOWN),
  KEYCODE_ENTRY(VK_PRINTSCREEN),
  KEYCODE_ENTRY(VK_INSERT),
  KEYCODE_ENTRY(VK_HELP),
  KEYCODE_ENTRY(VK_DELETE),
  KEYCODE_ENTRY(VK_0),
  KEYCODE_ENTRY(VK_1),
  KEYCODE_ENTRY(VK_2),
  KEYCODE_ENTRY(VK_3),
  KEYCODE_ENTRY(VK_4),
  KEYCODE_ENTRY(VK_5),
  KEYCODE_ENTRY(VK_6),
  KEYCODE_ENTRY(VK_7),
  KEYCODE_ENTRY(VK_8),
  KEYCODE_ENTRY(VK_9),
  KEYCODE_ENTRY(VK_SEMICOLON),
  KEYCODE_ENTRY(VK_EQUALS),
  KEYCODE_ENTRY(VK_A),
  KEYCODE_ENTRY(VK_B),
  KEYCODE_ENTRY(VK_C),
  KEYCODE_ENTRY(VK_D),
  KEYCODE_ENTRY(VK_E),
  KEYCODE_ENTRY(VK_F),
  KEYCODE_ENTRY(VK_G),
  KEYCODE_ENTRY(VK_H),
  KEYCODE_ENTRY(VK_I),
  KEYCODE_ENTRY(VK_J),
  KEYCODE_ENTRY(VK_K),
  KEYCODE_ENTRY(VK_L),
  KEYCODE_ENTRY(VK_M),
  KEYCODE_ENTRY(VK_N),
  KEYCODE_ENTRY(VK_O),
  KEYCODE_ENTRY(VK_P),
  KEYCODE_ENTRY(VK_Q),
  KEYCODE_ENTRY(VK_R),
  KEYCODE_ENTRY(VK_S),
  KEYCODE_ENTRY(VK_T),
  KEYCODE_ENTRY(VK_U),
  KEYCODE_ENTRY(VK_V),
  KEYCODE_ENTRY(VK_W),
  KEYCODE_ENTRY(VK_X),
  KEYCODE_ENTRY(VK_Y),
  KEYCODE_ENTRY(VK_Z),
  KEYCODE_ENTRY(VK_NUMPAD0),
  KEYCODE_ENTRY(VK_NUMPAD1),
  KEYCODE_ENTRY(VK_NUMPAD2),
  KEYCODE_ENTRY(VK_NUMPAD3),
  KEYCODE_ENTRY(VK_NUMPAD4),
  KEYCODE_ENTRY(VK_NUMPAD5),
  KEYCODE_ENTRY(VK_NUMPAD6),
  KEYCODE_ENTRY(VK_NUMPAD7),
  KEYCODE_ENTRY(VK_NUMPAD8),
  KEYCODE_ENTRY(VK_NUMPAD9),
  KEYCODE_ENTRY(VK_MULTIPLY),
  KEYCODE_ENTRY(VK_ADD),
  KEYCODE_ENTRY(VK_SEPARATOR),
  KEYCODE_ENTRY(VK_SUBTRACT),
  KEYCODE_ENTRY(VK_DECIMAL),
  KEYCODE_ENTRY(VK_DIVIDE),
  KEYCODE_ENTRY(VK_F1),
  KEYCODE_ENTRY(VK_F2),
  KEYCODE_ENTRY(VK_F3),
  KEYCODE_ENTRY(VK_F4),
  KEYCODE_ENTRY(VK_F5),
  KEYCODE_ENTRY(VK_F6),
  KEYCODE_ENTRY(VK_F7),
  KEYCODE_ENTRY(VK_F8),
  KEYCODE_ENTRY(VK_F9),
  KEYCODE_ENTRY(VK_F10),
  KEYCODE_ENTRY(VK_F11),
  KEYCODE_ENTRY(VK_F12),
  KEYCODE_ENTRY(VK_F13),
  KEYCODE_ENTRY(VK_F14),
  KEYCODE_ENTRY(VK_F15),
  KEYCODE_ENTRY(VK_F16),
  KEYCODE_ENTRY(VK_F17),
  KEYCODE_ENTRY(VK_F18),
  KEYCODE_ENTRY(VK_F19),
  KEYCODE_ENTRY(VK_F20),
  KEYCODE_ENTRY(VK_F21),
  KEYCODE_ENTRY(VK_F22),
  KEYCODE_ENTRY(VK_F23),
  KEYCODE_ENTRY(VK_F24),
  KEYCODE_ENTRY(VK_NUM_LOCK),
  KEYCODE_ENTRY(VK_SCROLL_LOCK),
  KEYCODE_ENTRY(VK_COMMA),
  KEYCODE_ENTRY(VK_PERIOD),
  KEYCODE_ENTRY(VK_SLASH),
  KEYCODE_ENTRY(VK_BACK_QUOTE),
  KEYCODE_ENTRY(VK_OPEN_BRACKET),
  KEYCODE_ENTRY(VK_BACK_SLASH),
  KEYCODE_ENTRY(VK_CLOSE_BRACKET),
  KEYCODE_ENTRY(VK_QUOTE)

#undef KEYCODE_ENTRY
};

PRUint32
uGlobalMenuItem::GetKeyCode(nsAString &aKeyName)
{
  nsCAutoString keyName;
  CopyUTF16toUTF8(aKeyName, keyName);
  ToUpperCase(keyName); // We want case-insensitive comparison with data
                        // stored as uppercase.

  PRUint32 keyNameLength = keyName.Length();
  const char* keyNameStr = keyName.get();
  for (PRUint16 i = 0; i < (sizeof(gKeyCodes) / sizeof(gKeyCodes[0])); ++i)
    if (keyNameLength == gKeyCodes[i].strlength &&
        !strcmp(gKeyCodes[i].str, keyNameStr))
      return gKeyCodes[i].keycode;

  return 0;
}

// XXX: Borrowed from widget/src/gtk2/nsGtkKeyUtils.cpp
struct nsKeyConverter {
    int vkCode; // Platform independent key code
    int keysym; // GDK keysym key code
};

//
// Netscape keycodes are defined in widget/public/nsGUIEvent.h
// GTK keycodes are defined in <gdk/gdkkeysyms.h>
//
static struct nsKeyConverter nsKeycodes[] = {
    { nsIDOMKeyEvent::DOM_VK_CANCEL,     GDK_Cancel },
    { nsIDOMKeyEvent::DOM_VK_BACK_SPACE, GDK_BackSpace },
    { nsIDOMKeyEvent::DOM_VK_TAB,        GDK_Tab },
    { nsIDOMKeyEvent::DOM_VK_TAB,        GDK_ISO_Left_Tab },
    { nsIDOMKeyEvent::DOM_VK_CLEAR,      GDK_Clear },
    { nsIDOMKeyEvent::DOM_VK_RETURN,     GDK_Return },
    { nsIDOMKeyEvent::DOM_VK_SHIFT,      GDK_Shift_L },
    { nsIDOMKeyEvent::DOM_VK_SHIFT,      GDK_Shift_R },
    { nsIDOMKeyEvent::DOM_VK_CONTROL,    GDK_Control_L },
    { nsIDOMKeyEvent::DOM_VK_CONTROL,    GDK_Control_R },
    { nsIDOMKeyEvent::DOM_VK_ALT,        GDK_Alt_L },
    { nsIDOMKeyEvent::DOM_VK_ALT,        GDK_Alt_R },
    { nsIDOMKeyEvent::DOM_VK_META,       GDK_Meta_L },
    { nsIDOMKeyEvent::DOM_VK_META,       GDK_Meta_R },
    { nsIDOMKeyEvent::DOM_VK_PAUSE,      GDK_Pause },
    { nsIDOMKeyEvent::DOM_VK_CAPS_LOCK,  GDK_Caps_Lock },
#ifndef MOZILLA_1_9_2_BRANCH
    { nsIDOMKeyEvent::DOM_VK_KANA,       GDK_Kana_Lock },
    { nsIDOMKeyEvent::DOM_VK_KANA,       GDK_Kana_Shift },
    { nsIDOMKeyEvent::DOM_VK_HANGUL,     GDK_Hangul },
    // { nsIDOMKeyEvent::DOM_VK_JUNJA,      GDK_XXX },
    // { nsIDOMKeyEvent::DOM_VK_FINAL,      GDK_XXX },
    { nsIDOMKeyEvent::DOM_VK_HANJA,      GDK_Hangul_Hanja },
    { nsIDOMKeyEvent::DOM_VK_KANJI,      GDK_Kanji },
#endif
    { nsIDOMKeyEvent::DOM_VK_ESCAPE,     GDK_Escape },
#ifndef MOZILLA_1_9_2_BRANCH
    { nsIDOMKeyEvent::DOM_VK_CONVERT,    GDK_Henkan },
    { nsIDOMKeyEvent::DOM_VK_NONCONVERT, GDK_Muhenkan },
    // { nsIDOMKeyEvent::DOM_VK_ACCEPT,     GDK_XXX },
    { nsIDOMKeyEvent::DOM_VK_MODECHANGE, GDK_Mode_switch },
#endif
    { nsIDOMKeyEvent::DOM_VK_SPACE,      GDK_space },
    { nsIDOMKeyEvent::DOM_VK_PAGE_UP,    GDK_Page_Up },
    { nsIDOMKeyEvent::DOM_VK_PAGE_DOWN,  GDK_Page_Down },
    { nsIDOMKeyEvent::DOM_VK_END,        GDK_End },
    { nsIDOMKeyEvent::DOM_VK_HOME,       GDK_Home },
    { nsIDOMKeyEvent::DOM_VK_LEFT,       GDK_Left },
    { nsIDOMKeyEvent::DOM_VK_UP,         GDK_Up },
    { nsIDOMKeyEvent::DOM_VK_RIGHT,      GDK_Right },
    { nsIDOMKeyEvent::DOM_VK_DOWN,       GDK_Down },
#ifndef MOZILLA_1_9_2_BRANCH
    { nsIDOMKeyEvent::DOM_VK_SELECT,     GDK_Select },
    { nsIDOMKeyEvent::DOM_VK_PRINT,      GDK_Print },
    { nsIDOMKeyEvent::DOM_VK_EXECUTE,    GDK_Execute },
#endif
    { nsIDOMKeyEvent::DOM_VK_PRINTSCREEN, GDK_Print },
    { nsIDOMKeyEvent::DOM_VK_INSERT,     GDK_Insert },
    { nsIDOMKeyEvent::DOM_VK_DELETE,     GDK_Delete },
    { nsIDOMKeyEvent::DOM_VK_HELP,       GDK_Help },

    // keypad keys
    { nsIDOMKeyEvent::DOM_VK_LEFT,       GDK_KP_Left },
    { nsIDOMKeyEvent::DOM_VK_RIGHT,      GDK_KP_Right },
    { nsIDOMKeyEvent::DOM_VK_UP,         GDK_KP_Up },
    { nsIDOMKeyEvent::DOM_VK_DOWN,       GDK_KP_Down },
    { nsIDOMKeyEvent::DOM_VK_PAGE_UP,    GDK_KP_Page_Up },
    // Not sure what these are
    //{ nsIDOMKeyEvent::DOM_VK_,       GDK_KP_Prior },
    //{ nsIDOMKeyEvent::DOM_VK_,        GDK_KP_Next },
    // GDK_KP_Begin is the 5 on the non-numlock keypad
    //{ nsIDOMKeyEvent::DOM_VK_,        GDK_KP_Begin },
    { nsIDOMKeyEvent::DOM_VK_PAGE_DOWN,  GDK_KP_Page_Down },
    { nsIDOMKeyEvent::DOM_VK_HOME,       GDK_KP_Home },
    { nsIDOMKeyEvent::DOM_VK_END,        GDK_KP_End },
    { nsIDOMKeyEvent::DOM_VK_INSERT,     GDK_KP_Insert },
    { nsIDOMKeyEvent::DOM_VK_DELETE,     GDK_KP_Delete },

    { nsIDOMKeyEvent::DOM_VK_MULTIPLY,   GDK_KP_Multiply },
    { nsIDOMKeyEvent::DOM_VK_ADD,        GDK_KP_Add },
    { nsIDOMKeyEvent::DOM_VK_SEPARATOR,  GDK_KP_Separator },
    { nsIDOMKeyEvent::DOM_VK_SUBTRACT,   GDK_KP_Subtract },
    { nsIDOMKeyEvent::DOM_VK_DECIMAL,    GDK_KP_Decimal },
    { nsIDOMKeyEvent::DOM_VK_DIVIDE,     GDK_KP_Divide },
    { nsIDOMKeyEvent::DOM_VK_RETURN,     GDK_KP_Enter },
    { nsIDOMKeyEvent::DOM_VK_NUM_LOCK,   GDK_Num_Lock },
    { nsIDOMKeyEvent::DOM_VK_SCROLL_LOCK,GDK_Scroll_Lock },

    { nsIDOMKeyEvent::DOM_VK_COMMA,      GDK_comma },
    { nsIDOMKeyEvent::DOM_VK_PERIOD,     GDK_period },
    { nsIDOMKeyEvent::DOM_VK_SLASH,      GDK_slash },
    { nsIDOMKeyEvent::DOM_VK_BACK_SLASH, GDK_backslash },
    { nsIDOMKeyEvent::DOM_VK_BACK_QUOTE, GDK_grave },
    { nsIDOMKeyEvent::DOM_VK_OPEN_BRACKET, GDK_bracketleft },
    { nsIDOMKeyEvent::DOM_VK_CLOSE_BRACKET, GDK_bracketright },
    { nsIDOMKeyEvent::DOM_VK_SEMICOLON, GDK_colon },
    { nsIDOMKeyEvent::DOM_VK_QUOTE, GDK_apostrophe },

    // context menu key, keysym 0xff67, typically keycode 117 on 105-key (Microsoft) 
    // x86 keyboards, located between right 'Windows' key and right Ctrl key
    { nsIDOMKeyEvent::DOM_VK_CONTEXT_MENU, GDK_Menu },
#ifndef MOZILLA_1_9_2_BRANCH
    { nsIDOMKeyEvent::DOM_VK_SLEEP,      GDK_Sleep },
#endif

    // NS doesn't have dash or equals distinct from the numeric keypad ones,
    // so we'll use those for now.  See bug 17008:
    { nsIDOMKeyEvent::DOM_VK_SUBTRACT, GDK_minus },
    { nsIDOMKeyEvent::DOM_VK_EQUALS, GDK_equal },

    // Some shifted keys, see bug 15463 as well as 17008.
    // These should be subject to different keyboard mappings.
    { nsIDOMKeyEvent::DOM_VK_QUOTE, GDK_quotedbl },
    { nsIDOMKeyEvent::DOM_VK_OPEN_BRACKET, GDK_braceleft },
    { nsIDOMKeyEvent::DOM_VK_CLOSE_BRACKET, GDK_braceright },
    { nsIDOMKeyEvent::DOM_VK_BACK_SLASH, GDK_bar },
    { nsIDOMKeyEvent::DOM_VK_SEMICOLON, GDK_semicolon },
    { nsIDOMKeyEvent::DOM_VK_BACK_QUOTE, GDK_asciitilde },
    { nsIDOMKeyEvent::DOM_VK_COMMA, GDK_less },
    { nsIDOMKeyEvent::DOM_VK_PERIOD, GDK_greater },
    { nsIDOMKeyEvent::DOM_VK_SLASH,      GDK_question },
    { nsIDOMKeyEvent::DOM_VK_1, GDK_exclam },
    { nsIDOMKeyEvent::DOM_VK_2, GDK_at },
    { nsIDOMKeyEvent::DOM_VK_3, GDK_numbersign },
    { nsIDOMKeyEvent::DOM_VK_4, GDK_dollar },
    { nsIDOMKeyEvent::DOM_VK_5, GDK_percent },
    { nsIDOMKeyEvent::DOM_VK_6, GDK_asciicircum },
    { nsIDOMKeyEvent::DOM_VK_7, GDK_ampersand },
    { nsIDOMKeyEvent::DOM_VK_8, GDK_asterisk },
    { nsIDOMKeyEvent::DOM_VK_9, GDK_parenleft },
    { nsIDOMKeyEvent::DOM_VK_0, GDK_parenright },
    { nsIDOMKeyEvent::DOM_VK_SUBTRACT, GDK_underscore },
    { nsIDOMKeyEvent::DOM_VK_EQUALS, GDK_plus }
};

PRUint32
uGlobalMenuItem::MozKeyCodeToGdkKeyCode(PRUint32 aMozKeyCode)
{
  if (aMozKeyCode >= nsIDOMKeyEvent::DOM_VK_A &&
      aMozKeyCode <= nsIDOMKeyEvent::DOM_VK_Z)
    return aMozKeyCode;

  if (aMozKeyCode >= nsIDOMKeyEvent::DOM_VK_0 &&
      aMozKeyCode <= nsIDOMKeyEvent::DOM_VK_9)
    return aMozKeyCode;

  if (aMozKeyCode >= nsIDOMKeyEvent::DOM_VK_NUMPAD0 &&
      aMozKeyCode <= nsIDOMKeyEvent::DOM_VK_NUMPAD9)
    return aMozKeyCode - nsIDOMKeyEvent::DOM_VK_NUMPAD0 + GDK_0;

  if (aMozKeyCode >= nsIDOMKeyEvent::DOM_VK_F1 &&
      aMozKeyCode <= nsIDOMKeyEvent::DOM_VK_F24)
    return aMozKeyCode - nsIDOMKeyEvent::DOM_VK_F1 + GDK_KEY_F1;

  for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(nsKeycodes); i++) {
    if (nsKeycodes[i].vkCode == aMozKeyCode)
      return nsKeycodes[i].keysym;
  }

  return GDK_VoidSymbol;
}

void
uGlobalMenuItem::SyncAccelFromContent()
{
  nsAutoString modStr;
  mKeyContent->GetAttr(kNameSpaceID_None, uWidgetAtoms::modifiers, modStr);

  PRUint32 modifier = 0;

  if (!modStr.IsEmpty()) {
    char* str = ToNewUTF8String(modStr);
    char *token = strtok(str, ", \t");
    while(token) {
      if (strcmp(token, "shift") == 0) {
        modifier |= GDK_SHIFT_MASK;
      } else if (strcmp(token, "alt") == 0) {
        modifier |= GDK_MOD1_MASK;
      } else if (strcmp(token, "meta") == 0) {
        modifier |= GDK_META_MASK;
      } else if (strcmp(token, "control") == 0) {
        modifier |= GDK_CONTROL_MASK;
      } else if (strcmp(token, "accel") == 0) {
        nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
        if (prefs) {
          PRInt32 accel;
          prefs->GetIntPref("ui.key.accelKey", &accel);
          if (accel == nsIDOMKeyEvent::DOM_VK_META) {
            modifier |= GDK_META_MASK;
          } else if (accel == nsIDOMKeyEvent::DOM_VK_ALT) {
            modifier |= GDK_MOD1_MASK;
          } else {
            modifier |= GDK_CONTROL_MASK;
          }
        } else {
          // This is the default, see layout/xul/base/src/nsMenuFrame.cpp
          modifier |= GDK_CONTROL_MASK;
        }
      }

      token = strtok(nsnull, ", \t");
    }

    nsMemory::Free(str);
  }

  nsAutoString keyStr;
  guint key = 0;
  mKeyContent->GetAttr(kNameSpaceID_None, uWidgetAtoms::key, keyStr);

  nsCAutoString cKeyStr;
  CopyUTF16toUTF8(keyStr, cKeyStr);

  if (!cKeyStr.IsEmpty()) {
    key = gdk_keyval_from_name(cKeyStr.get());
  }

  if (key == 0 && !keyStr.IsEmpty()) {
    key = gdk_unicode_to_keyval(*keyStr.BeginReading());
  }

  if (key == 0) {
    mKeyContent->GetAttr(kNameSpaceID_None, uWidgetAtoms::keycode, keyStr);
    if (!keyStr.IsEmpty())
      key = MozKeyCodeToGdkKeyCode(GetKeyCode(keyStr));
  }

  if (key == 0) {
    key = GDK_VoidSymbol;
  }

  if (key != GDK_VoidSymbol) {
    dbusmenu_menuitem_property_set_shortcut(mDbusMenuItem, key,
                                       static_cast<GdkModifierType>(modifier));
  } else {
    dbusmenu_menuitem_property_remove(mDbusMenuItem,
                                      DBUSMENU_MENUITEM_PROP_SHORTCUT);
  }
}

void
uGlobalMenuItem::SyncTypeAndStateFromContent()
{
  static nsIContent::AttrValuesArray attrs[] =
    { &uWidgetAtoms::checkbox, &uWidgetAtoms::radio, nsnull };
  PRInt32 type = mContent->FindAttrValueIn(kNameSpaceID_None,
                                           uWidgetAtoms::type,
                                           attrs, eCaseMatters);

  if (type >= 0 && type < 2) {
    if (type == 0) {
      dbusmenu_menuitem_property_set(mDbusMenuItem,
                                     DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE,
                                     DBUSMENU_MENUITEM_TOGGLE_CHECK);
      mType = CheckBox;
    } else if (type == 1) {
      dbusmenu_menuitem_property_set(mDbusMenuItem,
                                     DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE,
                                     DBUSMENU_MENUITEM_TOGGLE_RADIO);
      mType = Radio;
    }

    nsIContent *content = mCommandContent ? mCommandContent : mContent;
    PRBool lastToggleState = mToggleState;
    mToggleState = content->AttrValueIs(kNameSpaceID_None, 
                                        uWidgetAtoms::checked,
                                        uWidgetAtoms::_true,
                                        eCaseMatters);
    dbusmenu_menuitem_property_set_int(mDbusMenuItem,
                                       DBUSMENU_MENUITEM_PROP_TOGGLE_STATE,
                                       mToggleState ?
                                       DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED : 
                                        DBUSMENU_MENUITEM_TOGGLE_STATE_UNCHECKED);

    if (mCommandContent && lastToggleState != mToggleState) {
      mContent->SetAttr(kNameSpaceID_None, uWidgetAtoms::checked,
                        mToggleState ? NS_LITERAL_STRING("true") :
                         NS_LITERAL_STRING("false"), PR_TRUE);
    }

    mIsToggle = PR_TRUE;
  } else {
    dbusmenu_menuitem_property_remove(mDbusMenuItem,
                                      DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE);
    dbusmenu_menuitem_property_remove(mDbusMenuItem,
                                      DBUSMENU_MENUITEM_PROP_TOGGLE_STATE);
    mIsToggle = PR_FALSE;
    mType = Normal;
  }
}

void
uGlobalMenuItem::SyncProperties()
{
  SyncVisibilityFromContent();
  SyncSensitivityFromContent(mCommandContent);
  SyncLabelFromContent(mCommandContent);
  SyncTypeAndStateFromContent();
  if (mKeyContent) {
    SyncAccelFromContent();
  } else {
    dbusmenu_menuitem_property_remove(mDbusMenuItem,
                                      DBUSMENU_MENUITEM_PROP_SHORTCUT);
  }
  SyncIconFromContent();
  UpdateInfoFromContentClass();
}

/*static*/ void
uGlobalMenuItem::ItemActivatedCallback(DbusmenuMenuitem *menuItem,
                                       PRUint32 timeStamp,
                                       void *data)
{
  uGlobalMenuItem *self = static_cast<uGlobalMenuItem *>(data);
  self->Activate();
}

void
uGlobalMenuItem::Activate()
{
  // This first bit seems backwards, but it's not really. If autocheck is
  // not set or autocheck==true, then the checkbox state is usually updated
  // by the input event that clicked on the menu item. In this case, we need
  // to manually update the checkbox state. If autocheck==false, then the input 
  // event doesn't toggle the checkbox state, and it is up  to the handler on
  // this node to do it instead. In this case, we leave it alone

  // XXX: it is important to update the checkbox state before dispatching
  //      the event, as the handler might check the new state
  if (!mContent->AttrValueIs(kNameSpaceID_None, uWidgetAtoms::autocheck,
                             uWidgetAtoms::_false, eCaseMatters) && 
      mType != Normal) {
    if (!mToggleState) {
      // We're currently not checked, so check now
      mContent->SetAttr(kNameSpaceID_None, uWidgetAtoms::checked,
                        NS_LITERAL_STRING("true"), PR_TRUE);
    } else if (mToggleState && mType == CheckBox) {
      // We're currently checked, so uncheck now. Don't do this for radio buttons
      mContent->UnsetAttr(kNameSpaceID_None, uWidgetAtoms::checked, PR_TRUE);
    }
  }

  nsIDocument *doc = mContent->GetOwnerDoc();
  if (doc) {
    nsCOMPtr<nsIDOMDocumentEvent> docEvent = do_QueryInterface(doc);
    if (docEvent) {
      nsCOMPtr<nsIDOMEvent> event;
      docEvent->CreateEvent(NS_LITERAL_STRING("xulcommandevent"),
                            getter_AddRefs(event));
      if (event) {
        nsCOMPtr<nsIDOMXULCommandEvent> cmdEvent = do_QueryInterface(event);
        if (cmdEvent) {
          nsCOMPtr<nsIDOMDocumentView> domDocView = do_QueryInterface(doc);
          nsCOMPtr<nsIDOMAbstractView> window;
          domDocView->GetDefaultView(getter_AddRefs(window));
          if (window) {
            cmdEvent->InitCommandEvent(NS_LITERAL_STRING("command"),
                                       PR_TRUE, PR_TRUE, window, 0,
                                       PR_FALSE, PR_FALSE, PR_FALSE,
                                       PR_FALSE, nsnull);
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
  }
}

nsresult
uGlobalMenuItem::ConstructDbusMenuItem()
{
  mDbusMenuItem = dbusmenu_menuitem_new();
  if (!mDbusMenuItem)
    return NS_ERROR_OUT_OF_MEMORY;

  mHandlerID = g_signal_connect(G_OBJECT(mDbusMenuItem),
                                DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
                                G_CALLBACK(ItemActivatedCallback),
                                this);

  SyncProperties();

  return NS_OK;
}

nsresult
uGlobalMenuItem::Init(uGlobalMenuObject *aParent,
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
#ifdef MOZILLA_1_9_2_BRANCH
    nsCOMPtr<nsIDOMDocument> domDoc(do_QueryInterface(doc));
    nsCOMPtr<nsIDOMElement> domElmt;
#endif
    if (!attr.IsEmpty()) {
#ifdef MOZILLA_1_9_2_BRANCH
      if (domDoc) {
        domDoc->GetElementById(attr, getter_AddRefs(domElmt));
      }

      mCommandContent = do_QueryInterface(domElmt);
#else
      mCommandContent = doc->GetElementById(attr);
#endif
    }

    mContent->GetAttr(kNameSpaceID_None, uWidgetAtoms::key, attr);
    if (!attr.IsEmpty()) {
#ifdef MOZILLA_1_9_2_BRANCH
      if (domDoc) {
        domDoc->GetElementById(attr, getter_AddRefs(domElmt));
      }

      mKeyContent = do_QueryInterface(domElmt);
#else
      mKeyContent = doc->GetElementById(attr);
#endif
    }
  }

  mListener->RegisterForContentChanges(mContent, this);
  if (mCommandContent) {
    mListener->RegisterForContentChanges(mCommandContent, this);
  }
  if (mKeyContent) {
    mListener->RegisterForContentChanges(mKeyContent, this);
  }

  nsresult rv = ConstructDbusMenuItem();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

void
uGlobalMenuItem::UncheckSiblings()
{
  if (mType != Radio) {
    // If we're not a radio button, we don't care
    return;
  }

  nsAutoString name;
  mContent->GetAttr(kNameSpaceID_None, uWidgetAtoms::name, name);
  if (name.IsEmpty()) {
    // If we don't have a name, then we can't find our siblings
    return;
  }

  nsIContent *parent = mContent->GetParent();
  if (!parent) {
    return;
  }

  PRUint32 count = parent->GetChildCount();
  for (PRUint32 i = 0; i < count; i++) {
    nsIContent *sibling = parent->GetChildAt(i);
    if (sibling->AttrValueIs(kNameSpaceID_None, uWidgetAtoms::name,
        name, eCaseMatters) && sibling != mContent) {
      if (sibling->AttrValueIs(kNameSpaceID_None, uWidgetAtoms::type,
          uWidgetAtoms::radio, eCaseMatters)) {
        sibling->UnsetAttr(kNameSpaceID_None, uWidgetAtoms::checked, PR_TRUE);
      }
    }
  }
}

uGlobalMenuItem::~uGlobalMenuItem()
{
  mListener->UnregisterForContentChanges(mContent);
  if (mCommandContent) {
    mListener->UnregisterForContentChanges(mCommandContent);
  }
  if (mKeyContent) {
    mListener->UnregisterForContentChanges(mKeyContent);
  }

  DestroyIconLoader();

  if (mDbusMenuItem) {
    g_signal_handler_disconnect(mDbusMenuItem, mHandlerID);
    g_object_unref(mDbusMenuItem);
  }
}

/*static*/ uGlobalMenuObject*
uGlobalMenuItem::Create(uGlobalMenuObject *aParent,
                        uGlobalMenuDocListener *aListener,
                        nsIContent *aContent,
                        uGlobalMenuBar *aMenuBar)
{
  uGlobalMenuItem *menuitem = new uGlobalMenuItem();
  if (!menuitem) {
    return nsnull;
  }

  if (NS_FAILED(menuitem->Init(aParent, aListener, aContent, aMenuBar))) {
    delete menuitem;
    return nsnull;
  }

  return static_cast<uGlobalMenuObject *>(menuitem);
}

void
uGlobalMenuItem::ObserveAttributeChanged(nsIDocument *aDocument,
                                         nsIContent *aContent,
                                         nsIAtom *aAttribute)
{
  NS_ASSERTION(aContent == mContent || aContent == mCommandContent ||
               aContent == mKeyContent,
               "Received an event that wasn't meant for us!");

  nsIDocument *doc = mContent->GetCurrentDoc();

  if (aAttribute == uWidgetAtoms::command && doc && aContent == mContent) {
    if (mCommandContent) {
      mListener->UnregisterForContentChanges(mCommandContent);
    }
    nsAutoString command;
    mContent->GetAttr(kNameSpaceID_None, uWidgetAtoms::command, command);
    if (!command.IsEmpty()) {
#ifdef MOZILLA_1_9_2_BRANCH
      nsCOMPtr<nsIDOMDocument> domDoc(do_QueryInterface(doc));
      nsCOMPtr<nsIDOMElement> domElmt;

      if (domDoc) {
        domDoc->GetElementById(command, getter_AddRefs(domElmt));
      }

      mCommandContent = do_QueryInterface(domElmt);
#else
      mCommandContent = doc->GetElementById(command);
#endif
      if (mCommandContent) {
        mListener->RegisterForContentChanges(mCommandContent, this);
      }
    } else {
      mCommandContent = nsnull;
    }
    SyncProperties();
  } else if (aAttribute == uWidgetAtoms::key && doc && aContent == mContent) {
    if (mKeyContent) {
      mListener->UnregisterForContentChanges(mKeyContent);
    }
    nsAutoString key;
    mContent->GetAttr(kNameSpaceID_None, uWidgetAtoms::key, key);
    if (!key.IsEmpty()) {
#ifdef MOZILLA_1_9_2_BRANCH
      nsCOMPtr<nsIDOMDocument> domDoc(do_QueryInterface(doc));
      nsCOMPtr<nsIDOMElement> domElmt;

      if (domDoc) {
        domDoc->GetElementById(key, getter_AddRefs(domElmt));
      }

      mKeyContent = do_QueryInterface(domElmt);
#else
      mKeyContent = doc->GetElementById(key);
#endif
      if (mKeyContent) {
        mListener->RegisterForContentChanges(mKeyContent, this);
      }
    } else {
      mKeyContent = nsnull;
    }
    SyncProperties();
  } else if (aAttribute == uWidgetAtoms::label ||
             aAttribute == uWidgetAtoms::accesskey) {
    SyncLabelFromContent(mCommandContent);
  } else if (aAttribute == uWidgetAtoms::hidden) {
    SyncVisibilityFromContent();
  } else if (aAttribute == uWidgetAtoms::disabled) {
    SyncSensitivityFromContent(mCommandContent);
  } else if (aAttribute == uWidgetAtoms::keycode ||
             aAttribute == uWidgetAtoms::key ||
             aAttribute == uWidgetAtoms::modifiers) {
    SyncAccelFromContent();
  } else if (aAttribute == uWidgetAtoms::checked) {
    SyncTypeAndStateFromContent();
    if (mContent->AttrValueIs(kNameSpaceID_None, uWidgetAtoms::checked,
        uWidgetAtoms::_true, eCaseMatters)) {
      UncheckSiblings();
    }
  } else if (aAttribute == uWidgetAtoms::type) {
    SyncTypeAndStateFromContent();
  } else if (aAttribute == uWidgetAtoms::image) {
    SyncIconFromContent();
  } else if (aAttribute == uWidgetAtoms::_class) {
    UpdateInfoFromContentClass();
  }
}

void
uGlobalMenuItem::ObserveContentRemoved(nsIDocument *aDocument,
                                       nsIContent *aContainer,
                                       nsIContent *aChild,
                                       PRInt32 aIndexInContainer)
{
  NS_ASSERTION(0, "We can't remove content from a menuitem!");
}

void
uGlobalMenuItem::ObserveContentInserted(nsIDocument *aDocument,
                                        nsIContent *aContainer,
                                        nsIContent *aChild,
                                        PRInt32 aIndexInContainer)
{
  NS_ASSERTION(0, "We can't insert content in to a menuitem!");
}
