#include "dispatchers.hpp"

static const std::string overviewWorksapceName = "OVERVIEW";
static std::string workspaceNameBackup;
static int workspaceIdBackup;

void recalculateAllMonitor() {
  for (auto &m : g_pCompositor->m_vMonitors) {
    CMonitor *pMonitor = m.get();
    g_pLayoutManager->getCurrentLayout()->recalculateMonitor(pMonitor->ID);
  }
}

// only change layout,keep data of previous layout
void switchToLayoutWithoutReleaseData(std::string layout) {
  for (size_t i = 0; i < g_pLayoutManager->m_vLayouts.size(); ++i) {
    if (g_pLayoutManager->m_vLayouts[i].first == layout) {
      if (i == (size_t)g_pLayoutManager->m_iCurrentLayoutID)
        return;
      g_pLayoutManager->m_iCurrentLayoutID = i;
      return;
    }
  }
  hycov_log(ERR, "Unknown layout!");
}

bool isDirectionArg(std::string arg) {
  if (arg == "l" || arg == "r" || arg == "u" || arg == "d" || arg == "left" ||
      arg == "right" || arg == "up" || arg == "down" || arg == "leftcross" ||
      arg == "rightcross" || arg == "upcross" || arg == "downcross") {
    return true;
  } else {
    return false;
  }
}

bool isCrossMonitor(std::string arg) {
  if (arg == "leftcross" || arg == "rightcross" || arg == "upcross" ||
      arg == "downcross") {
    return true;
  } else {
    return false;
  }
}

std::optional<ShiftDirection> parseShiftArg(std::string arg) {
  if (arg == "l" || arg == "left" || arg == "leftcross")
    return ShiftDirection::Left;
  else if (arg == "r" || arg == "right" || arg == "rightcross")
    return ShiftDirection::Right;
  else if (arg == "u" || arg == "up" || arg == "upcross")
    return ShiftDirection::Up;
  else if (arg == "d" || arg == "down" || arg == "downcross")
    return ShiftDirection::Down;
  else
    return {};
}

