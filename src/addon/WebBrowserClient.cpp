/*
 *      Copyright (C) 2015-2017 Team KODI
 *      http:/kodi.tv
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#undef Success     // Definition conflicts with cef_message_router.h
#undef RootWindow  // Definition conflicts with root_window.h

#include <kodi/ActionIDs.h>
#include <kodi/Filesystem.h>
#include <kodi/General.h>
#include <kodi/XBMC_vkeys.h>
#include <kodi/gui/dialogs/ContextMenu.h>
#include <kodi/gui/dialogs/FileBrowser.h>
#include <kodi/gui/dialogs/Keyboard.h>
#include <kodi/gui/dialogs/Select.h>
#include <kodi/gui/dialogs/YesNo.h>

#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/cef_command_line.h"
#include "include/cef_parser.h"
#include "include/wrapper/cef_helpers.h"
#include "include/base/cef_bind.h"
#include "include/views/cef_textfield.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_stream_resource_handler.h"

#include "p8-platform/util/StringUtils.h"

#include "addon.h"
#include "WebBrowserClient.h"
#include "WebBrowserManager.h"
#include "URICheckHandler.h"
#include "Utils.h"
#include "DOMVisitor.h"
#include "MessageIds.h"
#include "SystemTranslator.h"
#include "JSInterface/Handler.h"
#include "JSInterface/JSDialogHandler.h"


#define ZOOM_MULTIPLY 25.0

using namespace std;
using namespace P8PLATFORM;


enum DOMTestType {
  DOM_TEST_STRUCTURE,
  DOM_TEST_MODIFY,
};


// Custom menu command Ids.
enum client_menu_ids {
  CLIENT_ID_OPEN_SELECTED_SIDE = MENU_ID_USER_FIRST,
  CLIENT_ID_OPEN_SELECTED_SIDE_IN_NEW_TAB,
  CLIENT_ID_OPEN_KEYBOARD,
};


//#define ACTION_MOUSE_START            100
//#define ACTION_MOUSE_LEFT_CLICK       100
//#define ACTION_MOUSE_RIGHT_CLICK      101
//#define ACTION_MOUSE_MIDDLE_CLICK     102
//#define ACTION_MOUSE_DOUBLE_CLICK     103
//#define ACTION_MOUSE_WHEEL_UP         104
//#define ACTION_MOUSE_WHEEL_DOWN       105
//#define ACTION_MOUSE_DRAG             106
//#define ACTION_MOUSE_MOVE             107
//#define ACTION_MOUSE_LONG_CLICK       108
//#define ACTION_MOUSE_END              109

int GetCefStateModifiers(int actionId)
{
  int modifiers = 0;
  switch (actionId)
  {
    case ACTION_MOUSE_LEFT_CLICK:
      modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;
      fprintf(stderr, "ACTION_MOUSE_LEFT_CLICK\n");
      break;
    case ACTION_MOUSE_RIGHT_CLICK:
      modifiers |= EVENTFLAG_RIGHT_MOUSE_BUTTON;
      fprintf(stderr, "ACTION_MOUSE_RIGHT_CLICK\n");
      break;
    case ACTION_MOUSE_MIDDLE_CLICK:
      modifiers |= EVENTFLAG_MIDDLE_MOUSE_BUTTON;
      fprintf(stderr, "EVENTFLAG_MIDDLE_MOUSE_BUTTON\n");
      break;
    case ACTION_MOUSE_DOUBLE_CLICK:
//      modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;
fprintf(stderr, "ACTION_MOUSE_DOUBLE_CLICK\n");
      break;
    case ACTION_MOUSE_WHEEL_UP:
//      modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;
fprintf(stderr, "ACTION_MOUSE_WHEEL_UP\n");
      break;
    case ACTION_MOUSE_WHEEL_DOWN:
//      modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;
fprintf(stderr, "ACTION_MOUSE_WHEEL_DOWN\n");
      break;
    case ACTION_MOUSE_DRAG:
//      modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;
fprintf(stderr, "ACTION_MOUSE_DRAG\n");
      break;
    case ACTION_MOUSE_MOVE:
//      modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;
fprintf(stderr, "ACTION_MOUSE_MOVE\n");
      break;
    case ACTION_MOUSE_LONG_CLICK:
//      modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;
fprintf(stderr, "ACTION_MOUSE_LONG_CLICK\n");
      break;
    default:
      break;
  }



//  if (state & GDK_SHIFT_MASK)
////    modifiers |= EVENTFLAG_SHIFT_DOWN;
////  if (state & GDK_LOCK_MASK)
////    modifiers |= EVENTFLAG_CAPS_LOCK_ON;
////  if (state & GDK_CONTROL_MASK)
////    modifiers |= EVENTFLAG_CONTROL_DOWN;
////  if (state & GDK_MOD1_MASK)
////    modifiers |= EVENTFLAG_ALT_DOWN;
////  if (state & GDK_BUTTON1_MASK)
////    modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;
////  if (state & GDK_BUTTON2_MASK)
////    modifiers |= EVENTFLAG_MIDDLE_MOUSE_BUTTON;
////  if (state & GDK_BUTTON3_MASK)
////    modifiers |= EVENTFLAG_RIGHT_MOUSE_BUTTON;
  return modifiers;
}












class CNavigationEntryVisitor : public CefNavigationEntryVisitor
{
public:
  CNavigationEntryVisitor() { }

  virtual bool Visit(CefRefPtr<CefNavigationEntry> entry, bool current, int index, int total) override;

private:
  IMPLEMENT_REFCOUNTING(CNavigationEntryVisitor);
};

bool CNavigationEntryVisitor::Visit(CefRefPtr<CefNavigationEntry> entry, bool current, int index, int total)
{
  fprintf(stderr, "-- %s\n", __PRETTY_FUNCTION__);
  fprintf(stderr, " - current %i\n", current);
  fprintf(stderr, " - index %i\n", index);
  fprintf(stderr, " - total %i\n", total);
  if (entry && index >= 0)
  {
  fprintf(stderr, " - entry->GetURL() %s\n", entry->GetURL().ToString().c_str());
  fprintf(stderr, " - entry->GetDisplayURL() %s\n", entry->GetDisplayURL().ToString().c_str());
  fprintf(stderr, " - entry->GetTitle() %s\n", entry->GetTitle().ToString().c_str());
  }
  return true;
}






// class ClientDownloadImageCallback : public CefDownloadImageCallback
// {
//  public:
//   explicit ClientDownloadImageCallback() {}
//
//   void OnDownloadImageFinished(const CefString& image_url,
//                                int http_status_code,
//                                CefRefPtr<CefImage> image) OVERRIDE
//   {
//
//   }
//
//  private:
//   IMPLEMENT_REFCOUNTING(ClientDownloadImageCallback);
//   DISALLOW_COPY_AND_ASSIGN(ClientDownloadImageCallback);
// };

CWebBrowserClient::CWebBrowserClient(KODI_HANDLE handle, int iUniqueClientId, CWebBrowser* instance)
  : CWebControl(handle, iUniqueClientId),
    m_mainBrowserHandler(instance),
    m_renderViewReady(false),
    m_iUniqueClientId(iUniqueClientId),
    m_iMousePreviousFlags(0),
    m_iMousePreviousControl(MBT_LEFT),
    m_browserId(-1),
    m_browser(nullptr),
    m_browserCount(0),
    m_isFullScreen(false),
    m_focusedField{0},
    m_isLoading{false}
{
  m_fMouseXScaleFactor = (GetXPos() + GetWidth()) / (GetSkinXPos() + GetSkinWidth());
  m_fMouseYScaleFactor = (GetYPos() + GetHeight()) / (GetSkinYPos() + GetSkinHeight());

  // CEF related sub classes to manage web parts
  m_resourceManager = new CefResourceManager();

  // Create the browser-side router for query handling.
  m_jsDialogHandler = new CJSDialogHandler(this);
  m_renderer = new CRendererClient(this);
}

CWebBrowserClient::~CWebBrowserClient()
{
}



void CWebBrowserClient::SetBrowser(CefRefPtr<CefBrowser> browser)
{
//   fprintf(stderr, "-- %s\n", __func__);
  m_browser = browser;
}

bool CWebBrowserClient::SetInactive()
{
  m_renderViewReady = false;
  if (m_browser.get())
  {
    m_browser->GetHost()->SetFocus(false);
    return true;
  }

  return false;
}

bool CWebBrowserClient::SetActive()
{
  m_renderViewReady = true;
  if (m_browser.get())
  {
    m_browser->GetHost()->SetFocus(true);
    SetOpenedAddress(m_currentURL);
    SetOpenedTitle(m_currentTitle);
    SetIconURL(m_currentIcon);
    return true;
  }

  return false;
}

bool CWebBrowserClient::CloseComplete()
{
  if (m_browser.get())
  {
    m_browser->GetHost()->CloseBrowser(true);
    m_browser = nullptr;
    return true;
  }

  return false;
}

void CWebBrowserClient::DestroyRenderer()
{
  m_renderer = nullptr;
}

bool CWebBrowserClient::OnAction(int actionId, uint32_t buttoncode, wchar_t unicode, int &nextItem)
{
  if (!m_browser.get())
    return false;

  CefRefPtr<CefBrowserHost> host = m_browser->GetHost();

  fprintf(stderr, "----------int actionId %i, uint32_t buttoncode %X, wchar_t unicode %i, int &nextItem %i\n",
          actionId, buttoncode, unicode, nextItem);

  if (!m_focusedField.isEditable)
  {
    switch (actionId)
    {
      case ACTION_VOLUME_UP:
      case ACTION_VOLUME_DOWN:
      case ACTION_VOLAMP:
      case ACTION_MUTE:
        return false;
      case ACTION_NAV_BACK:
      case ACTION_MENU:
      case ACTION_PREVIOUS_MENU:
        return false;
      case ACTION_ZOOM_OUT:
      {
        int zoomTo = kodi::GetSettingInt("main.zoomlevel") - kodi::GetSettingInt("main.zoom_step_size");
        if (zoomTo < 30)
          break;

        LOG_MESSAGE(ADDON_LOG_DEBUG, "%s - Zoom out to %i %%", __FUNCTION__, zoomTo);
        m_browser->GetHost()->SetZoomLevel(PercentageToZoomLevel(zoomTo));
        kodi::SetSettingInt("main.zoomlevel", zoomTo);
        break;
      }
      case ACTION_ZOOM_IN:
      {
        int zoomTo = kodi::GetSettingInt("main.zoomlevel") + kodi::GetSettingInt("main.zoom_step_size");
        if (zoomTo > 330)
          break;

        LOG_MESSAGE(ADDON_LOG_DEBUG, "%s - Zoom in to %i %% - %i %i", __FUNCTION__, zoomTo, kodi::GetSettingInt("main.zoomlevel"), kodi::GetSettingInt("main.zoom_step_size"));
        m_browser->GetHost()->SetZoomLevel(PercentageToZoomLevel(zoomTo));
        kodi::SetSettingInt("main.zoomlevel", zoomTo);
        break;
      }
      default:
        break;
    };
  }

  CefKeyEvent key_event;
  key_event.modifiers = CSystemTranslator::ButtonCodeToModifier(buttoncode);
  key_event.windows_key_code = CSystemTranslator::ButtonCodeToKeyboardCode(buttoncode);
  key_event.native_key_code = 0;
  key_event.is_system_key = false;
  key_event.character = unicode;
  key_event.unmodified_character = CSystemTranslator::ButtonCodeToUnmodifiedCharacter(buttoncode);
  key_event.focus_on_editable_field = m_focusedField.isEditable;

  if (key_event.windows_key_code == VKEY_RETURN)
  {
    // We need to treat the enter key as a key press of character \r.  This
    // is apparently just how webkit handles it and what it expects.
    key_event.unmodified_character = '\r';
  }

  key_event.type = KEYEVENT_RAWKEYDOWN;
  host->SendKeyEvent(key_event);
  key_event.type = KEYEVENT_KEYUP;
  host->SendKeyEvent(key_event);
  key_event.type = KEYEVENT_CHAR;
  host->SendKeyEvent(key_event);

  return true;
}

bool CWebBrowserClient::OnMouseEvent(int id, double x, double y, double offsetX, double offsetY, int state)
{
  if (!m_browser.get())
    return true;

  static const int scrollbarPixelsPerTick = 40;

  CefRefPtr<CefBrowserHost> host = m_browser->GetHost();

  CefMouseEvent mouse_event;
  mouse_event.x = x * m_fMouseXScaleFactor;
  mouse_event.y = y * m_fMouseYScaleFactor;

  switch (id)
  {
    case ACTION_MOUSE_LEFT_CLICK:
    {
      if (m_focusedField.isEditable &&
          mouse_event.x >= m_focusedField.x &&
          mouse_event.x <= m_focusedField.x + m_focusedField.width &&
          mouse_event.y >= m_focusedField.y &&
          mouse_event.y <= m_focusedField.y + m_focusedField.height)
      {
        CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create(AddonClientMessage::FocusedSelected);
        m_browser->SendProcessMessage(PID_RENDERER, message);
      }
      else
      {
        m_focusedField = {0};

        mouse_event.modifiers = 0;
        mouse_event.modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;
        host->SendMouseClickEvent(mouse_event, MBT_LEFT, false, 1);
        host->SendMouseClickEvent(mouse_event, MBT_LEFT, true, 1);
        m_iMousePreviousFlags = mouse_event.modifiers;
        m_iMousePreviousControl = MBT_LEFT;
      }
      break;
    }
    case ACTION_MOUSE_RIGHT_CLICK:
      mouse_event.modifiers = 0;
      mouse_event.modifiers |= EVENTFLAG_RIGHT_MOUSE_BUTTON;
      host->SendMouseClickEvent(mouse_event, MBT_RIGHT, false, 1);
      host->SendMouseClickEvent(mouse_event, MBT_RIGHT, true, 1);
      m_iMousePreviousFlags = mouse_event.modifiers;
      m_iMousePreviousControl = MBT_RIGHT;
      break;
    case ACTION_MOUSE_MIDDLE_CLICK:
      mouse_event.modifiers = 0;
      mouse_event.modifiers |= EVENTFLAG_MIDDLE_MOUSE_BUTTON;
      host->SendMouseClickEvent(mouse_event, MBT_MIDDLE, false, 1);
      host->SendMouseClickEvent(mouse_event, MBT_MIDDLE, true, 1);
      m_iMousePreviousFlags = mouse_event.modifiers;
      m_iMousePreviousControl = MBT_MIDDLE;
      break;
    case ACTION_MOUSE_DOUBLE_CLICK:
      mouse_event.modifiers = m_iMousePreviousFlags;
      host->SendMouseClickEvent(mouse_event, m_iMousePreviousControl, false, 1);
      host->SendMouseClickEvent(mouse_event, m_iMousePreviousControl, true, 1);
      m_iMousePreviousControl = MBT_LEFT;
      m_iMousePreviousFlags = 0;
      break;
    case ACTION_MOUSE_WHEEL_UP:
      host->SendMouseWheelEvent(mouse_event, 0, scrollbarPixelsPerTick);
      break;
    case ACTION_MOUSE_WHEEL_DOWN:
      host->SendMouseWheelEvent(mouse_event, 0, -scrollbarPixelsPerTick);
      break;
    case ACTION_MOUSE_DRAG:

      break;
    case ACTION_MOUSE_MOVE:
    {
      bool mouse_leave = state == 3 ? true : false;
      host->SendMouseMoveEvent(mouse_event, mouse_leave);
      break;
    }
    case ACTION_MOUSE_LONG_CLICK:

      break;
    default:
      break;
  }

  return true;
}

bool CWebBrowserClient::Initialize()
{
  if (!m_browser.get())
  {
    LOG_MESSAGE(ADDON_LOG_ERROR, "%s - Called without present browser", __FUNCTION__);
    return false;
  }

  m_browser->GetHost()->SetZoomLevel(PercentageToZoomLevel(kodi::GetSettingInt("main.zoomlevel")));

  return true;
}

void CWebBrowserClient::Render()
{
  if (m_renderViewReady)
    m_renderer->Render();
}

bool CWebBrowserClient::Dirty()
{
  if (!m_renderViewReady)
    return false;

  HandleMessages();
  return m_renderer->Dirty();
}

bool CWebBrowserClient::OpenWebsite(const std::string& strURL, bool single, bool allowMenus)
{
//   fprintf(stderr, "-- %s\n", __func__);
  if (!m_browser.get())
  {
    LOG_MESSAGE(ADDON_LOG_ERROR, "%s - Called without present browser", __FUNCTION__);
    return false;
  }

  CefRefPtr<CefFrame> frame = m_browser->GetMainFrame();
  if (!frame.get())
  {
    LOG_MESSAGE(ADDON_LOG_ERROR, "%s - Called without present frame", __FUNCTION__);
    return false;
  }

  if (m_strStartupURL.empty())
    m_strStartupURL = strURL;

  m_currentIcon = "";
  Message tMsg = {TMSG_SET_ICON_URL};
  tMsg.strParam = m_currentIcon;
  SendMessage(tMsg, false);

  frame->LoadURL(strURL);

  return true;
}

void CWebBrowserClient::Reload()
{
  m_browser->Reload();
}

void CWebBrowserClient::StopLoad()
{
  m_browser->StopLoad();
}

void CWebBrowserClient::GoBack()
{
  m_browser->GoBack();
}

void CWebBrowserClient::GoForward()
{
  m_browser->GoForward();
}

void CWebBrowserClient::OpenOwnContextMenu()
{
  std::vector<std::string> entries;
  entries.push_back(kodi::GetLocalizedString(30009));
  entries.push_back(kodi::GetLocalizedString(30323));
  entries.push_back(kodi::GetLocalizedString(30032));

  int ret = kodi::gui::dialogs::ContextMenu::Show("", entries);
  if (ret >= 0)
  {
    switch (ret)
    {
      case 0:
      {
        m_mainBrowserHandler->OpenDownloadDialog();
        break;
      }
      case 1:
      {
        m_mainBrowserHandler->OpenCookieHandler();
        break;
      }
      case 2:
      {
        kodi::OpenSettings();
        break;
      }
      default:
        break;
    }
  }
}


bool CWebBrowserClient::GetHistory(std::vector<std::string>& historyWebsiteNames, bool behindCurrent)
{
  if (!m_browser)
    return false;

  bool currentFound = false;
  for (const auto& entry : m_historyWebsiteNames)
  {
    if (!behindCurrent && entry.second)
      break;
    if (entry.second)
    {
      currentFound = true;
      continue;
    }

    if (behindCurrent && !currentFound)
      continue;

    historyWebsiteNames.push_back(entry.first);
  }

  return true;
}

void CWebBrowserClient::SearchText(const std::string& text, bool forward, bool matchCase, bool findNext)
{
  if (m_browser)
  {
    if (m_currentSearchText != text)
      m_browser->GetHost()->StopFinding(true);
    m_browser->GetHost()->Find(0, text, forward, matchCase, findNext);
    m_currentSearchText = text;
  }
}

void CWebBrowserClient::StopSearch(bool clearSelection)
{
  if (m_browser)
    m_browser->GetHost()->StopFinding(clearSelection);
  m_currentSearchText.clear();
}

// -----------------------------------------------------------------------------

CefRefPtr<CefDialogHandler> CWebBrowserClient::GetDialogHandler()
{
  return m_mainBrowserHandler->GetUploadHandler();
}

CefRefPtr<CefDownloadHandler> CWebBrowserClient::GetDownloadHandler()
{
  return m_mainBrowserHandler->GetDownloadHandler();
}

CefRefPtr<CefGeolocationHandler> CWebBrowserClient::GetGeolocationHandler()
{
  return m_mainBrowserHandler->GetGeolocationPermission();
}

CefRefPtr<CefJSDialogHandler> CWebBrowserClient::GetJSDialogHandler()
{
  return m_jsDialogHandler;
}

CefRefPtr<CefRenderHandler> CWebBrowserClient::GetRenderHandler()
{
  return m_renderer;
}

/// CefClient methods
//@{
bool CWebBrowserClient::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefProcessId source_process,
                                                 CefRefPtr<CefProcessMessage> message)
{
  CEF_REQUIRE_UI_THREAD();

  if (m_messageRouter->OnProcessMessageReceived(browser, source_process, message))
    return true;

  std::string message_name = message->GetName();
  CefRefPtr<CefBrowserHost> host = browser->GetHost();
  if (message_name == RendererMessage::ExecuteJavaScriptBrowserSide)
  {
    browser->GetMainFrame()->ExecuteJavaScript(message->GetArgumentList()->GetString(0),
                                               message->GetArgumentList()->GetString(1),
                                               message->GetArgumentList()->GetInt(2));
    return true;
  }
  else if (message_name == RendererMessage::FocusedNodeChanged)
  {
    m_focusedField.focusOnEditableField = message->GetArgumentList()->GetBool(0);
    m_focusedField.isEditable = message->GetArgumentList()->GetBool(1);
    m_focusedField.x = message->GetArgumentList()->GetInt(2);
    m_focusedField.y = message->GetArgumentList()->GetInt(3);
    m_focusedField.width = message->GetArgumentList()->GetInt(4);
    m_focusedField.height = message->GetArgumentList()->GetInt(5);
    m_focusedField.type = message->GetArgumentList()->GetString(6);
    m_focusedField.value = message->GetArgumentList()->GetString(7);
    return true;
  }
  else if (message_name == RendererMessage::SendString)
  {
    if (m_focusedField.focusOnEditableField)
    {
      CefRange replacement_range;
      replacement_range.from = 0;
      replacement_range.to = -1;
      m_browser->GetHost()->ImeCommitText(message->GetArgumentList()->GetString(0), replacement_range, 0);
    }
    return true;
  }
  else if (message_name == RendererMessage::ShowKeyboard)
  {
    std::string type = message->GetArgumentList()->GetString(0);
    std::string header = message->GetArgumentList()->GetString(1);
    std::string value = message->GetArgumentList()->GetString(2);
    std::string id = message->GetArgumentList()->GetString(3);
    std::string name = message->GetArgumentList()->GetString(4);
    std::string markup = message->GetArgumentList()->GetString(5);

    if (header.empty())
    {
      if (type == "password")
        header = kodi::GetLocalizedString(30012);
      else if (type == "text")
        header = kodi::GetLocalizedString(30013);
      else if (type == "textarea")
        header = kodi::GetLocalizedString(30014);
    }

    if (kodi::gui::dialogs::Keyboard::ShowAndGetInput(value, header, true, type == "password"))
    {
      std::string code;
      if (!id.empty())
        code = "document.getElementById(\"" + id + "\").value = \"" + value + "\"\n";
      else
      {
        code =
          "var myelements = document.getElementsByName(\"" + name + "\")\n"
          "for (var i=0; i<myelements.length; i++) {\n"
          "  if (myelements[i].nodeType != 1) {\n"
          "    break;\n"
          "  }\n"
          "  myelements[i].value = \"" + value + "\"\n"
          "}\n";
      }
      browser->GetMainFrame()->ExecuteJavaScript(code, browser->GetFocusedFrame()->GetURL(), 0);
    }
  }
  else if (message_name == RendererMessage::ShowSelect)
  {
    std::string type = message->GetArgumentList()->GetString(0);
    std::string header = message->GetArgumentList()->GetString(1);
    std::string value = message->GetArgumentList()->GetString(2);
    std::string id = message->GetArgumentList()->GetString(3);
    std::string name = message->GetArgumentList()->GetString(4);
    std::string markup = message->GetArgumentList()->GetString(5);

    std::vector<SSelectionEntry> entries;
    for (int i = 0; i < message->GetArgumentList()->GetInt(6); ++i)
    {
      SSelectionEntry entry;
      entry.id = message->GetArgumentList()->GetString(i*3+0+7);
      entry.name = message->GetArgumentList()->GetString(i*3+1+7);
      entry.selected = message->GetArgumentList()->GetBool(i*3+2+7);
      entries.push_back(std::move(entry));
    }

    if (!entries.empty())
    {
      bool ret = false;
      if (type == "select-multiple")
        ret = kodi::gui::dialogs::Select::ShowMultiSelect("", entries);
      else
        ret = kodi::gui::dialogs::Select::Show("", entries) >= 0;

      if (ret)
      {
        std::string values = "";
        for (const auto& entry : entries)
        {
          if (entry.selected)
            values += entry.id + ",";
        }
        const std::string& code =
            "var values = \"" + values + "\";\n"
            "var myselect = document.getElementsByName(\"" + name +"\");\n"
            "for (var i=0; i<myselect.length; i++) {\n"
            "  if (myselect[i].nodeType != 1)\n"
            "    continue;\n"
            "  for (var j=0; j<myselect[i].options.length; j++) {\n"
            "    myselect[i].options[j].selected=false;\n"
            "  }\n"
            "  values.split(',').forEach(function(v) {\n"
            "    Array.from(myselect[i]).find(c => c.value == v).selected = true;\n"
            "  });\n"
            "}\n";
        browser->GetMainFrame()->ExecuteJavaScript(code, browser->GetFocusedFrame()->GetURL(), 0);
      }
    }
  }

  return false;
}
//@}

/// CefDisplayHandler methods
//@{
void CWebBrowserClient::OnAddressChange(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& url)
{
  if (frame->IsMain())
  {
    m_currentURL = url.ToString();

    Message tMsg = {TMSG_SET_OPENED_ADDRESS};
    tMsg.strParam = url.ToString().c_str();
    SendMessage(tMsg, false);
  }
}

void CWebBrowserClient::OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title)
{
  if (m_currentTitle != title.ToString().c_str())
  {
    m_currentTitle = title.ToString().c_str();

    Message tMsg = {TMSG_SET_OPENED_TITLE};
    tMsg.strParam = m_currentTitle.c_str();
    SendMessage(tMsg, false);
  }
}

void CWebBrowserClient::OnFaviconURLChange(CefRefPtr<CefBrowser> browser, const std::vector<CefString>& icon_urls)
{
  LOG_INTERNAL_MESSAGE(ADDON_LOG_DEBUG, "From currently opened web site given icon urls (first one used)");
  unsigned int listSize = icon_urls.size();
  for (unsigned int i = 0; i < listSize; ++i)
    LOG_INTERNAL_MESSAGE(ADDON_LOG_DEBUG, " - Icon %i - %s", i+1, icon_urls[i].ToString().c_str());

  Message tMsg = {TMSG_SET_ICON_URL};
  if (listSize > 0)
    m_currentIcon = icon_urls[0].ToString();
  else
    m_currentIcon = "";

  tMsg.strParam = m_currentIcon;
  SendMessage(tMsg, false);
}

void CWebBrowserClient::OnFullscreenModeChange(CefRefPtr<CefBrowser> browser, bool fullscreen)
{
  if (m_isFullScreen != fullscreen)
  {
    m_isFullScreen = fullscreen;

    Message tMsg = {TMSG_FULLSCREEN_MODE_CHANGE};
    tMsg.param1 = fullscreen ? 1 : 0;
    SendMessage(tMsg, false);
  }
}

bool CWebBrowserClient::OnTooltip(CefRefPtr<CefBrowser> browser, CefString& text)
{
  if (m_currentTooltip != text.ToString().c_str())
  {
    m_currentTooltip = text.ToString().c_str();

    Message tMsg = {TMSG_SET_TOOLTIP};
    tMsg.strParam = m_currentTooltip.c_str();
    SendMessage(tMsg, false);
  }

  return true;
}

void CWebBrowserClient::OnStatusMessage(CefRefPtr<CefBrowser> browser, const CefString& value)
{
  if (m_currentStatusMsg != value.ToString().c_str())
  {
    m_currentStatusMsg = value.ToString().c_str();

    Message tMsg = {TMSG_SET_STATUS_MESSAGE};
    tMsg.strParam = m_currentStatusMsg.c_str();
    SendMessage(tMsg, false);
  }
}

bool CWebBrowserClient::OnConsoleMessage(CefRefPtr<CefBrowser> browser, const CefString& message, const CefString& source, int line)
{
  LOG_INTERNAL_MESSAGE(ADDON_LOG_ERROR, "%s - Message: %s - Source: %s - Line: %i", __FUNCTION__,
                       message.ToString().c_str(), source.ToString().c_str(), line);
  return true;
}
//@}

/// CefLifeSpanHandler methods
//@{
bool CWebBrowserClient::OnBeforePopup(CefRefPtr<CefBrowser> browser,
                                      CefRefPtr<CefFrame> frame,
                                      const CefString& target_url,
                                      const CefString& target_frame_name,
                                      CefRequestHandler::WindowOpenDisposition target_disposition,
                                      bool user_gesture,
                                      const CefPopupFeatures& popupFeatures,
                                      CefWindowInfo& windowInfo,
                                      CefRefPtr<CefClient>& client,
                                      CefBrowserSettings& settings,
                                      bool* no_javascript_access)
{
// #ifdef DEBUG_LOGS
  LOG_MESSAGE(ADDON_LOG_DEBUG, "--------------------------------------->%s - %s", __FUNCTION__, std::string(target_url).c_str());
  LOG_MESSAGE(ADDON_LOG_DEBUG, " - target_url '%s'", std::string(target_url).c_str());
  LOG_MESSAGE(ADDON_LOG_DEBUG, " - target_frame_name '%s'", std::string(target_frame_name).c_str());
  LOG_MESSAGE(ADDON_LOG_DEBUG, " - user_gesture '%i'", user_gesture);
  LOG_MESSAGE(ADDON_LOG_DEBUG, " - no_javascript_access '%i'", *no_javascript_access);
  LOG_MESSAGE(ADDON_LOG_DEBUG, " - windowInfo.x '%i'", windowInfo.x);
  LOG_MESSAGE(ADDON_LOG_DEBUG, " - windowInfo.y '%i'", windowInfo.y);
  LOG_MESSAGE(ADDON_LOG_DEBUG, " - windowInfo.height '%i'", windowInfo.height);
  LOG_MESSAGE(ADDON_LOG_DEBUG, " - windowInfo.width '%i'", windowInfo.width);
  LOG_MESSAGE(ADDON_LOG_DEBUG, " - windowInfo.windowless_rendering_enabled '%i'", windowInfo.windowless_rendering_enabled);
  LOG_MESSAGE(ADDON_LOG_DEBUG, " - popupFeatures.dialog '%i'", popupFeatures.dialog);
  LOG_MESSAGE(ADDON_LOG_DEBUG, " - popupFeatures.fullscreen '%i'", popupFeatures.fullscreen);
  LOG_MESSAGE(ADDON_LOG_DEBUG, " - popupFeatures.height '%i'", popupFeatures.height);
  LOG_MESSAGE(ADDON_LOG_DEBUG, " - popupFeatures.width '%i'", popupFeatures.width);
  LOG_MESSAGE(ADDON_LOG_DEBUG, " - popupFeatures.heightSet '%i'", popupFeatures.heightSet);
  LOG_MESSAGE(ADDON_LOG_DEBUG, " - popupFeatures.widthSet '%i'", popupFeatures.widthSet);
  LOG_MESSAGE(ADDON_LOG_DEBUG, " - popupFeatures.locationBarVisible '%i'", popupFeatures.locationBarVisible);
  LOG_MESSAGE(ADDON_LOG_DEBUG, " - popupFeatures.menuBarVisible '%i'", popupFeatures.menuBarVisible);
  LOG_MESSAGE(ADDON_LOG_DEBUG, " - popupFeatures.resizable '%i'", popupFeatures.resizable);
  LOG_MESSAGE(ADDON_LOG_DEBUG, " - popupFeatures.scrollbarsVisible '%i'", popupFeatures.scrollbarsVisible);
  LOG_MESSAGE(ADDON_LOG_DEBUG, " - popupFeatures.statusBarVisible '%i'", popupFeatures.statusBarVisible);
  LOG_MESSAGE(ADDON_LOG_DEBUG, " - popupFeatures.toolBarVisible '%i'", popupFeatures.toolBarVisible);
  LOG_MESSAGE(ADDON_LOG_DEBUG, " - target_disposition '%i'", target_disposition);
// #endif

  if (target_disposition == WOD_UNKNOWN)
  {
    kodi::Log(ADDON_LOG_ERROR, "Browser popup requested with unknown target disposition");
    return false;
  }

  windowInfo.windowless_rendering_enabled = true;

  if (kodi::GetSettingBoolean("main.allow_open_to_tabs") && target_disposition != WOD_CURRENT_TAB)
    RequestOpenSiteInNewTab(target_url); /* Request to do on kodi itself */
  else
    OpenWebsite(std::string(target_url), false, false);
  return false; /* Cancel popups in off-screen rendering mode */
}

