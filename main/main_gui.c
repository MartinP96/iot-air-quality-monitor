#include "main_gui.h"


/************************
 *   GLOBAL VARIABLES
 ***********************/
SemaphoreHandle_t xGuiSemaphore;

/************************
 *   STATIC VARIABLES
 ***********************/
static lv_obj_t * tv;
static lv_obj_t * t1;
static lv_obj_t * t2;
static lv_obj_t * t3;
static lv_obj_t * wifi_keyboard;
static lv_obj_t * wifi_popup;
static lv_style_t style_box;
static lv_obj_t* wifi_dropdown;
static lv_obj_t* preload;
static lv_obj_t* wifi_mbox;
static lv_style_t style_modal;
static lv_obj_t *wifi_pass_textbox;
static lv_obj_t *lmeter_CO2;
static lv_obj_t *label_CO2;
static gui_wifi_data_typ wifi_data;
static lv_obj_t *temperature_textbox;
static lv_obj_t *humidity_textbox;
static lv_obj_t *lmeter_VOC;
static lv_obj_t *label_VOC;

/***********************************
 *          GUI Tasks 
 ***********************************/
// Main GUI Task

void T00_user_interface_task(void *pvParameter)
{
    // Declare imag buffers
    lv_color_t* img_buffer1 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    lv_color_t* img_buffer2 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(img_buffer1 != NULL);
    assert(img_buffer2 != NULL);

    // Initialize LVGL display driver
    gui_initialize(img_buffer1, img_buffer2);
    
    // Create GUI Application
    gui_create_gui();

    // Create sub tasks
    xTaskCreate(T_measurement_task, "measurement_task", 1024*4, NULL, 1, NULL);
    xTaskCreate(T02_02_refresh_task, "gui_refersh_task", 1024*2, NULL, 0, NULL);
    xTaskCreate(T_mqtt_communication_task, "communication_task", 1024*2, NULL, 0, NULL);
    
    //Main GUI task while loop
    while (1) {
    /* Delay 1 tick (assumes FreeRTOS tistatic void ck is 10ms */
        vTaskDelay(pdMS_TO_TICKS(10));
        
        /* Try to take the semaphore, call lvgl related function on success */
        if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)) {
            lv_task_handler();
            xSemaphoreGive(xGuiSemaphore);
        }
    }
    free(img_buffer1);
    free(img_buffer2);
    vTaskDelete(NULL);
}

// WiFi Scan Task //
static void T02_01_wifi_scan_task(void *pvParameter)
{
    // Scan for wifi access points
    lv_obj_set_hidden(preload, false); 
    lv_obj_set_hidden(wifi_dropdown, false);
    lv_dropdown_set_options(wifi_dropdown, "Searching...\n");

    wifi_ap_record_t ap_info[10];
    memset(ap_info, 0, sizeof(ap_info));
    int ap_count = wifi_scan(ap_info);
    char wifi_ssid_str[100] = "Select Wifi\n";
    lv_dropdown_clear_options(wifi_dropdown);
    for (int i = 0; (i < DEFAULT_SCAN_LIST_SIZE) && (i < ap_count); i++) {
        ESP_LOGI(TAG_WIFI, "SSID \t\t%s", ap_info[i].ssid);
        strcat(wifi_ssid_str, (char*) ap_info[i].ssid);
        if (i < ap_count - 1) strcat(wifi_ssid_str, "\n");
    }
    lv_dropdown_set_options(wifi_dropdown,wifi_ssid_str);
    lv_obj_set_hidden(preload, true); 
    
    vTaskDelete(NULL);
}

// GUI Refresh Measurement indicators task //
static void T02_02_refresh_task(void *pvParameter)
{
    while(1)
    {
        measurement_packet_st received_value;
        if (xQueueReceive(measurement_queue, &received_value, portMAX_DELAY ) == pdPASS)
		{
            // co2 meter
			lv_linemeter_set_value(lmeter_CO2, received_value.co2);
            char co2_val_str[10];
            sprintf(co2_val_str, "%d", received_value.co2);
            strcat(co2_val_str, "\nppm");
            lv_label_set_text(label_CO2, co2_val_str);

            // temperature meter
            char temperature_str[20];
            sprintf(temperature_str, "%.2f", received_value.temperature);
            strcat(temperature_str, " °C");
            
            if (strcmp(temperature_str, lv_textarea_get_text(temperature_textbox)) != 0) {
            lv_textarea_set_text(temperature_textbox, temperature_str); }

            // humidity meter
            char humidty_str[20];
            sprintf(humidty_str, "%.2f", received_value.humidity);
            strcat(humidty_str, " %RH");
            if (strcmp(humidty_str, lv_textarea_get_text(humidity_textbox)) != 0) {
            lv_textarea_set_text(humidity_textbox, humidty_str);}

            // VOC meter
            /*
            lv_linemeter_set_value(lmeter_VOC, received_value.voc);
            char voc_val_str[10];
            sprintf(voc_val_str, "%d", received_value.voc);
            lv_label_set_text(label_VOC, voc_val_str);
            */
		}
    }
    vTaskDelete(NULL);
}