CWindow *direction_select(std::string arg) {
  CWindow *pTempClient = g_pCompositor->m_pLastWindow;
  auto dataSize = g_pCompositor->m_vWindows.size();
  auto pTempCWindows = new CWindow *[dataSize + 1];
  CWindow *pTempFocusCWindows = nullptr;
  int last = -1;
  if (!pTempClient) {
    delete[] pTempCWindows;
    return nullptr;
  }

  if (!isDirectionArg(arg)) {
    hycov_log(
        ERR,
        "Cannot move focus in direction {}, unsupported direction. Supported: "
        "l/left/leftcross,r/right/rightcross,u/up/upcross,d/down/downcross",
        arg);
    delete[] pTempCWindows;
    return nullptr;
  }

  for (auto &w : g_pCompositor->m_vWindows) {
    CWindow *pWindow = w.get();

    if (pTempClient == pWindow || pWindow->isHidden() ||
        !pWindow->m_bIsMapped || pWindow->m_bFadingOut) {
      continue;
    }

    auto *pMonitor = g_pCompositor->getMonitorFromID(pWindow->m_iMonitorID);

    if (!((isCrossMonitor(arg) &&
           pWindow->m_iMonitorID != pTempClient->m_iMonitorID &&
           !g_pCompositor->isWorkspaceSpecial(pTempClient->m_iWorkspaceID) &&
           pWindow->m_iWorkspaceID == pMonitor->activeWorkspace) ||
          pTempClient->m_iWorkspaceID == pWindow->m_iWorkspaceID)) {
      continue;
    }

    last++;
    pTempCWindows[last] = pWindow;
  }

  if (last < 0) {
    delete[] pTempCWindows;
    return nullptr;
  }
  int sel_x = pTempClient->m_vRealPosition.goalv().x;
  int sel_y = pTempClient->m_vRealPosition.goalv().y;
  long long int distance = LLONG_MAX;
  ;
  // int temp_focus = 0;

  auto values = CVarList(arg);
  auto shift = parseShiftArg(values[0]);
  switch (shift.value()) {
  case ShiftDirection::Up:
    // Find the window with the closest coordinates
    // in the top left corner of the window (is limited to same x)
    for (int _i = 0; _i <= last; _i++) {
      if (pTempCWindows[_i]->m_vRealPosition.goalv().y < sel_y &&
          pTempCWindows[_i]->m_vRealPosition.goalv().x == sel_x) {
        int dis_x = pTempCWindows[_i]->m_vRealPosition.goalv().x - sel_x;
        int dis_y = pTempCWindows[_i]->m_vRealPosition.goalv().y - sel_y;
        long long int tmp_distance = dis_x * dis_x + dis_y * dis_y;
        if (tmp_distance < distance) {
          distance = tmp_distance;
          pTempFocusCWindows = pTempCWindows[_i];
        }
      }
    }
    // if find nothing above
    // find again(is unlimited to x)
    if (!pTempFocusCWindows) {
      for (int _i = 0; _i <= last; _i++) {
        if (pTempCWindows[_i]->m_vRealPosition.goalv().y < sel_y) {
          int dis_x = pTempCWindows[_i]->m_vRealPosition.goalv().x - sel_x;
          int dis_y = pTempCWindows[_i]->m_vRealPosition.goalv().y - sel_y;
          long long int tmp_distance = dis_x * dis_x + dis_y * dis_y;
          if (tmp_distance < distance) {
            distance = tmp_distance;
            pTempFocusCWindows = pTempCWindows[_i];
          }
        }
      }
    }
    break;
  case ShiftDirection::Down:
    for (int _i = 0; _i <= last; _i++) {
      if (pTempCWindows[_i]->m_vRealPosition.goalv().y > sel_y &&
          pTempCWindows[_i]->m_vRealPosition.goalv().x == sel_x) {
        int dis_x = pTempCWindows[_i]->m_vRealPosition.goalv().x - sel_x;
        int dis_y = pTempCWindows[_i]->m_vRealPosition.goalv().y - sel_y;
        long long int tmp_distance = dis_x * dis_x + dis_y * dis_y;
        if (tmp_distance < distance) {
          distance = tmp_distance;
          pTempFocusCWindows = pTempCWindows[_i];
        }
      }
    }
    if (!pTempFocusCWindows) {
      for (int _i = 0; _i <= last; _i++) {
        if (pTempCWindows[_i]->m_vRealPosition.goalv().y > sel_y) {
          int dis_x = pTempCWindows[_i]->m_vRealPosition.goalv().x - sel_x;
          int dis_y = pTempCWindows[_i]->m_vRealPosition.goalv().y - sel_y;
          long long int tmp_distance = dis_x * dis_x + dis_y * dis_y;
          if (tmp_distance < distance) {
            distance = tmp_distance;
            pTempFocusCWindows = pTempCWindows[_i];
          }
        }
      }
    }
    break;
  case ShiftDirection::Left:
    for (int _i = 0; _i <= last; _i++) {
      if (pTempCWindows[_i]->m_vRealPosition.goalv().x < sel_x &&
          pTempCWindows[_i]->m_vRealPosition.goalv().y == sel_y) {
        int dis_x = pTempCWindows[_i]->m_vRealPosition.goalv().x - sel_x;
        int dis_y = pTempCWindows[_i]->m_vRealPosition.goalv().y - sel_y;
        long long int tmp_distance = dis_x * dis_x + dis_y * dis_y;
        if (tmp_distance < distance) {
          distance = tmp_distance;
          pTempFocusCWindows = pTempCWindows[_i];
        }
      }
    }
    if (!pTempFocusCWindows) {
      for (int _i = 0; _i <= last; _i++) {
        if (pTempCWindows[_i]->m_vRealPosition.goalv().x < sel_x) {
          int dis_x = pTempCWindows[_i]->m_vRealPosition.goalv().x - sel_x;
          int dis_y = pTempCWindows[_i]->m_vRealPosition.goalv().y - sel_y;
          long long int tmp_distance = dis_x * dis_x + dis_y * dis_y;
          if (tmp_distance < distance) {
            distance = tmp_distance;
            pTempFocusCWindows = pTempCWindows[_i];
          }
        }
      }
    }
    break;
  case ShiftDirection::Right:
    for (int _i = 0; _i <= last; _i++) {
      if (pTempCWindows[_i]->m_vRealPosition.goalv().x > sel_x &&
          pTempCWindows[_i]->m_vRealPosition.goalv().y == sel_y) {
        int dis_x = pTempCWindows[_i]->m_vRealPosition.goalv().x - sel_x;
        int dis_y = pTempCWindows[_i]->m_vRealPosition.goalv().y - sel_y;
        long long int tmp_distance = dis_x * dis_x + dis_y * dis_y;
        if (tmp_distance < distance) {
          distance = tmp_distance;
          pTempFocusCWindows = pTempCWindows[_i];
        }
      }
    }
    if (!pTempFocusCWindows) {
      for (int _i = 0; _i <= last; _i++) {
        if (pTempCWindows[_i]->m_vRealPosition.goalv().x > sel_x) {
          int dis_x = pTempCWindows[_i]->m_vRealPosition.goalv().x - sel_x;
          int dis_y = pTempCWindows[_i]->m_vRealPosition.goalv().y - sel_y;
          long long int tmp_distance = dis_x * dis_x + dis_y * dis_y;
          if (tmp_distance < distance) {
            distance = tmp_distance;
            pTempFocusCWindows = pTempCWindows[_i];
          }
        }
      }
    }
    break;
  }
  delete[] pTempCWindows;
  return pTempFocusCWindows;
}