void CWebBrowserClient::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
  CEF_REQUIRE_UI_THREAD();

  m_browserCount++;

  if (m_messageRouter == nullptr)
  {
    // Create the browser-side router for query handling.
    CefMessageRouterConfig config;
    m_messageRouter = CefMessageRouterBrowserSide::Create(config);

    // Register handlers with the router.
    CreateMessageHandlers(m_messageHandlers);
    for (const auto& entry : m_messageHandlers)
      m_messageRouter->AddHandler(entry, false);
  }

  CLockObject lock(m_Mutex);
  if (!m_browser.get())
  {
    m_browser = browser;
    m_browserId = browser->GetIdentifier();

    /* Inform Kodi the control is ready */
    SetControlReady(true);
  }

  if (kodi::GetSettingBoolean("system.mouse_cursor_change_disabled"))
    browser->GetHost()->SetMouseCursorChangeDisabled(true);
}

bool CWebBrowserClient::DoClose(CefRefPtr<CefBrowser> browser)
{
  CEF_REQUIRE_UI_THREAD();
  return false; /* Allow the close. For windowed browsers this will result in the OS close event being sent */
}

void CWebBrowserClient::OnBeforeClose(CefRefPtr<CefBrowser> browser)
{
  CEF_REQUIRE_UI_THREAD();

  m_messageRouter->OnBeforeClose(browser);
  if (--m_browserCount == 0)
  {
    for (const auto& entry : m_messageHandlers)
      m_messageRouter->RemoveHandler(entry);
    m_messageHandlers.clear();
    m_messageRouter = nullptr;
  }
}
//@}