/***********************************
 * GUI_INITIALIZE - ini LCD gui
 ***********************************/

static void gui_initialize(lv_color_t *buf1, lv_color_t *buf2)
{
    xGuiSemaphore = xSemaphoreCreateMutex();
    
    /* Initialize SPI or I2C bus used by the drivers */
    lv_init();
    lvgl_driver_init();

    static lv_disp_buf_t disp_buf;
    uint32_t size_in_px = DISP_BUF_SIZE;
    lv_disp_buf_init(&disp_buf, buf1, buf2, size_in_px);
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = disp_driver_flush;
    disp_drv.buffer = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    /* Touch screen configuration */
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.read_cb = touch_driver_read;
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    lv_indev_drv_register(&indev_drv);

    /* Create and start a periodic timer interrupt to call lv_tick_inc */
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_task,
        .name = "periodic_gui"
    };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));
}

/********************************************
 * GUI_CREATE_GUI - Create GUI application
 ********************************************/

void static gui_create_gui(void)
{
    lv_disp_size_t disp_size = lv_disp_get_size_category(NULL);

    // Create Tab viewpreload
    tv = lv_tabview_create(lv_scr_act(), NULL);

    //lv_obj_set_style_local_pad_top(tv, LV_TABVIEW_PART_TAB_BG, LV_STATE_DEFAULT, 1);
	//lv_obj_set_style_local_pad_bottom(tv, LV_TABVIEW_PART_TAB_BG, LV_STATE_DEFAULT, 1);
	//lv_obj_set_style_local_pad_top(tv, LV_TABVIEW_PART_TAB_BTN, LV_STATE_DEFAULT, 1);
	//lv_obj_set_style_local_pad_bottom(tv, LV_TABVIEW_PART_TAB_BTN, LV_STATE_DEFAULT, 1);

    t1 = lv_tabview_add_tab(tv, "1");
    // t2 = lv_tabview_add_tab(tv, "2");
    t3 = lv_tabview_add_tab(tv, "Conf.");
    lv_style_init(&style_box);
    lv_style_set_value_align(&style_box, LV_STATE_DEFAULT, LV_ALIGN_OUT_TOP_LEFT);
    lv_style_set_value_ofs_y(&style_box, LV_STATE_DEFAULT, - LV_DPX(10));
    lv_style_set_margin_top(&style_box, LV_STATE_DEFAULT, LV_DPX(30));

    // Create Tabs
    Create_TAB1(t1);
    //Create_TAB2(t2);
    Create_TAB3(t3);
}

/********************************************
 * Create_TAB1 - Create Tab 1 GUI application
 ********************************************/