CWindow *get_circle_next_window(std::string arg) {
  bool next_ready = false;
  CWindow *pTempClient = g_pCompositor->m_pLastWindow;
  for (auto &w : g_pCompositor->m_vWindows) {
    CWindow *pWindow = w.get();
    if (pTempClient->m_iWorkspaceID != pWindow->m_iWorkspaceID ||
        pWindow->isHidden() || !pWindow->m_bIsMapped || pWindow->m_bFadingOut)
      continue;
    if (next_ready)
      return pWindow;
    if (pWindow == pTempClient)
      next_ready = true;
  }

  for (auto &w : g_pCompositor->m_vWindows) {
    CWindow *pWindow = w.get();
    if (pTempClient->m_iWorkspaceID != pWindow->m_iWorkspaceID ||
        pWindow->isHidden() || !pWindow->m_bIsMapped || pWindow->m_bFadingOut)
      continue;
    return pWindow;
  }
  return nullptr;
}

void warpcursor_and_focus_to_window(CWindow *pWindow) {
  g_pCompositor->focusWindow(pWindow);
  g_pCompositor->warpCursorTo(pWindow->middle());
  g_pInputManager->m_pForcedFocus = pWindow;
  g_pInputManager->simulateMouseMovement();
  g_pInputManager->m_pForcedFocus = nullptr;
}

void dispatch_circle(std::string arg) {
  CWindow *pWindow;
  pWindow = get_circle_next_window(arg);
  if (pWindow) {
    warpcursor_and_focus_to_window(pWindow);
  }
}

void dispatch_focusdir(std::string arg) {
  CWindow *pWindow;
  pWindow = direction_select(arg);
  if (pWindow) {
    warpcursor_and_focus_to_window(pWindow);
  }
}