/// CefRequestHandler methods
//@{
bool CWebBrowserClient::OnBeforeBrowse(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, bool is_redirect)
{
  CEF_REQUIRE_UI_THREAD();

  m_messageRouter->OnBeforeBrowse(browser, frame);
  return false;
}

bool CWebBrowserClient::OnOpenURLFromTab(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString& target_url,
                                         CefRequestHandler::WindowOpenDisposition target_disposition, bool user_gesture)
{
  return false;
}

CefRequestHandler::ReturnValue CWebBrowserClient::OnBeforeResourceLoad(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                                                       CefRefPtr<CefRequest> request, CefRefPtr<CefRequestCallback> callback)
{
  CEF_REQUIRE_IO_THREAD();

#if DEBUG_LOGS
  LOG_MESSAGE(ADDON_LOG_DEBUG, "OnBeforeResourceLoad:");
  LOG_MESSAGE(ADDON_LOG_DEBUG, " - url = '%s'", request->GetURL().ToString().c_str());
  LOG_MESSAGE(ADDON_LOG_DEBUG, " - method = '%s'", request->GetMethod().ToString().c_str());
  LOG_MESSAGE(ADDON_LOG_DEBUG, " - flags = %i", request->GetFlags());
  LOG_MESSAGE(ADDON_LOG_DEBUG, " - resourceType = %i", static_cast<int>(request->GetResourceType()));
  LOG_MESSAGE(ADDON_LOG_DEBUG, " - transitionType = %i", static_cast<int>(request->GetTransitionType()));
#endif

  return m_resourceManager->OnBeforeResourceLoad(browser, frame, request, callback);
}

