#include <pebble.h>
#include "batteries/batteries.h"

static TextLayer *s_watch_battery_layer;
static TextLayer *s_phone_battery_layer;

static int s_watch_battery_level;
static int32_t s_phone_battery_level = 0;

static BitmapLayer *s_bt_icon_layer;
static GBitmap *s_bt_icon_bitmap;

void load_batteries(Window *window, Layer *window_layer, GRect bounds) {
  s_watch_battery_layer = text_layer_create(
      GRect(25, 70, bounds.size.w, 50));
  s_phone_battery_layer = text_layer_create(
      GRect(100, 70, bounds.size.w, 50));
  
  text_layer_set_background_color(s_watch_battery_layer, GColorClear);
  text_layer_set_background_color(s_phone_battery_layer, GColorClear);
  text_layer_set_text_color(s_watch_battery_layer, GColorWhite);
  text_layer_set_text_color(s_phone_battery_layer, GColorWhite);
  text_layer_set_text(s_watch_battery_layer, " ?");
  text_layer_set_text(s_phone_battery_layer, " ?");
  text_layer_set_text_alignment(s_watch_battery_layer, GTextAlignmentLeft);
  text_layer_set_text_alignment(s_phone_battery_layer, GTextAlignmentLeft);

  text_layer_set_font(s_watch_battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_font(s_phone_battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));

  // Create the Bluetooth icon GBitmap
  s_bt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BLUETOOTH_DISABLED);
  
  // Create the BitmapLayer to display the GBitmap
  s_bt_icon_layer = bitmap_layer_create(GRect(95, 70, 30, 30));
  bitmap_layer_set_bitmap(s_bt_icon_layer, s_bt_icon_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bt_icon_layer));

  layer_add_child(window_layer, text_layer_get_layer(s_watch_battery_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_phone_battery_layer));
  
  // Show the correct state of the BT connection from the start
  bluetooth_callback(connection_service_peek_pebble_app_connection());
}

void unload_batteries() {
  // Destroy TextLayer
  text_layer_destroy(s_watch_battery_layer);
  text_layer_destroy(s_phone_battery_layer);

  // Destroy BitmapLayers
  gbitmap_destroy(s_bt_icon_bitmap);
  bitmap_layer_destroy(s_bt_icon_layer);
}

/**
 * num is expected be in [0, 100] and string will be ? for 0
 *
 * Result will be stored in buffer, which is expected to be precisely size 4.
 */
static void battery_level_to_string(int level, char* buffer) {
  if (level == 0) {
    buffer[0] = ' ';
    buffer[1] = '?';
    buffer[2] = ' ';
    return;
  }
  buffer[2] = '0' + level % 10;
  level /= 10;
  buffer[1] = '0' + level % 10;
  level /= 10;
  buffer[0] = level ? '1' : ' ';
}

void battery_callback(BatteryChargeState state) {
  // Record the new battery level
  s_watch_battery_level = state.charge_percent;
  // layer_mark_dirty(s_watch_battery_layer); // TODO: investigate why this is commented out; remove or uncomment
}

void update_watch_battery_layer() {
  static char s_buffer[4] = " ? ";
  battery_level_to_string(s_watch_battery_level, s_buffer);

  // Display this on the TextLayer
  text_layer_set_text(s_watch_battery_layer, s_buffer);
}

void update_phone_battery_layer() {
  static char s_buffer[4] = " ? ";
  battery_level_to_string(s_phone_battery_level, s_buffer);

  // Display this on the TextLayer
  text_layer_set_text(s_phone_battery_layer, s_buffer);
}

void request_from_phone() {
  // Declare the dictionary's iterator
  DictionaryIterator *out_iter;

  // Prepare the outbox buffer for this message
  AppMessageResult result = app_message_outbox_begin(&out_iter);

  if (result == APP_MSG_OK) {
    // A dummy value
    int value = 0;
  
    // Add an item to ask for weather data
    dict_write_int(out_iter, MESSAGE_KEY_phone_battery, &value, sizeof(int), true);
  } else {
    // The outbox cannot be used right now
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error preparing the outbox: %d", (int)result);
  }

  // Send this message
  result = app_message_outbox_send();
  
  // Check the result
  if (result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error sending the outbox: %d", (int)result);
  }
}

void bluetooth_callback(bool connected) {
  // Show Bluetooth icon if disconnected, phone battery if connected
  layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer), connected);
  layer_set_hidden(text_layer_get_layer(s_phone_battery_layer), !connected);

  if (connected) {
    // If we just connected, zero out the phone battery to indicate we don't know what it is
    s_phone_battery_level = 0;
  } else {
    // Otherwise, issue a vibrating alert to indicate we just disconnected
    vibes_double_pulse();
  }
}

// TODO: inbox dropped https://developer.pebble.com/guides/communication/sending-and-receiving-data/
void inbox_received_callback(DictionaryIterator *iter, void *context) {
  // A new message has been successfully received
  Tuple *battery_tuple = dict_find(iter, MESSAGE_KEY_phone_battery);
  if (battery_tuple) {
    bool do_update = s_phone_battery_level == 0;

    s_phone_battery_level = battery_tuple->value->int32;
    
    // If the battery level was 0, update right away, otherwise the tick handler can do it later
    if (do_update) {
      update_phone_battery_layer();
    }
  }
}