static void Create_TAB1(lv_obj_t *tab_ptr)
{
    // CO2 Meter
    lmeter_CO2 = lv_linemeter_create(t1, NULL);
    lv_obj_set_drag_parent(lmeter_CO2, true);
    lv_obj_set_size(lmeter_CO2, 140, 140);
    lv_obj_add_style(lmeter_CO2, LV_LINEMETER_PART_MAIN, &style_box);
    lv_obj_set_x(lmeter_CO2, +10);
    lv_obj_set_y(lmeter_CO2, +35);
    lv_obj_set_style_local_value_str(lmeter_CO2, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, "CO2:");
    lv_linemeter_set_range(lmeter_CO2, 0, 5000);
    label_CO2 = lv_label_create(lmeter_CO2, NULL);
    lv_obj_align(label_CO2, lmeter_CO2, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_local_text_font(label_CO2, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_theme_get_font_title());

    // Temperature & Humidity
    temperature_textbox = lv_textarea_create(t1, NULL);
    lv_obj_set_size(temperature_textbox, 120, 35);
    lv_obj_align(temperature_textbox, NULL, LV_ALIGN_CENTER, 80, -25);
    lv_textarea_set_cursor_hidden(temperature_textbox, true);
    lv_textarea_set_one_line(temperature_textbox, true);

    humidity_textbox = lv_textarea_create(t1, NULL);
    lv_obj_set_size(humidity_textbox, 120, 35);
    lv_obj_align(humidity_textbox, NULL, LV_ALIGN_CENTER, 80, +30);
    lv_textarea_set_cursor_hidden(humidity_textbox, true);
    lv_textarea_set_one_line(humidity_textbox, true);
}

/********************************************
 * Create_TAB2 - Create Tab 2 GUI application
 ********************************************/

static void Create_TAB2(lv_obj_t *tab_ptr)
{
    // CREATE Line Meter (VOC)
    lmeter_VOC = lv_linemeter_create(t2, NULL);
    lv_obj_set_drag_parent(lmeter_VOC, true);
    lv_linemeter_set_value(lmeter_VOC, 50);
    //lv_obj_set_size(lmeter_CO2, meter_size, meter_size);
    lv_obj_set_size(lmeter_VOC, 140, 140);
    lv_obj_add_style(lmeter_VOC, LV_LINEMETER_PART_MAIN, &style_box);
    lv_obj_set_x(lmeter_VOC, +10);
    lv_obj_set_y(lmeter_VOC, +35);
    lv_linemeter_set_range(lmeter_VOC, 0, 4096);
    lv_obj_set_style_local_value_str(lmeter_VOC, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, "VOC:");
    label_VOC = lv_label_create(lmeter_VOC, NULL);
    lv_obj_align(label_VOC, lmeter_VOC, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_local_text_font(label_VOC, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_theme_get_font_title());

    // CREATE Line Meter (OZONE)
    lv_obj_t * lmeter_OZONE = lv_linemeter_create(t2, NULL);
    lv_obj_set_drag_parent(lmeter_OZONE, true);
    lv_linemeter_set_value(lmeter_OZONE, 50);
    //lv_obj_set_size(lmeter_CO2, meter_size, meter_size);
    lv_obj_set_size(lmeter_OZONE, 140, 140);
    lv_obj_add_style(lmeter_OZONE, LV_LINEMETER_PART_MAIN, &style_box);
    lv_obj_set_x(lmeter_OZONE, +160);
    lv_obj_set_y(lmeter_OZONE, +35);
    lv_obj_set_style_local_value_str(lmeter_OZONE, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, "OZONE:");
    lv_obj_t * label_OZONE = lv_label_create(lmeter_OZONE, NULL);
    lv_obj_align(label_OZONE, lmeter_OZONE, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_local_text_font(label_OZONE, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_theme_get_font_title());
}

/********************************************
 * Create_TAB3 - Create Tab 1 GUI application
 ********************************************/

static void Create_TAB3(lv_obj_t *tab_ptr)
{
    // Create wifi toggle button
    lv_obj_t *switch_enable_wifi = lv_switch_create(tab_ptr, NULL);
    lv_obj_set_x(switch_enable_wifi, +120);
    lv_obj_set_y(switch_enable_wifi, +17);
    lv_obj_set_event_cb(switch_enable_wifi, wifi_enable_switch_event_handler);
    lv_obj_t * label1 = lv_label_create(tab_ptr, NULL);
    lv_label_set_text(label1, "Wifi Enable:");
    lv_obj_align(label1, switch_enable_wifi, LV_ALIGN_OUT_LEFT_MID, -10, 0);
    //lv_switch_on(switch_enable_wifi, LV_ANIM_OFF);
    lv_switch_off(switch_enable_wifi, LV_ANIM_OFF);

    // Create dropdown menu
    lv_disp_size_t disp_size = lv_disp_get_size_category(NULL);
    //lv_obj_t * wifi_dropdown = lv_dropdown_create(tab_ptr, NULL);
    wifi_dropdown = lv_dropdown_create(tab_ptr, NULL);
    lv_obj_add_style(wifi_dropdown, LV_CONT_PART_MAIN, &style_box);
    lv_obj_set_style_local_value_str(wifi_dropdown, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, "Dropdown");
    lv_obj_set_width(wifi_dropdown, 250);
    lv_obj_set_y(wifi_dropdown, +90);
    lv_obj_set_x(wifi_dropdown, +10);
    lv_obj_set_style_local_value_str(wifi_dropdown, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, "WiFi Settings:");
    lv_dropdown_set_dir(wifi_dropdown, LV_DROPDOWN_DIR_DOWN);
    lv_dropdown_set_options(wifi_dropdown, "\n");
    lv_obj_set_event_cb(wifi_dropdown, wifi_connection_select_event_handler);
    lv_obj_set_hidden(wifi_dropdown, true);

    /*Create a Preloader object*/
    preload = lv_spinner_create(tab_ptr, NULL);
    lv_obj_set_size(preload, 35, 35);
    lv_obj_set_style_local_line_width(preload, LV_SPINNER_PART_BG, LV_STATE_DEFAULT, 5);
    lv_obj_set_style_local_line_width(preload, LV_SPINNER_PART_INDIC, LV_STATE_DEFAULT, 5);
    lv_obj_align(preload, switch_enable_wifi, LV_ALIGN_OUT_RIGHT_MID, +20, 0);
    lv_obj_set_hidden(preload, true); 
}


/********************************************
 *         GUI_EVENT_FUNCTIONS
 ********************************************/

// WIFI ENABLE BUTTON
static void wifi_enable_switch_event_handler(lv_obj_t * obj, lv_event_t event)
{
    if(event == LV_EVENT_VALUE_CHANGED) {
        if (lv_switch_get_state(obj)) {
           //xTaskCreate(T02_01_wifi_scan_task, "wifiscan", 1024*6, NULL, 3, NULL);
           xTaskCreatePinnedToCore(T02_01_wifi_scan_task, "gui_wifiscan", 1024*4, NULL, 3, NULL, 1);
           lv_obj_set_hidden(wifi_dropdown, false);  
        }
        else {
            wifi_disconnect();
            lv_obj_set_hidden(wifi_dropdown, true);
        }
    }
}

//WIFI DROPDOWN SELECT BUTTON
static void wifi_connection_select_event_handler(lv_obj_t * obj_dropdown, lv_event_t event)
{
    if(event == LV_EVENT_VALUE_CHANGED ) {

        wifi_popup = lv_obj_create(lv_scr_act(), NULL);
        lv_obj_reset_style_list(wifi_popup, LV_OBJ_PART_MAIN);
        lv_obj_add_style(wifi_popup, LV_OBJ_PART_MAIN, &style_modal);
        lv_obj_set_pos(wifi_popup, 0, -80);
        lv_obj_set_size(wifi_popup, LV_HOR_RES, LV_VER_RES);
        wifi_mbox = lv_msgbox_create(wifi_popup, NULL);
        char ssid[30];
        lv_dropdown_get_selected_str(obj_dropdown, ssid, sizeof(ssid));
        lv_msgbox_set_text(wifi_mbox, ssid);
        lv_obj_align(wifi_mbox, NULL, LV_ALIGN_CENTER, 0, 0);

        // Create Text box
        wifi_pass_textbox = lv_textarea_create(wifi_mbox, NULL);
        lv_textarea_set_text(wifi_pass_textbox, "Enter Wifi Password");
        lv_textarea_set_one_line(wifi_pass_textbox, true);
        
        // Create Keyboard
        wifi_keyboard = lv_keyboard_create(lv_scr_act(), NULL);
        lv_keyboard_set_textarea(wifi_keyboard, wifi_pass_textbox); // Focus it on one of the text areas to start 
        lv_keyboard_set_cursor_manage(wifi_keyboard, true); // Automatically show/hide cursors on text areas 
        lv_obj_set_event_cb(wifi_keyboard, wifi_keyboard_event_handler);
    }
}

// WIFI PASSWORD KEYBOARD SHOW EVENT
void wifi_keyboard_event_handler(lv_obj_t * obj, lv_event_t event)
{
    lv_keyboard_def_event_cb(wifi_keyboard, event);

    if (event == LV_EVENT_CANCEL) {
        if(wifi_keyboard) {
            lv_obj_del(wifi_keyboard);
            lv_obj_del(wifi_popup);
            wifi_keyboard = NULL;
            wifi_popup = NULL;
        }
    } else if (event == LV_EVENT_APPLY) {
        if(wifi_keyboard) {

            //char temp_buf[100];
            lv_dropdown_get_selected_str(wifi_dropdown, wifi_data.ssid, sizeof(wifi_data.ssid));
            wifi_connect(wifi_data.ssid, wifi_data.pass);

            lv_obj_del(wifi_keyboard);
            lv_obj_del(wifi_pass_textbox);
            lv_obj_del(wifi_popup);
            wifi_keyboard = NULL;
            wifi_popup = NULL;
            wifi_pass_textbox = NULL;

        }
    }
}

/********************************************
 *         GUI_HELPER_FUNCTIONS
 ********************************************/

static void lv_tick_task(void *arg) 
{
    (void) arg;
    lv_tick_inc(LV_TICK_PERIOD_MS);
}