CefRefPtr<CefResourceHandler> CWebBrowserClient::GetResourceHandler(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                                                    CefRefPtr<CefRequest> request)
{
  CEF_REQUIRE_IO_THREAD();

  return m_resourceManager->GetResourceHandler(browser, frame, request);
}

void CWebBrowserClient::OnResourceRedirect(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                           CefRefPtr<CefRequest> request, CefRefPtr<CefResponse> response, CefString& new_url)
{
#if DEBUG_LOGS
  LOG_MESSAGE(ADDON_LOG_DEBUG, "OnResourceRedirect:");
  LOG_MESSAGE(ADDON_LOG_DEBUG, " - status = %i", response->GetStatus());
  LOG_MESSAGE(ADDON_LOG_DEBUG, " - statusText = '%s'", response->GetStatusText().ToString().c_str());
  LOG_MESSAGE(ADDON_LOG_DEBUG, " - contentType = '%s'", response->GetMimeType().ToString().c_str());
  LOG_MESSAGE(ADDON_LOG_DEBUG, " - url = '%s'", request->GetURL().ToString().c_str());
  LOG_MESSAGE(ADDON_LOG_DEBUG, " - id = %lu", request->GetIdentifier());
  LOG_MESSAGE(ADDON_LOG_DEBUG, " - new_url = '%s'", new_url.ToString().c_str());
#endif
}