void dispatch_toggleoverview(std::string arg) {
  if (g_hycov_isOverView &&
      (!g_hycov_enable_alt_release_exit || arg == "internalToggle")) {
    dispatch_leaveoverview("");
    hycov_log(LOG, "leave overview:toggleMethod:{},enable_alt_release_exit:{}",
              arg, g_hycov_enable_alt_release_exit);
  } else if (g_hycov_isOverView && g_hycov_enable_alt_release_exit &&
             arg != "internalToggle") {
    dispatch_circle("");
    hycov_log(LOG, "toggle overview:switch focus circlely");
  } else if (g_hycov_enable_alt_release_exit && g_hycov_alt_toggle_auto_next &&
             arg != "internalToggle") {
    dispatch_enteroverview(arg);
    dispatch_circle("");
    hycov_log(LOG, "enter overview:alt switch mode auto next");
  } else {
    dispatch_enteroverview(arg);
    hycov_log(LOG, "enter overview:toggleMethod:{}", arg);
  }
}

void dispatch_enteroverview(std::string arg) {
  if (g_hycov_isOverView) {
    return;
  }

  const auto pMonitor = g_pCompositor->m_pLastMonitor;
  if (pMonitor->specialWorkspaceID != 0)
    pMonitor->setSpecialWorkspace(nullptr);

  // force display all workspace window in one monitor,ignore
  // `only_active_worksapce` and `only_active_monitor`

  // TESTOWO ali clients exit fullscreen status before enter overview
  CWindow *pActiveWindow = g_pCompositor->m_pLastWindow;
  CWorkspace *pActiveWorkspace;
  CMonitor *pActiveMonitor;

  bool isNoShouldTileWindow = true;

  for (auto &w : g_pCompositor->m_vWindows) {
    CWindow *pWindow = w.get();
    if (pWindow->isHidden() || !pWindow->m_bIsMapped || pWindow->m_bFadingOut ||
        g_pCompositor->isWorkspaceSpecial(pWindow->m_iWorkspaceID))
      continue;
    isNoShouldTileWindow = false;
  }

  // if no clients, forbit enter overview
  if (isNoShouldTileWindow) {
    return;
  }

  hycov_log(LOG, "enter overview");
  g_hycov_isOverView = true;

  // enter overview layout
  //  g_pLayoutManager->switchToLayout("ovgrid");
  switchToLayoutWithoutReleaseData("ovgrid");
  g_pLayoutManager->getCurrentLayout()->onEnable();

  // change workspace name to OVERVIEW
  pActiveMonitor = g_pCompositor->m_pLastMonitor;
  pActiveWorkspace =
      g_pCompositor->getWorkspaceByID(pActiveMonitor->activeWorkspace);
  workspaceNameBackup = pActiveWorkspace->m_szName;
  workspaceIdBackup = pActiveWorkspace->m_iID;
  g_pCompositor->renameWorkspace(pActiveMonitor->activeWorkspace,
                                 overviewWorksapceName);

  // disable changeworkspace
  if (g_hycov_disable_workspace_change) {
    g_hycov_pChangeworkspaceHook->hook();
    g_hycov_pMoveActiveToWorkspaceHook->hook();
  }

  // disable spawn
  if (g_hycov_disable_spawn) {
    g_hycov_pSpawnHook->hook();
  }

  g_hycov_pCKeybindManager_changeGroupActiveHook->hook();
  g_hycov_pCKeybindManager_toggleGroupHook->hook();
  g_hycov_pCKeybindManager_moveOutOfGroupHook->hook();

  return;
}

