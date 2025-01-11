#pragma once

void semaphore_init();

void freeze_request_thread();
void unfreeze_request_thread();

// Don't use unless you want trouble
void freeze_render_thread();
// Don't use unless you want trouble
void unfreeze_render_thread();