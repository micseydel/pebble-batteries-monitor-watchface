/*

TODO

Charging indicators
Colors
Bluetooth indicator - https://developer.pebble.com/tutorials/intermediate/add-bluetooth/
Configurable intervals - https://developer.pebble.com/tutorials/intermediate/slate/
Detect staleness

*/

#include <pebble.h>

// Largest expected inbox and outbox message sizes
const uint32_t inbox_size = 64;
const uint32_t outbox_size = sizeof(int);


static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_watch_battery_layer;
static TextLayer *s_phone_battery_layer;

static int s_watch_battery_level;
static int32_t s_phone_battery_level = 0;

static BitmapLayer *s_background_layer, *s_bt_icon_layer;
static GBitmap *s_background_bitmap, *s_bt_icon_bitmap;

static GFont s_time_font;

/**
 * num is expected be in [0, 100] and string will be ? for 0
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

static void battery_callback(BatteryChargeState state) {
  // Record the new battery level
  s_watch_battery_level = state.charge_percent;
//   layer_mark_dirty(s_watch_battery_layer);
}

static void update_date() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[12];
  strftime(s_buffer, sizeof(s_buffer), "%d %B", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_date_layer, s_buffer);
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[9];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
}

static void update_watch_battery_layer() {
  static char s_buffer[4] = " ? ";
  battery_level_to_string(s_watch_battery_level, s_buffer);

  // Display this on the TextLayer
  text_layer_set_text(s_watch_battery_layer, s_buffer);
}

static void update_phone_battery_layer() {
  static char s_buffer[4] = " ? ";
  battery_level_to_string(s_phone_battery_level, s_buffer);

  // Display this on the TextLayer
  text_layer_set_text(s_phone_battery_layer, s_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  update_date();
  update_watch_battery_layer();
  update_phone_battery_layer();
}

static void bluetooth_callback(bool connected) {
  // Show icon if disconnected
  layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer), connected);
  layer_set_hidden(text_layer_get_layer(s_phone_battery_layer), !connected);
  if (connected) {
    s_phone_battery_level = 0;
  }

  if(!connected) {
    // Issue a vibrating alert
    vibes_double_pulse();
  }
}

static void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create GBitmap
  s_background_bitmap = gbitmap_create_with_resource(
    //RESOURCE_ID_IMAGE_BACKGROUND
    RESOURCE_ID_BACKGROUND_IMAGE
  );

  // Create BitmapLayer to display the GBitmap
  s_background_layer = bitmap_layer_create(bounds);

  // Set the bitmap onto the layer and add to the window
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));

  // Create the TextLayer with specific bounds
  s_time_layer = text_layer_create(
      GRect(0, 126, bounds.size.w, 50));
  s_date_layer = text_layer_create(
      GRect(0, 0, bounds.size.w, 50));
  s_watch_battery_layer = text_layer_create(
      GRect(25, 70, bounds.size.w, 50));
  s_phone_battery_layer = text_layer_create(
      GRect(100, 70, bounds.size.w, 50));

  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_background_color(s_watch_battery_layer, GColorClear);
  text_layer_set_background_color(s_phone_battery_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_text_color(s_watch_battery_layer, GColorWhite);
  text_layer_set_text_color(s_phone_battery_layer, GColorWhite);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_text(s_date_layer, "1 January");
  text_layer_set_text(s_watch_battery_layer, "?");
  text_layer_set_text(s_phone_battery_layer, "?");
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  text_layer_set_text_alignment(s_watch_battery_layer, GTextAlignmentLeft);
  text_layer_set_text_alignment(s_phone_battery_layer, GTextAlignmentLeft);

  // Create GFont
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PERFECT_DOS_48));

  // Apply to TextLayer
//   text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_font(s_watch_battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_font(s_phone_battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  
  // Create the Bluetooth icon GBitmap
  s_bt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BLUETOOTH_DISABLED);
  
  // Create the BitmapLayer to display the GBitmap
  s_bt_icon_layer = bitmap_layer_create(GRect(95, 70, 30, 30));
  bitmap_layer_set_bitmap(s_bt_icon_layer, s_bt_icon_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bt_icon_layer));

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_watch_battery_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_phone_battery_layer));
  
  // Show the correct state of the BT connection from the start
  bluetooth_callback(connection_service_peek_pebble_app_connection());
}

static void main_window_unload(Window *window) {
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  text_layer_destroy(s_watch_battery_layer);
  text_layer_destroy(s_phone_battery_layer);

  // Unload GFont
  fonts_unload_custom_font(s_time_font);

  // Destroy GBitmap
  gbitmap_destroy(s_background_bitmap);

  // Destroy BitmapLayers
  bitmap_layer_destroy(s_background_layer);
  gbitmap_destroy(s_bt_icon_bitmap);
  bitmap_layer_destroy(s_bt_icon_layer);
}


static void inbox_received_callback(DictionaryIterator *iter, void *context) {
  // A new message has been successfully received
  Tuple *battery_tuple = dict_find(iter, MESSAGE_KEY_phone_battery);
  if (battery_tuple) {
    bool do_update = s_phone_battery_level == 0;

    s_phone_battery_level = battery_tuple->value->int32;
    
    if (do_update) {
      update_phone_battery_layer();
    }
  }
}

//TODO: inbox dropped https://developer.pebble.com/guides/communication/sending-and-receiving-data/

static void request_from_phone() {
  // Declare the dictionary's iterator
  DictionaryIterator *out_iter;

  // Prepare the outbox buffer for this message
  AppMessageResult result = app_message_outbox_begin(&out_iter);

  if(result == APP_MSG_OK) {
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
  if(result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error sending the outbox: %d", (int)result);
  }
}

static void init() {
  // Register for battery level updates
  battery_state_service_subscribe(battery_callback);

  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set the background color
  window_set_background_color(s_main_window, GColorBlack);

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);

  // Make sure the time is displayed from the start
  update_time();

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  battery_callback(battery_state_service_peek());
  
  // Open AppMessage
  app_message_open(inbox_size, outbox_size);

  // Register to be notified about inbox received events
  app_message_register_inbox_received(inbox_received_callback);
  
  request_from_phone();

  // Register for Bluetooth connection updates
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bluetooth_callback
  });
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}