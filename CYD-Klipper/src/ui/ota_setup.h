#pragma once

String ota_new_version_name();
bool ota_has_update();
void ota_do_update(bool variant_automatic = false);
void ota_init();
void set_ready_for_ota_update();
bool is_ready_for_ota_update();