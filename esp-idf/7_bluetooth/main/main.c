// Ref: https://github.com/espressif/esp-idf/blob/release/v3.3/examples/bluetooth/gatt_server/tutorial/Gatt_Server_Example_Walkthrough.md
// Ref: https://github.com/espressif/esp-idf/tree/release/v3.3/examples/bluetooth/gatt_server

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#include "sdkconfig.h"

#define GATTS_TAG "GATTS_DEMO"

///Declare the static function
static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

#define GATTS_SERVICE_UUID_TEST_A   0x00FF
#define GATTS_CHAR_UUID_TEST_A      0xFF01
#define GATTS_DESCR_UUID_TEST_A     0x3333
// ハンドルはつぎの4つ
// Service, Characteristic, Characteristic value, Characteristic descriptor 
#define GATTS_NUM_HANDLE_TEST_A     4

#define TEST_DEVICE_NAME            "ESP_BLE_EXAMPLE"
#define TEST_MANUFACTURER_DATA_LEN  17

#define GATTS_DEMO_CHAR_VAL_LEN_MAX 0x40

#define PREPARE_BUF_MAX_SIZE 1024

static esp_gatt_char_prop_t a_property = 0; // サービスの特性につけるプロパティ

// サービスの特性につける属性
// NULLにできないので適当な値をつける
static uint8_t char1_str[] = {0x11,0x22,0x33}; // ダミーデータ
static esp_attr_value_t gatts_demo_char1_val =
{
    .attr_max_len = GATTS_DEMO_CHAR_VAL_LEN_MAX,
    .attr_len     = sizeof(char1_str),
    .attr_value   = char1_str,
};

// アドバタイジングの設定が成功したか確認するためのフラグ
static uint8_t adv_config_done = 0;
#define adv_config_flag      (1 << 0)
#define scan_rsp_config_flag (1 << 1)


// ------------- アドバタイジングデータの定義 ---------------
// アドバタイジングデータは、31オクテット(バイト)までなら任意のデータ作れる
static uint8_t adv_service_uuid128[32] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    //first uuid, 16bit, [12],[13] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xEE, 0x00, 0x00, 0x00,
    //second uuid, 32bit, [12], [13], [14], [15] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};
// The length of adv data must be less than 31 bytes
//static uint8_t test_manufacturer[TEST_MANUFACTURER_DATA_LEN] =  {0x12, 0x23, 0x45, 0x56};
//adv data

// Ref: https://docs.espressif.com/projects/esp-idf/en/stable/api-reference/bluetooth/esp_gap_ble.html#_CPPv418esp_ble_adv_data_t
static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false, // アドバタイジングデータはスキャン応答ではない
    .include_name = true, // アドバタイジングデータはデバイス名を含む
    .include_txpower = true, // アドバタイジングデータは送信電力レベルを含む
    .min_interval = 0x0006, //slave connection min interval, Time = min_interval * 1.25 msec
    .max_interval = 0x0010, //slave connection max interval, Time = max_interval * 1.25 msec
    .appearance = 0x00, // デバイスの外観?
    .manufacturer_len = 0, // 製造者（ユーザ）が設定できる任意のデータの長さ
    .p_manufacturer_data =  NULL, // 製造者（ユーザ）が設定できる任意のデータのポインタ
    .service_data_len = 0, // サービスデータの長さ
    .p_service_data = NULL, // サービスデータのポインタ
    .service_uuid_len = sizeof(adv_service_uuid128), // サービスUUIDの長さ
    .p_service_uuid = adv_service_uuid128, // サービスUUID配列のポインタ
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
    // flag = LE General Discoverable Mode, BR/EDR Not Supported
};
// スキャン応答データ
static esp_ble_adv_data_t scan_rsp_data = {
    .set_scan_rsp = true, // アドバタイジングデータはスキャン応答である
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x0006,
    .max_interval = 0x0010,
    .appearance = 0x00,
    .manufacturer_len = 0, // TEST_MANUFACTURER_DATA_LEN, <- 任意データの例
    .p_manufacturer_data =  NULL, //&test_manufacturer[0], <- 任意データの例
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(adv_service_uuid128),
    .p_service_uuid = adv_service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};


// ------------- アドバタイジングパラメータの定義 ---------------
static esp_ble_adv_params_t adv_params = {
    .adv_int_min        = 0x20, // 最小アドバタイジングインターバル N * 0.625 mesc
    .adv_int_max        = 0x40, // 最大アドバタイジングインターバル N * 0.625 msec
    .adv_type           = ADV_TYPE_IND, // 接続可能アドバタイジング
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC, // オーナBTアドレスタイプをパブリックに
    //.peer_addr            =
    //.peer_addr_type       =
    .channel_map        = ADV_CHNL_ALL, // チャンネルマップは全て(37, 38, 39)
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY, // 誰でもスキャンと接続要求できる
};