bool CWebBrowserClient::OnResourceResponse(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                           CefRefPtr<CefRequest> request, CefRefPtr<CefResponse> response)
{
#if DEBUG_LOGS
  LOG_MESSAGE(ADDON_LOG_DEBUG, "onResourceReceived:");
  LOG_MESSAGE(ADDON_LOG_DEBUG, " - status = %i", response->GetStatus());
  LOG_MESSAGE(ADDON_LOG_DEBUG, " - statusText = '%s'", response->GetStatusText().ToString().c_str());
  LOG_MESSAGE(ADDON_LOG_DEBUG, " - contentType = '%s'", response->GetMimeType().ToString().c_str());
  LOG_MESSAGE(ADDON_LOG_DEBUG, " - url = '%s'", request->GetURL().ToString().c_str());
  LOG_MESSAGE(ADDON_LOG_DEBUG, " - id = %lu", request->GetIdentifier());
#endif
  return false;
}

bool CWebBrowserClient::GetAuthCredentials(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, bool isProxy, const CefString& host,
                                           int port, const CefString& realm, const CefString& scheme, CefRefPtr<CefAuthCallback> callback)
{
  ///TODO Useful and secure?
  return false;
}

bool CWebBrowserClient::OnQuotaRequest(CefRefPtr<CefBrowser> browser, const CefString& origin_url, int64 new_size, CefRefPtr<CefRequestCallback> callback)
{
  CEF_REQUIRE_IO_THREAD();

  static const int64 max_size = 1024 * 1024 * 20;  // 20mb.
  if (new_size > max_size)
    kodi::Log(ADDON_LOG_DEBUG, "JavaScript on '%s' requests a specific storage quota with size %li MBytes who becomes not granded",
                                origin_url.ToString().c_str(), new_size/1024/1024);

  // Grant the quota request if the size is reasonable.
  callback->Continue(new_size <= max_size);
  return true;
}

void CWebBrowserClient::OnProtocolExecution(CefRefPtr<CefBrowser> browser, const CefString& url, bool& allow_os_execution)
{
  CEF_REQUIRE_UI_THREAD();

  std::string urlStr = url;

  // Allow OS execution of Spotify URIs.
  if (urlStr.find("spotify:") == 0)
    allow_os_execution = true;
}

bool CWebBrowserClient::OnCertificateError(CefRefPtr<CefBrowser> browser, ErrorCode cert_error,
                                           const CefString& request_url, CefRefPtr<CefSSLInfo> ssl_info,
                                           CefRefPtr<CefRequestCallback> callback)
{
  CEF_REQUIRE_UI_THREAD();

  CefRefPtr<CefX509Certificate> cert = ssl_info->GetX509Certificate();
  if (cert.get())
  {
    bool canceled = false;
    std::string subject = cert->GetSubject()->GetDisplayName().ToString();
    std::string certStatusString = GetCertStatusString(ssl_info->GetCertStatus());
    std::string text = StringUtils::Format(kodi::GetLocalizedString(31001).c_str(), subject.c_str(), certStatusString.c_str(), subject.c_str());
    bool ret = kodi::gui::dialogs::YesNo::ShowAndGetInput(kodi::GetLocalizedString(31000), text, canceled,
                                                          kodi::GetLocalizedString(31003), kodi::GetLocalizedString(31002));

#ifdef SHOW_ERROR_PAGE
    if (!ret)
    {
      //Load the error page.
      LoadErrorPage(browser->GetMainFrame(), request_url, cert_error, GetCertificateInformation(cert, ssl_info->GetCertStatus()));
    }
#endif
    callback->Continue(ret);
    return true;
  }

  return false;  // Cancel the request.
}

void CWebBrowserClient::OnPluginCrashed(CefRefPtr<CefBrowser> browser, const CefString& plugin_path)
{
  kodi::Log(ADDON_LOG_ERROR, "Browser Plugin '%s' crashed (URL: '%s'", plugin_path.ToString().c_str(),
                                                                       browser->GetFocusedFrame()->GetURL().ToString().c_str());
}

void CWebBrowserClient::OnRenderViewReady(CefRefPtr<CefBrowser> browser)
{
  fprintf(stderr, "void CWebBrowserClient::OnRenderViewReady(CefRefPtr<CefBrowser> browser)\n");
  m_renderViewReady = true;
}

void CWebBrowserClient::OnRenderProcessTerminated(CefRefPtr<CefBrowser> browser, TerminationStatus status)
{
  LOG_MESSAGE(ADDON_LOG_DEBUG, "%s", __FUNCTION__);
  CEF_REQUIRE_UI_THREAD();

  m_messageRouter->OnRenderProcessTerminated(browser);

  // Don't reload if there's no start URL, or if the crash URL was specified.
  if (m_strStartupURL.empty() || m_strStartupURL == "chrome://crash")
    return;

  CefRefPtr<CefFrame> frame = browser->GetMainFrame();
  std::string url = frame->GetURL();

  // Don't reload if the termination occurred before any URL had successfully
  // loaded.
  if (url.empty())
    return;

  std::string start_url = m_strStartupURL;
  StringUtils::ToLower(url);
  StringUtils::ToLower(start_url);

  // Don't reload the URL that just resulted in termination.
  if (url.find(start_url) == 0)
    return;

  frame->LoadURL(m_strStartupURL);
}
//@}