void dispatch_leaveoverview(std::string arg) {
  if (!g_hycov_isOverView) {
    return;
  }

  const auto pMonitor = g_pCompositor->m_pLastMonitor;
  if (pMonitor->specialWorkspaceID != 0)
    pMonitor->setSpecialWorkspace(nullptr);

  // get default layout
  std::string *configLayoutName = &g_hycov_configLayoutName;

  hycov_log(LOG, "leave overview");
  g_hycov_isOverView = false;
  // mark exiting overview mode
  g_hycov_isOverViewExiting = true;

  // restore workspace name
  g_pCompositor->renameWorkspace(workspaceIdBackup, workspaceNameBackup);

  // enable changeworkspace
  if (g_hycov_disable_workspace_change) {
    g_hycov_pChangeworkspaceHook->unhook();
    g_hycov_pMoveActiveToWorkspaceHook->unhook();
  }

  // enable spawn
  if (g_hycov_disable_spawn) {
    g_hycov_pSpawnHook->unhook();
  }

  g_hycov_pCKeybindManager_changeGroupActiveHook->unhook();
  g_hycov_pCKeybindManager_toggleGroupHook->unhook();
  g_hycov_pCKeybindManager_moveOutOfGroupHook->unhook();

  // if no clients, just exit overview, don't restore client's state
  if (g_hycov_OvGridLayout->m_lOvGridNodesData.empty()) {
    switchToLayoutWithoutReleaseData(*configLayoutName);
    recalculateAllMonitor();
    g_hycov_OvGridLayout->m_lOvGridNodesData.clear();
    g_hycov_isOverViewExiting = false;
    return;
  }

  // move clients to it's original workspace
  g_hycov_OvGridLayout->moveWindowToSourceWorkspace();
  // go to the workspace where the active client was before
  g_hycov_OvGridLayout->changeToActivceSourceWorkspace();

  for (auto &n : g_hycov_OvGridLayout->m_lOvGridNodesData) {
    // make all window restore it's style
    n.pWindow->m_sSpecialRenderData.border = n.ovbk_windowIsWithBorder;
    n.pWindow->m_sSpecialRenderData.decorate = n.ovbk_windowIsWithDecorate;
    n.pWindow->m_sSpecialRenderData.rounding = n.ovbk_windowIsWithRounding;
    n.pWindow->m_sSpecialRenderData.shadow = n.ovbk_windowIsWithShadow;
  }

  // exit overview layout,go back to old layout
  CWindow *pActiveWindow = g_pCompositor->m_pLastWindow;
  g_pCompositor->focusWindow(nullptr);
  // g_pLayoutManager->switchToLayout(*configLayoutName);
  // g_pLayoutManager->getCurrentLayout()->onDisable();
  switchToLayoutWithoutReleaseData(*configLayoutName);
  recalculateAllMonitor();

  for (auto &n : g_hycov_OvGridLayout->m_lOvGridNodesData) {
    // if client not in old layout,create tiling of the client
    if (!n.isInOldLayout) {
      if (n.pWindow->m_bFadingOut || !n.pWindow->m_bIsMapped ||
          n.pWindow->isHidden()) {
        continue;
      }
      hycov_log(LOG,
                "create tiling window in old "
                "layout,window:{},workspace:{},inoldlayout:{}",
                n.pWindow, n.workspaceID, n.isInOldLayout);
      g_pLayoutManager->getCurrentLayout()->onWindowCreatedTiling(n.pWindow);
    }
    // restore active window in group
    if (n.isGroupActive) {
      n.pWindow->setGroupCurrent(n.pWindow);
    }
  }

  // clean overview layout node date
  g_hycov_OvGridLayout->m_lOvGridNodesData.clear();

  // mark has exited overview mode
  g_hycov_isOverViewExiting = false;
  return;
}

void registerDispatchers() {
  HyprlandAPI::addDispatcher(PHANDLE, "hycov:enteroverview",
                             dispatch_enteroverview);
  HyprlandAPI::addDispatcher(PHANDLE, "hycov:leaveoverview",
                             dispatch_leaveoverview);
  HyprlandAPI::addDispatcher(PHANDLE, "hycov:toggleoverview",
                             dispatch_toggleoverview);
  HyprlandAPI::addDispatcher(PHANDLE, "hycov:movefocus", dispatch_focusdir);
}