// ------------- GATTプロファイルの定義 ---------------
// プロファイルは複数定義できるが、今回はひとつだけ作成
#define PROFILE_NUM 1
#define PROFILE_A_APP_ID 0
struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};
/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_A_APP_ID] = {
        .gatts_cb = gatts_profile_a_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    }
};


// ------------- 長いデータ書き込み用のバッファの定義 ---------------
typedef struct {
    uint8_t                 *prepare_buf;
    int                     prepare_len;
} prepare_type_env_t;
static prepare_type_env_t a_prepare_write_env;

void example_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);
void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);


static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        ESP_LOGI(GATTS_TAG, "GAP, アドバタイジングデータがセットされました");
        adv_config_done &= (~adv_config_flag);
        if (adv_config_done == 0){
            ESP_LOGI(GATTS_TAG, "GAP, アドバタイジングを開始します\n");
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
        ESP_LOGI(GATTS_TAG, "GAP, スキャン応答データがセットされました");
        adv_config_done &= (~scan_rsp_config_flag);
        if (adv_config_done == 0){
            ESP_LOGI(GATTS_TAG, "GAP, アドバタイジングを開始します\n");
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        ESP_LOGI(GATTS_TAG, "GAP, アドバタイジングが開始されました\n");
        //advertising start complete event to indicate advertising start successfully or failed
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(GATTS_TAG, "Advertising start failed\n");
        }
        break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT\n");
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(GATTS_TAG, "Advertising stop failed\n");
        } else {
            ESP_LOGI(GATTS_TAG, "Stop adv successfully\n");
        }
        break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
        ESP_LOGI(GATTS_TAG, "GAP, 接続パラメータが更新されました");
        ESP_LOGI(GATTS_TAG, "GAP, update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d\n",
                  param->update_conn_params.status,
                  param->update_conn_params.min_int,
                  param->update_conn_params.max_int,
                  param->update_conn_params.conn_int,
                  param->update_conn_params.latency,
                  param->update_conn_params.timeout);
        break;
    default:
        break;
    }
}

void example_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param){
    esp_gatt_status_t status = ESP_GATT_OK;
    if (param->write.need_rsp){
        ESP_LOGI(GATTS_TAG, "クライアントからの書き込み要求への応答が必要です");
        if (param->write.is_prep){
            // 書き込みデータが長いときは、準備書き込み要求が来る
            ESP_LOGI(GATTS_TAG, "長いデータの書き込み要求です");

            if (prepare_write_env->prepare_buf == NULL) {
                // 書き込みデータを格納するするバッファのメモリ領域を確保
                prepare_write_env->prepare_buf = (uint8_t *)malloc(PREPARE_BUF_MAX_SIZE*sizeof(uint8_t));
                prepare_write_env->prepare_len = 0;
                if (prepare_write_env->prepare_buf == NULL) {
                    ESP_LOGE(GATTS_TAG, "Gatt_server prep no mem\n");
                    status = ESP_GATT_NO_RESOURCES;
                }
            } else {
                if(param->write.offset > PREPARE_BUF_MAX_SIZE) {
                    status = ESP_GATT_INVALID_OFFSET;
                } else if ((param->write.offset + param->write.len) > PREPARE_BUF_MAX_SIZE) {
                    status = ESP_GATT_INVALID_ATTR_LEN;
                }
            }

            // 応答データを作成
            esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));
            gatt_rsp->attr_value.len = param->write.len;
            gatt_rsp->attr_value.handle = param->write.handle;
            gatt_rsp->attr_value.offset = param->write.offset;
            gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE; // 認証は不要
            memcpy(gatt_rsp->attr_value.value, param->write.value, param->write.len);
            // 応答データを送信
            esp_err_t response_err = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, gatt_rsp);
            if (response_err != ESP_OK){
               ESP_LOGE(GATTS_TAG, "Send response error\n");
            }
            free(gatt_rsp);
            if (status != ESP_GATT_OK){
                return;
            }
            memcpy(prepare_write_env->prepare_buf + param->write.offset,
                   param->write.value,
                   param->write.len);
            prepare_write_env->prepare_len += param->write.len;

        }else{
            // 応答する
            esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, NULL);
            ESP_LOGI(GATTS_TAG, "書き込み要求に対して応答しました");
        }
    }else{
        ESP_LOGI(GATTS_TAG, "クライアントからの書き込み要求ですが、応答は不要です");
    }
}

