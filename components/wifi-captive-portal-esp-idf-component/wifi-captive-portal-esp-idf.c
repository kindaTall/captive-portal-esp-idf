/**	wifi-captive-portal-esp-idf-component

  Copyright (c) 2021 Jeremy Carter <jeremy@jeremycarter.ca>

  This code is released under the license terms contained in the
  file named LICENSE, which is found in the top-level folder in
  this project. You must agree to follow those license terms,
  otherwise you aren't allowed to copy, distribute, or use any
  part of this project in any way.
*/
#include "wifi-captive-portal-esp-idf.h"

const char *TAG = "cap-core";

const char *wifi_captive_portal_esp_idf_wifi_task_name = "wifi_captive_portal_esp_idf_wifi_task";
const uint32_t wifi_captive_portal_esp_idf_wifi_task_stack_depth = 4096;
UBaseType_t wifi_captive_portal_esp_idf_wifi_task_priority = 5;

const char *wifi_captive_portal_esp_idf_httpd_task_name = "wifi_captive_portal_esp_idf_httpd_task";
const uint32_t wifi_captive_portal_esp_idf_httpd_task_stack_depth = 4096;
UBaseType_t wifi_captive_portal_esp_idf_httpd_task_priority = 5;

const char *wifi_captive_portal_esp_idf_task_name = "wifi_captive_portal_esp_idf_task";


static void wifi_captive_portal_esp_idf_wifi_stopped_event_handler(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data)
{
  ESP_LOGI(TAG, "event received: WIFI_CAPTIVE_PORTAL_ESP_IDF_WIFI_EVENT_STOPPED");
}

static void wifi_captive_portal_esp_idf_wifi_finish_event_handler(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data)
{
  ESP_LOGI(TAG, "event received: WIFI_CAPTIVE_PORTAL_ESP_IDF_WIFI_EVENT_FINISH");

  xTaskCreate(&wifi_captive_portal_esp_idf_httpd_task, wifi_captive_portal_esp_idf_httpd_task_name, wifi_captive_portal_esp_idf_httpd_task_stack_depth * 8, NULL, wifi_captive_portal_esp_idf_httpd_task_priority, NULL);
  ESP_LOGI(TAG, "Task started: %s", wifi_captive_portal_esp_idf_httpd_task_name);
}



void wifi_captive_portal_esp_idf(void)
{
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  esp_event_loop_args_t wifi_captive_portal_esp_idf_wifi_event_loop_args = {
      .queue_size = 5,
      .task_name = "wifi_captive_portal_esp_idf_wifi_event_loop_task", // task will be created
      .task_priority = uxTaskPriorityGet(NULL),
      .task_stack_size = wifi_captive_portal_esp_idf_wifi_task_stack_depth,
      .task_core_id = tskNO_AFFINITY};

  ESP_ERROR_CHECK(esp_event_loop_create(&wifi_captive_portal_esp_idf_wifi_event_loop_args, &wifi_captive_portal_esp_idf_wifi_event_loop_handle));
  ESP_ERROR_CHECK(esp_event_handler_instance_register_with(wifi_captive_portal_esp_idf_wifi_event_loop_handle, WIFI_CAPTIVE_PORTAL_ESP_IDF_WIFI_EVENT, WIFI_CAPTIVE_PORTAL_ESP_IDF_WIFI_EVENT_FINISH, wifi_captive_portal_esp_idf_wifi_finish_event_handler, wifi_captive_portal_esp_idf_wifi_event_loop_handle, NULL));
  ESP_ERROR_CHECK(esp_event_handler_instance_register_with(wifi_captive_portal_esp_idf_wifi_event_loop_handle, WIFI_CAPTIVE_PORTAL_ESP_IDF_WIFI_EVENT, WIFI_CAPTIVE_PORTAL_ESP_IDF_WIFI_EVENT_STOPPED, wifi_captive_portal_esp_idf_wifi_stopped_event_handler, wifi_captive_portal_esp_idf_wifi_event_loop_handle, NULL));

  esp_event_loop_args_t wifi_captive_portal_esp_idf_httpd_event_loop_args = {
      .queue_size = 5,
      .task_name = "wifi_captive_portal_esp_idf_httpd_event_loop_task", // task will be created
      .task_priority = uxTaskPriorityGet(NULL),
      .task_stack_size = wifi_captive_portal_esp_idf_httpd_task_stack_depth,
      .task_core_id = tskNO_AFFINITY};

  ESP_ERROR_CHECK(esp_event_loop_create(&wifi_captive_portal_esp_idf_httpd_event_loop_args, &wifi_captive_portal_esp_idf_httpd_event_loop_handle));

  xTaskCreate(&wifi_captive_portal_esp_idf_wifi_task, wifi_captive_portal_esp_idf_wifi_task_name, wifi_captive_portal_esp_idf_wifi_task_stack_depth * 8, NULL, wifi_captive_portal_esp_idf_wifi_task_priority, NULL);
  ESP_LOGI(TAG, "Task started: %s", wifi_captive_portal_esp_idf_wifi_task_name);
}
