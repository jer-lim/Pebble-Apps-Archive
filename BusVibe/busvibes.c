#include <pebble.h>
#include <stdlib.h>

static Window *window;
static TextLayer *text_layer;
static TextLayer *bus_stop_layer;
static TextLayer *bus_number_layer;

static const uint32_t const vibratePattern[]={150,250,150,250,150,250,150,250,150,250,150,250,150,250,150,250,150,250,150,250,150,250,150,250,150,250,150,250,150,250,150,250,150,250,150,250,150,250,150,250,150,250,150,250,150,250,150,250,150,250,150,250,150,250,150,250,150,250,150,250,150,250,150,250,150,250,150,250,150,250,150,250,150,250};

static int busNumber=0;
static int tempBusNumber=0;
static int digit=10;
char str[128];
char str2[5];
char str3[128];
char str4[10];
char str5[50];
char str6[5];
char busStopNum[6];
int vibrateNumber=0;
int currentBusStop=0;
bool haveBusStop=false;
bool busTimeSubmittable=false;
bool requestedResend=false;

ActionBarLayer *action_bar;

void busStop_out_sent_handler(DictionaryIterator *sent,void *context);
void busStop_out_failed_handler(DictionaryIterator *failed,AppMessageResult reason,void *context);

void busStopCycle_out_sent_handler(DictionaryIterator *sent,void *context);
void busStopCycle_out_failed_handler(DictionaryIterator *failed,AppMessageResult reason,void *context);

void busTime_out_sent_handler(DictionaryIterator *sent,void *context);
void busTime_out_failed_handler(DictionaryIterator *failed,AppMessageResult reason,void *context);

void out_sent_handler(DictionaryIterator *sent,void *context);
void out_failed_handler(DictionaryIterator *failed,AppMessageResult reason,void *context);

void in_received_handler(DictionaryIterator *received, void *context);
void in_dropped_handler(AppMessageResult reason, void *context);

void vibrateXTimes(int num);

void resetBusNumber();
void updateBusNumber();
void lockNumber();
void submitBusTime();

void getCurrentBusStop();
void getBusTiming();

void cycleUpBusStop();
void cycleDownBusStop();
void requestBusStop();

static void click_config_provider(void *context);
static void select_click_handler(ClickRecognizerRef recognizer, void *context);
static void up_click_handler(ClickRecognizerRef recognizer, void *context);
static void down_click_handler(ClickRecognizerRef recognizer, void *context);

static void number_click_config_provider(void *context);
static void number_select_click_handler(ClickRecognizerRef recognizer, void *context);
static void number_up_click_handler(ClickRecognizerRef recognizer, void *context);
static void number_down_click_handler(ClickRecognizerRef recognizer, void *context);
static void number_select_down_long_click_handler(ClickRecognizerRef recogniser, void *context);
static void number_select_up_long_click_handler(ClickRecognizerRef recogniser, void *context);
static void number_up_down_long_click_handler(ClickRecognizerRef recogniser, void *context);
static void number_down_down_long_click_handler(ClickRecognizerRef recogniser, void *context);

static void resubmit_click_config_provider(void *context);
static void resubmit_select_down_long_click_handler(ClickRecognizerRef recogniser, void *context);
static void resubmit_select_up_long_click_handler(ClickRecognizerRef recogniser, void *context);

static void window_load(Window *window);
static void window_unload(Window *window);
static void init(void);
static void deinit(void);
int main(void);

//BUS STOP OUT HANDLER
void busStop_out_sent_handler(DictionaryIterator *sent,void *context){
	APP_LOG(APP_LOG_LEVEL_DEBUG,"[PEBBLE] Message delivered - get bus stop");
}

//BUS STOP OUT HANDLER FAIL REPEATER
void busStop_out_failed_handler(DictionaryIterator *failed,AppMessageResult reason,void *context){
	APP_LOG(APP_LOG_LEVEL_DEBUG, "[PEBBLE] Message delivery failed on getting bus stop. Retrying...");
	getCurrentBusStop();
}

//BUS STOP OUT HANDLER
void busStopCycle_out_sent_handler(DictionaryIterator *sent,void *context){
	APP_LOG(APP_LOG_LEVEL_DEBUG,"[PEBBLE] Message delivered - cycle bus stop");
}

//BUS STOP OUT HANDLER FAIL REPEATER
void busStopCycle_out_failed_handler(DictionaryIterator *failed,AppMessageResult reason,void *context){
	APP_LOG(APP_LOG_LEVEL_DEBUG, "[PEBBLE] Message delivery failed on cycling bus stop. Retrying...");
	requestBusStop();
}

//BUS TIME OUT HANDLER
void busTime_out_sent_handler(DictionaryIterator *sent,void *context){
	APP_LOG(APP_LOG_LEVEL_DEBUG,"[PEBBLE] Message delivered - get bus stop");
}

//BUS TIME OUT HANDLER FAIL REPEATER
void busTime_out_failed_handler(DictionaryIterator *failed,AppMessageResult reason,void *context){
	APP_LOG(APP_LOG_LEVEL_DEBUG, "[PEBBLE] Message delivery failed on getting bus time. Retrying...");
	getBusTiming();
}

