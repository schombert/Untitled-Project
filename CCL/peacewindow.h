#pragma once
#include "globalhelpers.h"

void SetupPeaceWindow(war_id_t war_for, admin_id_t offer_from, bool is_demand) noexcept;
void InitPeaceWindow() noexcept;
void inline_update_peace_window() noexcept;
bool peace_window_open() noexcept;