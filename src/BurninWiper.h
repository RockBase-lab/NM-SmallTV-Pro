#pragma once
#include <Arduino.h>
#include "Settings.h"

void     burninInit();
void     burninService(const Settings& s);
bool     burninActive();
void     burninDismiss();
void     burninTriggerPreview(const char* effect, uint16_t durationSec);
uint32_t burninHeldMs();