void out_sent_handler(DictionaryIterator *sent,void *context){
	APP_LOG(APP_LOG_LEVEL_DEBUG,"[PEBBLE] Message delivered");
}

void out_failed_handler(DictionaryIterator *failed,AppMessageResult reason,void *context){
	APP_LOG(APP_LOG_LEVEL_DEBUG, "[PEBLbE] Message delivery failed");
}

void in_received_handler(DictionaryIterator *received, void *context){
	Tuple *busStop_tuple=dict_find(received,2);
	Tuple *busTime_tuple=dict_find(received,5);
	
	if(busStop_tuple){
		APP_LOG(APP_LOG_LEVEL_DEBUG,"[pPEBlb] From JS: receive_busStop: %s",busStop_tuple->value->cstring);
		strcpy(str,"Current Bus Stop:\n");
  		strcat(str,busStop_tuple->value->cstring);
		text_layer_set_text(bus_stop_layer, str);
		haveBusStop=true;
		busTimeSubmittable=true;
	}
	
	if(busTime_tuple){
		APP_LOG(APP_LOG_LEVEL_DEBUG,"[pPEBlb] From JS: receive_busTime: %s",busTime_tuple->value->cstring);
		strcpy(str4,busTime_tuple->value->cstring);
		if(strcmp(str4,"-1")!=0){
			vibrateNumber=atoi(str4);
			APP_LOG(APP_LOG_LEVEL_DEBUG,"[pPEBlb] From JS: Vibrating x times: %d",vibrateNumber);
			strcat(str4," min");
			text_layer_set_text(bus_number_layer, str4);

			snprintf(str6,sizeof(str6),"%02d",busNumber);
			strcpy(str5,"Next bus ");
			strcat(str5,str6);
			strcat(str5,":");
			text_layer_set_text(text_layer, str5);

			vibrateXTimes(vibrateNumber);
		}else{
			text_layer_set_text(bus_number_layer, "ERR");
		}
		window_set_click_config_provider(window, resubmit_click_config_provider);
	}
}

void in_dropped_handler(AppMessageResult reason, void *context){
	APP_LOG(APP_LOG_LEVEL_DEBUG,"[PEBBLEL] Incoming message dropped");
	DictionaryIterator *iter;
	if(requestedResend==false){
		requestedResend=true;
		app_message_outbox_begin(&iter);
		Tuplet value1=TupletInteger(6,1);
		dict_write_tuplet(iter,&value1);
		app_message_outbox_send();
		APP_LOG(APP_LOG_LEVEL_DEBUG,"[PEBBLEL] Requested JS resend last message");
	}
}

void vibrateXTimes(int num){
	VibePattern pat={
		.durations=vibratePattern,
		.num_segments=(num*2-1)
	};
	if(num>0) vibes_enqueue_custom_pattern(pat);
}


void resetBusNumber(){
	tempBusNumber=0;
}

void updateBusNumber(){
	int nnum=busNumber+tempBusNumber;
	snprintf(str2,sizeof(str2),"%d",nnum);
	text_layer_set_text(bus_number_layer, str2);
}

void lockNumber(){
	if(digit<1000){
		busNumber=busNumber+tempBusNumber;
		busNumber=busNumber*10;
		digit=digit*10;
		resetBusNumber();
		updateBusNumber();
		APP_LOG(APP_LOG_LEVEL_DEBUG,"[PEBBLEL] Locked number");
		vibrateXTimes(1);
	}else{
		submitBusTime();
	}
}

void submitBusTime(){
	if(busTimeSubmittable){
		busNumber=busNumber+tempBusNumber;
		vibrateXTimes(1);
		window_set_click_config_provider(window, click_config_provider);
		getBusTiming();
	}
}

//GET CURRENT BUS STOP
void getCurrentBusStop(){
	psleep(2000);
	app_message_register_outbox_sent(busStop_out_sent_handler);
	app_message_register_outbox_failed(busStop_out_failed_handler);
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	Tuplet value1=TupletInteger(1,1);
	dict_write_tuplet(iter,&value1);
	app_message_outbox_send();
	APP_LOG(APP_LOG_LEVEL_DEBUG, "%li", time(NULL));
	text_layer_set_text(bus_stop_layer, "Fetching\nBus Stop...");
}

void getBusTiming(){
	app_message_register_outbox_sent(busTime_out_sent_handler);
	app_message_register_outbox_failed(busTime_out_failed_handler);
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	Tuplet value1=TupletInteger(4,busNumber);
	dict_write_tuplet(iter,&value1);
	app_message_outbox_send();
	
	text_layer_set_text(text_layer, "Fetching bus time...");
}

void cycleUpBusStop(){
	if(currentBusStop>0 && haveBusStop){
		currentBusStop--;
		vibrateXTimes(1);
		requestBusStop();
	}
}

void cycleDownBusStop(){
	if(currentBusStop<9 && haveBusStop){
		currentBusStop++;
		vibrateXTimes(1);
		requestBusStop();
	}
}