/// CefFindHandler methods
//@{
void CWebBrowserClient::OnFindResult(CefRefPtr<CefBrowser> browser, int identifier, int count, const CefRect& selectionRect,
                                     int activeMatchOrdinal, bool finalUpdate)
{
  if (finalUpdate && activeMatchOrdinal <= 1)
  {
    std::string text = StringUtils::Format(kodi::GetLocalizedString(30038).c_str(), count, m_currentSearchText.c_str());
    kodi::QueueNotification(QUEUE_INFO, kodi::GetLocalizedString(30037), text);
  }
  LOG_MESSAGE(ADDON_LOG_DEBUG, "%s -identifier %i, count %i finalUpdate %i activeMatchOrdinal %i", __FUNCTION__, identifier, count, finalUpdate, activeMatchOrdinal);
}
//@}

/// CefContextMenuHandler methods
//@{
void CWebBrowserClient::OnBeforeContextMenu(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                                CefRefPtr<CefContextMenuParams> params, CefRefPtr<CefMenuModel> model)
{
  std::string url = params->GetLinkUrl().ToString();
  if (!url.empty())
  {
    model->InsertItemAt(0, CLIENT_ID_OPEN_SELECTED_SIDE, kodi::GetLocalizedString(30000 + CLIENT_ID_OPEN_SELECTED_SIDE));
    model->InsertItemAt(0, CLIENT_ID_OPEN_SELECTED_SIDE_IN_NEW_TAB, kodi::GetLocalizedString(30000 + CLIENT_ID_OPEN_SELECTED_SIDE_IN_NEW_TAB));

  }

  int flags = params->GetTypeFlags();
  if (flags & CM_TYPEFLAG_EDITABLE)
    model->InsertItemAt(0, CLIENT_ID_OPEN_KEYBOARD, kodi::GetLocalizedString(30000 + CLIENT_ID_OPEN_KEYBOARD));

#ifdef DEBUG_LOGS
  LOG_MESSAGE(ADDON_LOG_DEBUG, "CefContextMenuParams");
  LOG_MESSAGE(ADDON_LOG_DEBUG, "- %ix%i - TypeFlags: 0x%X - ImageContents: %s - MediaType: %i - MediaStateFlags %i - EditStateFlags %i", params->GetXCoord(),
      params->GetYCoord(), (int)params->GetTypeFlags(), params->HasImageContents() ? "yes" : "no",
      (int)params->GetMediaType(), (int)params->GetMediaStateFlags(), (int)params->GetEditStateFlags());
  LOG_MESSAGE(ADDON_LOG_DEBUG, "- LinkUrl:                %s", params->GetLinkUrl().ToString().c_str());
  LOG_MESSAGE(ADDON_LOG_DEBUG, "- UnfilteredLinkUrl:      %s", params->GetUnfilteredLinkUrl().ToString().c_str());
  LOG_MESSAGE(ADDON_LOG_DEBUG, "- SourceUrl:              %s", params->GetSourceUrl().ToString().c_str());
  LOG_MESSAGE(ADDON_LOG_DEBUG, "- PageUrl:                %s", params->GetPageUrl().ToString().c_str());
  LOG_MESSAGE(ADDON_LOG_DEBUG, "- FrameUrl :              %s", params->GetFrameUrl().ToString().c_str());
  LOG_MESSAGE(ADDON_LOG_DEBUG, "- FrameCharset :          %s", params->GetFrameCharset().ToString().c_str());
  LOG_MESSAGE(ADDON_LOG_DEBUG, "- SelectionText :         %s", params->GetSelectionText().ToString().c_str());
  LOG_MESSAGE(ADDON_LOG_DEBUG, "- MisspelledWord :        %s", params->GetMisspelledWord().ToString().c_str());
  std::vector<CefString> suggestions;
  LOG_MESSAGE(ADDON_LOG_DEBUG, "- DictionarySuggestions : %s", params->GetDictionarySuggestions(suggestions) ? "OK" : "fail");
  for (unsigned int i = 0; i < suggestions.size(); i++)
    LOG_MESSAGE(ADDON_LOG_DEBUG, "  - %02i: %s", i, suggestions[i].ToString().c_str());
  LOG_MESSAGE(ADDON_LOG_DEBUG, "- IsEditable :            %s", params->IsEditable() ? "yes" : "no");
  LOG_MESSAGE(ADDON_LOG_DEBUG, "- IsSpellCheckEnabled :   %s", params->IsSpellCheckEnabled() ? "yes" : "no");
  LOG_MESSAGE(ADDON_LOG_DEBUG, "- IsCustomMenu :          %s", params->IsCustomMenu() ? "yes" : "no");
  LOG_MESSAGE(ADDON_LOG_DEBUG, "- IsPepperMenu :          %s", params->IsPepperMenu() ? "yes" : "no");
  LOG_MESSAGE(ADDON_LOG_DEBUG, "CefMenuModel");
  LOG_MESSAGE(ADDON_LOG_DEBUG, "- Count:                  %i", model->GetCount());
  for (unsigned int i = 0; i < model->GetCount(); i++)
    LOG_MESSAGE(ADDON_LOG_DEBUG, "  - %02i: ID '%i' Type '%i' - Name '%s'",
                    i, model->GetCommandIdAt(i), model->GetTypeAt(i), model->GetLabelAt(i).ToString().c_str());
#endif
}

bool CWebBrowserClient::RunContextMenu(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefContextMenuParams> params,
                                           CefRefPtr<CefMenuModel> model, CefRefPtr<CefRunContextMenuCallback> callback)
{
  std::vector<std::pair<int, std::string>> entries;
  for (unsigned int i = 0; i < model->GetCount(); ++i)
  {
    int id = model->GetCommandIdAt(i);
    if (id < 0 ||
        id == MENU_ID_PRINT ||
        id == MENU_ID_VIEW_SOURCE ||
        !model->IsEnabled(id))
    {
      // ignored parts!
      continue;
    }

    cef_menu_item_type_t type = model->GetTypeAt(i);
    if (type == MENUITEMTYPE_SEPARATOR)
    {
      // ignore separators
      continue;
    }
    else if (type != MENUITEMTYPE_COMMAND)
    {
      // TODO add support for other formats e.g. boolean check
      LOG_MESSAGE(ADDON_LOG_ERROR, "cef_menu_item_type_t '%i' currently not supported!", type);
      continue;
    }

    entries.push_back(std::pair<int, std::string>(id, kodi::GetLocalizedString(30000 + id)));
  }

  if (entries.empty())
  {
    callback->Cancel();
    return true;
  }

  int ret = kodi::gui::dialogs::ContextMenu::Show("", entries);
  if (ret >= 0)
    callback->Continue(entries[ret].first, EVENTFLAG_LEFT_MOUSE_BUTTON);
  else
    callback->Cancel();

  return true;
}

bool CWebBrowserClient::OnContextMenuCommand(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefContextMenuParams> params,
                                                 int command_id, EventFlags event_flags)
{
  if (command_id == CLIENT_ID_OPEN_SELECTED_SIDE)
  {
    std::string url = params->GetLinkUrl().ToString();
    OpenWebsite(url, false, false);
  }
  if (command_id == CLIENT_ID_OPEN_SELECTED_SIDE_IN_NEW_TAB)
  {
    std::string url = params->GetLinkUrl().ToString();
    RequestOpenSiteInNewTab(url);
  }
  else if (command_id == CLIENT_ID_OPEN_KEYBOARD)
  {
    if (m_focusedField.focusOnEditableField)
    {
      CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create(AddonClientMessage::FocusedSelected);
      browser->SendProcessMessage(PID_RENDERER, message);
    }
    else
    {
      std::string header;
      if (m_focusedField.type == "password")
        header = kodi::GetLocalizedString(30012);
      else if (m_focusedField.type == "text")
        header = kodi::GetLocalizedString(30013);
      else if (m_focusedField.type == "textarea")
        header = kodi::GetLocalizedString(30014);

      if (kodi::gui::dialogs::Keyboard::ShowAndGetInput(m_focusedField.value, header, true, m_focusedField.type == "password"))
      {
        CefRange replacement_range;
        replacement_range.from = 0;
        replacement_range.to = -1;
        browser->GetHost()->ImeCommitText(m_focusedField.value, replacement_range, 0);
      }
    }
  }
  else
  {
    return false;
  }

  return true;
}
//@}

