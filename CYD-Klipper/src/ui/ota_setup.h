#pragma once

String OtaNewVersionName();
bool OtaHasUpdate();
void OtaDoUpdate(bool variantAutomatic = false);
void OtaInit();
void SetReadyForOtaUpdate();
bool IsReadyForOtaUpdate();