void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param){
    if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC){
        ESP_LOG_BUFFER_HEX(GATTS_TAG, prepare_write_env->prepare_buf, prepare_write_env->prepare_len);
    }else{
        ESP_LOGI(GATTS_TAG,"ESP_GATT_PREP_WRITE_CANCEL");
    }
    if (prepare_write_env->prepare_buf) {
        free(prepare_write_env->prepare_buf);
        prepare_write_env->prepare_buf = NULL;
    }
    prepare_write_env->prepare_len = 0;
}

static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    switch (event) {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(GATTS_TAG, "GATT, status:%d, アプリケーションID%dのプロファイルが登録されました\n", 
                param->reg.status, param->reg.app_id);
        gl_profile_tab[PROFILE_A_APP_ID].service_id.is_primary = true;
        gl_profile_tab[PROFILE_A_APP_ID].service_id.id.inst_id = 0x00;
        gl_profile_tab[PROFILE_A_APP_ID].service_id.id.uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_A_APP_ID].service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_TEST_A;

        // デバイス名の設定
        esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(TEST_DEVICE_NAME);
        if (set_dev_name_ret){
            ESP_LOGE(GATTS_TAG, "set device name failed, error code = %x", set_dev_name_ret);
        }

        // アドバタイジングデータの設定
        esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
        if (ret){
            ESP_LOGE(GATTS_TAG, "config adv data failed, error code = %x", ret);
        }
        adv_config_done |= adv_config_flag;
        // スキャン応答データの設定
        ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
        if (ret){
            ESP_LOGE(GATTS_TAG, "config scan response data failed, error code = %x", ret);
        }
        adv_config_done |= scan_rsp_config_flag;

        esp_ble_gatts_create_service(gatts_if, &gl_profile_tab[PROFILE_A_APP_ID].service_id, GATTS_NUM_HANDLE_TEST_A);
        break;

    case ESP_GATTS_READ_EVT: {
        ESP_LOGI(GATTS_TAG, 
                "GATT, クライアントからの読み取り要求です conn_id %d, trans_id %d, handle %d\n", 
                param->read.conn_id, param->read.trans_id, param->read.handle);
        // 応答データを作成
        // 今回はテキトーな値（0xdeadbeef）を返す
        esp_gatt_rsp_t rsp;
        memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
        rsp.attr_value.handle = param->read.handle;
        rsp.attr_value.len = 4;
        rsp.attr_value.value[0] = 0xde;
        rsp.attr_value.value[1] = 0xad;
        rsp.attr_value.value[2] = 0xbe;
        rsp.attr_value.value[3] = 0xef;
        // 特性とディスクリプタを作るときに自動応答をNULLにしたので、
        // 応答関数を用意する
        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
                                    ESP_GATT_OK, &rsp);
        break;
    }
    case ESP_GATTS_WRITE_EVT: {
        ESP_LOGI(GATTS_TAG, 
                "GATT, クライアントからの書き込み要求です conn_id %d, trans_id %d, handle %d", 
                param->write.conn_id, param->write.trans_id, param->write.handle);

        // 準備書き込みじゃなければ処理をする
        if (!param->write.is_prep){
            // 書き込みデータのサイズをと中身を表示
            ESP_LOGI(GATTS_TAG, "GATT, 書き込みデータのサイズ%d, データの中身:", param->write.len);
            ESP_LOG_BUFFER_HEX(GATTS_TAG, param->write.value, param->write.len);

            if (gl_profile_tab[PROFILE_A_APP_ID].descr_handle == param->write.handle 
                    && param->write.len == 2){
                uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
                if (descr_value == 0x0001){
                    if (a_property & ESP_GATT_CHAR_PROP_BIT_NOTIFY){
                        ESP_LOGI(GATTS_TAG, "notify enable");
                        uint8_t notify_data[15];
                        for (int i = 0; i < sizeof(notify_data); ++i)
                        {
                            notify_data[i] = i%0xff;
                        }
                        //the size of notify_data[] need less than MTU size
                        esp_ble_gatts_send_indicate(
                                gatts_if, 
                                param->write.conn_id, 
                                gl_profile_tab[PROFILE_A_APP_ID].char_handle,
                                sizeof(notify_data), 
                                notify_data, 
                                false
                                );
                    }
                }else if (descr_value == 0x0002){
                    if (a_property & ESP_GATT_CHAR_PROP_BIT_INDICATE){
                        ESP_LOGI(GATTS_TAG, "indicate enable");
                        uint8_t indicate_data[15];
                        for (int i = 0; i < sizeof(indicate_data); ++i)
                        {
                            indicate_data[i] = i%0xff;
                        }
                        //the size of indicate_data[] need less than MTU size
                        esp_ble_gatts_send_indicate(
                                gatts_if, 
                                param->write.conn_id, 
                                gl_profile_tab[PROFILE_A_APP_ID].char_handle,
                                sizeof(indicate_data), 
                                indicate_data, 
                                true
                                );
                    }
                }
                else if (descr_value == 0x0000){
                    ESP_LOGI(GATTS_TAG, "notify/indicate disable ");
                }else{
                    ESP_LOGE(GATTS_TAG, "unknown descr value");
                    ESP_LOG_BUFFER_HEX(GATTS_TAG, param->write.value, param->write.len);
                }

            }
        }
        example_write_event_env(gatts_if, &a_prepare_write_env, param);
        break;
    }
    case ESP_GATTS_EXEC_WRITE_EVT:
        ESP_LOGI(GATTS_TAG,"GATT, クライアントからの長いデータの書き込み要求の終わりです");
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        example_exec_write_event_env(&a_prepare_write_env, param);
        break;
    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
        break;
    case ESP_GATTS_UNREG_EVT:
        break;
    case ESP_GATTS_CREATE_EVT:
        ESP_LOGI(GATTS_TAG, "GATT, status %d, サービスが作成されました service_handle %d", 
                param->create.status, param->create.service_handle);
        gl_profile_tab[PROFILE_A_APP_ID].service_handle = param->create.service_handle;
        gl_profile_tab[PROFILE_A_APP_ID].char_uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_A_APP_ID].char_uuid.uuid.uuid16 = GATTS_CHAR_UUID_TEST_A;

        ESP_LOGI(GATTS_TAG, "GATT, サービス%dを開始します\n", 
                gl_profile_tab[PROFILE_A_APP_ID].service_handle);
        esp_ble_gatts_start_service(gl_profile_tab[PROFILE_A_APP_ID].service_handle);

        // プロパティ（特性を読める、書ける、通知できる）を作成
        // プロパティはクライアント側に通知されるもの
        a_property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
        // サービスに特性(Characteristic)を追加する
        esp_err_t add_char_ret = esp_ble_gatts_add_char(
                gl_profile_tab[PROFILE_A_APP_ID].service_handle, 
                &gl_profile_tab[PROFILE_A_APP_ID].char_uuid,
                ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, // 読み書き許可を設定
                a_property, // プロパティを設定（許可ではない、クライアントに提示するもの）
                &gatts_demo_char1_val, // 属性を付与
                NULL // 読み書き操作に対する自動応答をしない
                );

        if (add_char_ret){
            ESP_LOGE(GATTS_TAG, "add char failed, error code =%x",add_char_ret);
        }
        break;
    case ESP_GATTS_ADD_INCL_SRVC_EVT:
        break;
    case ESP_GATTS_ADD_CHAR_EVT: {
        uint16_t length = 0;
        const uint8_t *prf_char;

        ESP_LOGI(GATTS_TAG, 
                "GATT, status %d, サービスに特性が追加されました, attr_handle %d, service_handle %d", 
                param->add_char.status, 
                param->add_char.attr_handle, 
                param->add_char.service_handle);
        gl_profile_tab[PROFILE_A_APP_ID].char_handle = param->add_char.attr_handle;
        gl_profile_tab[PROFILE_A_APP_ID].descr_uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_A_APP_ID].descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;

        // デバッグのため、セットされた属性を取得する
        esp_err_t get_attr_ret = esp_ble_gatts_get_attr_value(param->add_char.attr_handle,  &length, &prf_char);
        if (get_attr_ret == ESP_FAIL){
            ESP_LOGE(GATTS_TAG, "ILLEGAL HANDLE");
        }
        ESP_LOGI(GATTS_TAG, "GATT, セットされた属性のサイズは%xです", length);
        ESP_LOGI(GATTS_TAG, "GATT, 属性の中身prf_charを表示します");
        for(int i = 0; i < length; i++){
            ESP_LOGI(GATTS_TAG, "GATT, prf_char[%x] =%x\n",i,prf_char[i]);
        }

        // 特性ディスクリプタを追加する
        esp_err_t add_descr_ret = esp_ble_gatts_add_char_descr(
                gl_profile_tab[PROFILE_A_APP_ID].service_handle, 
                &gl_profile_tab[PROFILE_A_APP_ID].descr_uuid,
                ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, // 読み書き許可
                NULL, // ディスクリプタの初期値をNULLに
                NULL
                );
        if (add_descr_ret){
            ESP_LOGE(GATTS_TAG, "add char descr failed, error code =%x", add_descr_ret);
        }
        break;
    }
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
        gl_profile_tab[PROFILE_A_APP_ID].descr_handle = param->add_char_descr.attr_handle;
        ESP_LOGI(GATTS_TAG, 
                "GATT, status %d, 特性にディスクリプタが追加されました, attr_handle %d, service_handle %d\n", 
                param->add_char_descr.status, 
                param->add_char_descr.attr_handle, 
                param->add_char_descr.service_handle);
        break;
    case ESP_GATTS_DELETE_EVT:
        break;
    case ESP_GATTS_START_EVT:
        ESP_LOGI(GATTS_TAG, "GATT, status %d, サービスが開始されました, service_handle %d\n",
                 param->start.status, param->start.service_handle);
        break;
    case ESP_GATTS_STOP_EVT:
        break;
    case ESP_GATTS_CONNECT_EVT: {
        // デバイスが接続されたので、接続パラメータを更新する
        esp_ble_conn_update_params_t conn_params = {0};
        // BDA(Bluetooth Device Adress)の取得
        memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        /* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
        conn_params.latency = 0;
        conn_params.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
        conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
        conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms
        ESP_LOGI(GATTS_TAG,
                "GATT, クライアントが接続されました, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x:\n",
                param->connect.conn_id,
                param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
                param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
        gl_profile_tab[PROFILE_A_APP_ID].conn_id = param->connect.conn_id;
        //start sent the update connection parameters to the peer device.
        esp_ble_gap_update_conn_params(&conn_params);
        break;
    }
    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_DISCONNECT_EVT, disconnect reason 0x%x", param->disconnect.reason);
        esp_ble_gap_start_advertising(&adv_params);
        break;
    case ESP_GATTS_CONF_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_CONF_EVT, status %d attr_handle %d", param->conf.status, param->conf.handle);
        if (param->conf.status != ESP_GATT_OK){
            ESP_LOG_BUFFER_HEX(GATTS_TAG, param->conf.value, param->conf.len);
        }
        break;
    case ESP_GATTS_OPEN_EVT:
    case ESP_GATTS_CANCEL_OPEN_EVT:
    case ESP_GATTS_CLOSE_EVT:
    case ESP_GATTS_LISTEN_EVT:
    case ESP_GATTS_CONGEST_EVT:
    default:
        break;
    }
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            ESP_LOGI(GATTS_TAG, "gatts_event_handler, gatts_ifをプロファイルテーブルに保存します\n");
            gl_profile_tab[param->reg.app_id].gatts_if = gatts_if;
        } else {
            ESP_LOGI(GATTS_TAG, "Reg app failed, app_id %04x, status %d\n",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }

    /* If the gatts_if equal to profile A, call profile A cb handler,
     * so here call each profile's callback */
    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            if (gatts_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                    gatts_if == gl_profile_tab[idx].gatts_if) {
                if (gl_profile_tab[idx].gatts_cb) {
                    gl_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}

void app_main()
{
    esp_err_t ret;
    // フラッシュメモリの初期化
    // 今回のBLEでは使わない？
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    // BLEを使うので、使わないクラシックBTのコントローラのメモリを開放する
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    // BT(Bluetooth)コントローラの設定
    // コントローラはユーザから見えないので、何をやっているのか心配しなくて良い
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    // コントローラをBLEモードにする
    // クラシックも設定できるけど、上でメモリを開放したので今回は無理
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    // Bluedroidの初期化
    // BluedroidはAndroidで使われているBTプロトコルスタックの実装
    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s init bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    // Bluedroidの有効化
    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s enable bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    // ここまででBluetoothスタックは稼働するようになったが
    // スタックとやり取りする機能を実装しないと意味がない
    // 機能はイベントハンドラとして実装する

    // GATTイベントハンドラを登録する
    // GATTはデータの送受信やデータの構造を定義したプロファイル
    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret){
        ESP_LOGE(GATTS_TAG, "gatts register error, error code = %x", ret);
        return;
    }
    // GAPイベントハンドラを登録する
    // GAPはデバイス検索やコネクションの確立などを担うプロファイル
    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret){
        ESP_LOGE(GATTS_TAG, "gap register error, error code = %x", ret);
        return;
    }
    // ユーザ定義のIDを使って、アプリケーションプロファイルを登録する
    // プロファイルは複数定義できるが、今回はひとつだけ
    ret = esp_ble_gatts_app_register(PROFILE_A_APP_ID);
    if (ret){
        ESP_LOGE(GATTS_TAG, "gatts app register error, error code = %x", ret);
        return;
    }

    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
    if (local_mtu_ret){
        ESP_LOGE(GATTS_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
    }

    return;
}
