#pragma once

void load_batteries(Window *window, Layer *window_layer, GRect bounds);

void unload_batteries();

void battery_callback(BatteryChargeState state);

void update_watch_battery_layer();

void update_phone_battery_layer();

void request_from_phone();

void bluetooth_callback(bool connected);

void inbox_received_callback(DictionaryIterator *iter, void *context);