/// CefLoadHandler methods
//@{
void CWebBrowserClient::OnLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward)
{
  CEF_REQUIRE_UI_THREAD();

//   Message tMsg = {TMSG_SET_LOADING_STATE};
//   tMsg.param1 = isLoading;
//   tMsg.param2 = canGoBack;
//   tMsg.param3 = canGoForward;
//   SendMessage(tMsg, false);
  SetLoadingState(isLoading, canGoBack, canGoForward);
}


void CWebBrowserClient::OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, TransitionType transition_type)
{
  LOG_MESSAGE(ADDON_LOG_DEBUG, "Load started (id='%d', URL='%s'", browser->GetIdentifier(), frame->GetURL().ToString().c_str());
  CEF_REQUIRE_UI_THREAD();

  m_isLoading = true;
  Initialize();
}

void CWebBrowserClient::OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode)
{
  LOG_MESSAGE(ADDON_LOG_DEBUG, "Load done with status code '%i'", httpStatusCode);
  CEF_REQUIRE_UI_THREAD();
  m_isLoading = false;

  class CHistoryReporter : public CefNavigationEntryVisitor
  {
  public:
    CHistoryReporter(std::vector<std::pair<std::string, bool>>& historyWebsiteNames) : m_historyWebsiteNames(historyWebsiteNames)
    {
      m_historyWebsiteNames.clear();
    }
    virtual bool Visit(CefRefPtr<CefNavigationEntry> entry, bool current, int index, int total) override
    {
      m_historyWebsiteNames.push_back(std::pair<std::string, bool>(entry->GetTitle(), current));
      return true;
    }

  private:
    std::vector<std::pair<std::string, bool>>& m_historyWebsiteNames;
    IMPLEMENT_REFCOUNTING(CHistoryReporter);
  };
  browser->GetHost()->GetNavigationEntries(new CHistoryReporter(m_historyWebsiteNames), false);
}

void CWebBrowserClient::OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode,
                                        const CefString& errorText, const CefString& failedUrl)
{
  CEF_REQUIRE_UI_THREAD();

  // Don't display an error for downloaded files.
  if (errorCode == ERR_ABORTED)
    return;

  // Don't display an error for external protocols that we allow the OS to
  // handle. See OnProtocolExecution().
  if (errorCode == ERR_UNKNOWN_URL_SCHEME)
  {
    std::string urlStr = frame->GetURL();
    if (urlStr.find("spotify:") == 0)
      return;
  }

  kodi::QueueNotification(QUEUE_WARNING, kodi::GetLocalizedString(30133), failedUrl.ToString());

  // Load the error page.
  LoadErrorPage(frame, failedUrl, errorCode, errorText);
}
//@}

// -----------------------------------------------------------------------------

int CWebBrowserClient::ZoomLevelToPercentage(double zoomlevel)
{
  return int((zoomlevel*ZOOM_MULTIPLY)+100.0);
}

double CWebBrowserClient::PercentageToZoomLevel(int percent)
{
  return (double(percent-100))/ZOOM_MULTIPLY;
}

void CWebBrowserClient::LoadErrorPage(CefRefPtr<CefFrame> frame,
                   const std::string& failed_url,
                   cef_errorcode_t error_code,
                   const std::string& other_info)
{
  std::stringstream ss;
  ss << "<html><head><title>"<< kodi::GetLocalizedString(30133) << "</title></head>"
        "<body bgcolor=\"white\">"
        "<h3>"<< kodi::GetLocalizedString(30133) << ".</h3>"
        "URL: <a href=\""
     << failed_url << "\">" << failed_url
     << "</a><br/>Error: " << GetErrorString(error_code) << " ("
     << error_code << ")";

  if (!other_info.empty())
    ss << "<br/>" << other_info;

  ss << "</body></html>";
  frame->LoadURL(GetDataURI(ss.str(), "text/html"));
}