void requestBusStop(){
	app_message_register_outbox_sent(busStopCycle_out_sent_handler);
	app_message_register_outbox_failed(busStopCycle_out_failed_handler);
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	Tuplet value1=TupletInteger(3,currentBusStop);
	dict_write_tuplet(iter,&value1);
	app_message_outbox_send();
	
	text_layer_set_text(bus_stop_layer, "Fetching\nBus Stop...");
	busTimeSubmittable=false;
}

/**
 * GENERIC BLANK CLICK HANDLER
 */
static void click_config_provider(void *context) {
	window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
	window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
	window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {}

/**
 * NUMBER SELECTION CLICK HANDLER
 */
static void number_click_config_provider(void *context) {
	window_single_click_subscribe(BUTTON_ID_SELECT, number_select_click_handler);
	window_single_click_subscribe(BUTTON_ID_UP, number_up_click_handler);
	window_single_click_subscribe(BUTTON_ID_DOWN, number_down_click_handler);
	window_long_click_subscribe(BUTTON_ID_SELECT, 1000, number_select_down_long_click_handler, number_select_up_long_click_handler);
	window_long_click_subscribe(BUTTON_ID_UP, 1000, number_up_down_long_click_handler, number_select_up_long_click_handler);
	window_long_click_subscribe(BUTTON_ID_DOWN, 1000, number_down_down_long_click_handler, number_select_up_long_click_handler);
}

static void number_select_click_handler(ClickRecognizerRef recognizer, void *context) {
	lockNumber();
}

static void number_up_click_handler(ClickRecognizerRef recognizer, void *context) {
	if(tempBusNumber<9) tempBusNumber++;
	else tempBusNumber=0;
	vibrateXTimes(tempBusNumber);
	updateBusNumber();
}

static void number_down_click_handler(ClickRecognizerRef recognizer, void *context) {
	if(tempBusNumber>0) tempBusNumber--;
	else tempBusNumber=9;
	vibrateXTimes(tempBusNumber);
	updateBusNumber();
}

static void number_select_down_long_click_handler(ClickRecognizerRef recogniser, void *context){
	submitBusTime();
}

static void number_select_up_long_click_handler(ClickRecognizerRef recogniser, void *context){}

static void number_up_down_long_click_handler(ClickRecognizerRef recogniser, void *context){
	cycleUpBusStop();
}

static void number_down_down_long_click_handler(ClickRecognizerRef recogniser, void *context){
	cycleDownBusStop();
}

/**
 * BUS NUMBER RESUBMIT CLICK HANDLER
 */
static void resubmit_click_config_provider(void *context) {
	window_long_click_subscribe(BUTTON_ID_SELECT, 1000, resubmit_select_down_long_click_handler, resubmit_select_up_long_click_handler);
}

static void resubmit_select_down_long_click_handler(ClickRecognizerRef recogniser, void *context){
	vibrateXTimes(1);
	getBusTiming();
}

static void resubmit_select_up_long_click_handler(ClickRecognizerRef recogniser, void *context){}

static void window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);
	
	text_layer = text_layer_create((GRect) { .origin = { 0, 50 }, .size = { bounds.size.w, 20 } });
	text_layer_set_text(text_layer, "Bus Number:");
	text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
	layer_add_child(window_layer, text_layer_get_layer(text_layer));
	
	bus_stop_layer = text_layer_create((GRect) { .origin = { 0, 10 }, .size = { bounds.size.w, 40 } });
	text_layer_set_text(bus_stop_layer, "Current Bus Stop:");
	text_layer_set_text_alignment(bus_stop_layer, GTextAlignmentCenter);
	layer_add_child(window_layer, text_layer_get_layer(bus_stop_layer));
	
	bus_number_layer = text_layer_create((GRect) { .origin = { 0, 70 }, .size = { bounds.size.w, 50 } });
	text_layer_set_text(bus_number_layer, "0");
	text_layer_set_text_alignment(bus_number_layer, GTextAlignmentCenter);
	text_layer_set_font(bus_number_layer,fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
	layer_add_child(window_layer, text_layer_get_layer(bus_number_layer));
	
	getCurrentBusStop();
	window_set_click_config_provider(window, number_click_config_provider);
	resetBusNumber();
}

static void window_unload(Window *window) {
	text_layer_destroy(text_layer);
	text_layer_destroy(bus_stop_layer);
	text_layer_destroy(bus_number_layer);
}

static void init(void) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "%li", time(NULL));
	window = window_create();
	window_set_click_config_provider(window, click_config_provider);
	window_set_window_handlers(window, (WindowHandlers) {
		.load = window_load,
		.unload = window_unload,
	});
	
	app_message_register_inbox_received(in_received_handler);
	app_message_register_inbox_dropped(in_dropped_handler);
	app_message_register_outbox_sent(out_sent_handler);
	app_message_register_outbox_failed(out_failed_handler);
	
	const uint32_t inbound_size=128;
	const uint32_t outbound_size=128;
	
	app_message_open(inbound_size,outbound_size);
	
	const bool animated = true;
	window_stack_push(window, animated);
}

static void deinit(void) {
	window_destroy(window);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}