std::string CWebBrowserClient::GetCertStatusString(cef_cert_status_t status)
{
#define FLAG(flag)                          \
  if (status & flag) {                      \
    result += std::string(#flag); \
  }

  std::string result;

  FLAG(CERT_STATUS_COMMON_NAME_INVALID);
  FLAG(CERT_STATUS_DATE_INVALID);
  FLAG(CERT_STATUS_AUTHORITY_INVALID);
  FLAG(CERT_STATUS_NO_REVOCATION_MECHANISM);
  FLAG(CERT_STATUS_UNABLE_TO_CHECK_REVOCATION);
  FLAG(CERT_STATUS_REVOKED);
  FLAG(CERT_STATUS_INVALID);
  FLAG(CERT_STATUS_WEAK_SIGNATURE_ALGORITHM);
  FLAG(CERT_STATUS_NON_UNIQUE_NAME);
  FLAG(CERT_STATUS_WEAK_KEY);
  FLAG(CERT_STATUS_PINNED_KEY_MISSING);
  FLAG(CERT_STATUS_NAME_CONSTRAINT_VIOLATION);
  FLAG(CERT_STATUS_VALIDITY_TOO_LONG);
  FLAG(CERT_STATUS_IS_EV);
  FLAG(CERT_STATUS_REV_CHECKING_ENABLED);
  FLAG(CERT_STATUS_SHA1_SIGNATURE_PRESENT);
  FLAG(CERT_STATUS_CT_COMPLIANCE_FAILED);

  if (result.empty())
    return "&nbsp;";
  return result;
}

std::string CWebBrowserClient::GetSSLVersionString(cef_ssl_version_t version)
{
#define VALUE(val, def)       \
  if (val == def) {           \
    return std::string(#def); \
  }

  VALUE(version, SSL_CONNECTION_VERSION_UNKNOWN);
  VALUE(version, SSL_CONNECTION_VERSION_SSL2);
  VALUE(version, SSL_CONNECTION_VERSION_SSL3);
  VALUE(version, SSL_CONNECTION_VERSION_TLS1);
  VALUE(version, SSL_CONNECTION_VERSION_TLS1_1);
  VALUE(version, SSL_CONNECTION_VERSION_TLS1_2);
  VALUE(version, SSL_CONNECTION_VERSION_QUIC);
  return std::string();
}

std::string CWebBrowserClient::GetErrorString(cef_errorcode_t code)
{
// Case condition that returns |code| as a string.
#define CASE(code) \
  case code:       \
    return #code

  switch (code) {
    CASE(ERR_NONE);
    CASE(ERR_FAILED);
    CASE(ERR_ABORTED);
    CASE(ERR_INVALID_ARGUMENT);
    CASE(ERR_INVALID_HANDLE);
    CASE(ERR_FILE_NOT_FOUND);
    CASE(ERR_TIMED_OUT);
    CASE(ERR_FILE_TOO_BIG);
    CASE(ERR_UNEXPECTED);
    CASE(ERR_ACCESS_DENIED);
    CASE(ERR_NOT_IMPLEMENTED);
    CASE(ERR_CONNECTION_CLOSED);
    CASE(ERR_CONNECTION_RESET);
    CASE(ERR_CONNECTION_REFUSED);
    CASE(ERR_CONNECTION_ABORTED);
    CASE(ERR_CONNECTION_FAILED);
    CASE(ERR_NAME_NOT_RESOLVED);
    CASE(ERR_INTERNET_DISCONNECTED);
    CASE(ERR_SSL_PROTOCOL_ERROR);
    CASE(ERR_ADDRESS_INVALID);
    CASE(ERR_ADDRESS_UNREACHABLE);
    CASE(ERR_SSL_CLIENT_AUTH_CERT_NEEDED);
    CASE(ERR_TUNNEL_CONNECTION_FAILED);
    CASE(ERR_NO_SSL_VERSIONS_ENABLED);
    CASE(ERR_SSL_VERSION_OR_CIPHER_MISMATCH);
    CASE(ERR_SSL_RENEGOTIATION_REQUESTED);
    CASE(ERR_CERT_COMMON_NAME_INVALID);
    CASE(ERR_CERT_DATE_INVALID);
    CASE(ERR_CERT_AUTHORITY_INVALID);
    CASE(ERR_CERT_CONTAINS_ERRORS);
    CASE(ERR_CERT_NO_REVOCATION_MECHANISM);
    CASE(ERR_CERT_UNABLE_TO_CHECK_REVOCATION);
    CASE(ERR_CERT_REVOKED);
    CASE(ERR_CERT_INVALID);
    CASE(ERR_CERT_END);
    CASE(ERR_INVALID_URL);
    CASE(ERR_DISALLOWED_URL_SCHEME);
    CASE(ERR_UNKNOWN_URL_SCHEME);
    CASE(ERR_TOO_MANY_REDIRECTS);
    CASE(ERR_UNSAFE_REDIRECT);
    CASE(ERR_UNSAFE_PORT);
    CASE(ERR_INVALID_RESPONSE);
    CASE(ERR_INVALID_CHUNKED_ENCODING);
    CASE(ERR_METHOD_NOT_SUPPORTED);
    CASE(ERR_UNEXPECTED_PROXY_AUTH);
    CASE(ERR_EMPTY_RESPONSE);
    CASE(ERR_RESPONSE_HEADERS_TOO_BIG);
    CASE(ERR_CACHE_MISS);
    CASE(ERR_INSECURE_RESPONSE);
    default:
      return "UNKNOWN";
  }
}

std::string CWebBrowserClient::GetDataURI(const std::string& data, const std::string& mime_type)
{
  return "data:" + mime_type + ";base64," +
         CefURIEncode(CefBase64Encode(data.data(), data.size()), false)
             .ToString();
}

std::string GetTimeString(const CefTime& value)
{
  if (value.GetTimeT() == 0)
    return "Unspecified";

  static const char* kMonths[] = {
      "January", "February", "March",     "April",   "May",      "June",
      "July",    "August",   "September", "October", "November", "December"};
  std::string month;
  if (value.month >= 1 && value.month <= 12)
    month = kMonths[value.month - 1];
  else
    month = "Invalid";

  std::stringstream ss;
  ss << month << " " << value.day_of_month << ", " << value.year << " "
     << std::setfill('0') << std::setw(2) << value.hour << ":"
     << std::setfill('0') << std::setw(2) << value.minute << ":"
     << std::setfill('0') << std::setw(2) << value.second;
  return ss.str();
}

std::string GetBinaryString(CefRefPtr<CefBinaryValue> value)
{
  if (!value.get())
    return "&nbsp;";

  // Retrieve the value.
  const size_t size = value->GetSize();
  std::string src;
  src.resize(size);
  value->GetData(const_cast<char*>(src.data()), size, 0);

  // Encode the value.
  return CefBase64Encode(src.data(), src.size());
}

// Return HTML string with information about a certificate.
std::string CWebBrowserClient::GetCertificateInformation(CefRefPtr<CefX509Certificate> cert, cef_cert_status_t certstatus)
{
  CefRefPtr<CefX509CertPrincipal> subject = cert->GetSubject();
  CefRefPtr<CefX509CertPrincipal> issuer = cert->GetIssuer();

  // Build a table showing certificate information. Various types of invalid
  // certificates can be tested using https://badssl.com/.
  std::stringstream ss;
  ss << "<h3>X.509 Certificate Information:</h3>"
        "<table border=1><tr><th>Field</th><th>Value</th></tr>";

  if (certstatus != CERT_STATUS_NONE)
  {
    ss << "<tr><td>Status</td><td>" << GetCertStatusString(certstatus) + "<br/>"
       << "</td></tr>";
  }

  ss << "<tr><td>Subject</td><td>"
     << (subject.get() ? subject->GetDisplayName().ToString() : "&nbsp;")
     << "</td></tr>"
        "<tr><td>Issuer</td><td>"
     << (issuer.get() ? issuer->GetDisplayName().ToString() : "&nbsp;")
     << "</td></tr>"
        "<tr><td>Serial #*</td><td>"
     << GetBinaryString(cert->GetSerialNumber()) << "</td></tr>"
     << "<tr><td>Valid Start</td><td>" << GetTimeString(cert->GetValidStart())
     << "</td></tr>"
        "<tr><td>Valid Expiry</td><td>"
     << GetTimeString(cert->GetValidExpiry()) << "</td></tr>";

  CefX509Certificate::IssuerChainBinaryList der_chain_list;
  CefX509Certificate::IssuerChainBinaryList pem_chain_list;
  cert->GetDEREncodedIssuerChain(der_chain_list);
  cert->GetPEMEncodedIssuerChain(pem_chain_list);
  DCHECK_EQ(der_chain_list.size(), pem_chain_list.size());

  der_chain_list.insert(der_chain_list.begin(), cert->GetDEREncoded());
  pem_chain_list.insert(pem_chain_list.begin(), cert->GetPEMEncoded());

  for (size_t i = 0U; i < der_chain_list.size(); ++i)
  {
    ss << "<tr><td>DER Encoded*</td>"
          "<td style=\"max-width:800px;overflow:scroll;\">"
       << GetBinaryString(der_chain_list[i])
       << "</td></tr>"
          "<tr><td>PEM Encoded*</td>"
          "<td style=\"max-width:800px;overflow:scroll;\">"
       << GetBinaryString(pem_chain_list[i]) << "</td></tr>";
  }

  ss << "</table> * Displayed value is base64 encoded.";
  return ss.str();
}

void CWebBrowserClient::CreateMessageHandlers(MessageHandlerSet& handlers)
{
  handlers.insert(new CJSHandler(this));
}

void CWebBrowserClient::SendMessage(Message& message, bool wait)
{
  if (!m_renderViewReady)
    return;

  std::shared_ptr<CEvent> waitEvent;
  if (wait)
  {
    message.waitEvent.reset(new CEvent(true));
    waitEvent = message.waitEvent;
  }

  CLockObject lock(m_Mutex);

  Message* msg    = new Message();
  msg->dwMessage  = message.dwMessage;
  msg->param1     = message.param1;
  msg->param2     = message.param2;
  msg->param3     = message.param3;
  msg->lpVoid     = message.lpVoid;
  msg->strParam   = message.strParam;
  msg->params     = message.params;
  msg->waitEvent  = message.waitEvent;

  m_processQueue.push(msg);
  lock.Unlock();

  if (waitEvent)
    waitEvent->Wait(1000);
}

void CWebBrowserClient::HandleMessages()
{
  // process threadmessages
  CLockObject lock(m_Mutex);
  while (!m_processQueue.empty())
  {
    Message* pMsg = m_processQueue.front();
    m_processQueue.pop();

    std::shared_ptr<CEvent> waitEvent = pMsg->waitEvent;
    lock.Unlock(); // <- see the large comment in SendMessage ^
    switch (pMsg->dwMessage)
    {
      case TMSG_SET_CONTROL_READY:
      {
        LOG_INTERNAL_MESSAGE(ADDON_LOG_DEBUG, "Web control %s", pMsg->param1 ? "ready" : "failed");
        SetControlReady(pMsg->param1);
        break;
      }
      case TMSG_SET_OPENED_ADDRESS:
      {
        LOG_INTERNAL_MESSAGE(ADDON_LOG_DEBUG, "Opened web site url '%s'", pMsg->strParam.c_str());
        SetOpenedAddress(pMsg->strParam);
        break;
      }
      case TMSG_SET_OPENED_TITLE:
      {
        LOG_INTERNAL_MESSAGE(ADDON_LOG_DEBUG, "Opened web site title '%s'", pMsg->strParam.c_str());
        SetOpenedTitle(pMsg->strParam);
        break;
      }
      case TMSG_SET_ICON_URL:
      {
        LOG_INTERNAL_MESSAGE(ADDON_LOG_DEBUG, "Opened web site set icon url '%s'", pMsg->strParam.c_str());
        SetIconURL(pMsg->strParam);
        break;
      }
      case TMSG_FULLSCREEN_MODE_CHANGE:
      {
        bool fullscreen = pMsg->param1 != 0;
        LOG_INTERNAL_MESSAGE(ADDON_LOG_DEBUG, "From currently opened web site becomes fullsreen requested as '%s'", fullscreen ? "yes" : "no");
        SetFullscreen(fullscreen);
        break;
      }
      case TMSG_SET_LOADING_STATE:
      {
        SetLoadingState(pMsg->param1, pMsg->param2, pMsg->param3);
        break;
      }
      case TMSG_SET_TOOLTIP:
      {
        SetTooltip(pMsg->strParam);
        break;
      }
      case TMSG_SET_STATUS_MESSAGE:
      {
        SetStatusMessage(pMsg->strParam);
        break;
      }
      default:
        break;
    };

    if (waitEvent)
      waitEvent->Signal();
    delete pMsg;

    lock.Lock();
  }
}