#include <stdio.h>
#include "https_dashboard.h"

#define WEB_PAGE_MOUNT_POINT_IN_FS "/www"
#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
#define SCRATCH_BUFSIZE (10240)
#define JSON_BUFFER_SIZE 1024

extern const unsigned char server_cert_pem_start[] asm("_binary_servercert_pem_start");
extern const unsigned char server_cert_pem_end[]   asm("_binary_servercert_pem_end");
extern const unsigned char prvtkey_pem_start[]     asm("_binary_prvtkey_pem_start");
extern const unsigned char prvtkey_pem_end[]       asm("_binary_prvtkey_pem_end");

static const char *TAG = "SERVER";

char json_raw_values_global[JSON_BUFFER_SIZE] = "{\"tensao\":0,\"corrente\":0,\"potencia\":0}";
char json_fft_global[JSON_BUFFER_SIZE] = "{\"ordens\":[],\"tensao_harm\":[],\"corrente_harm\":[]}";

SemaphoreHandle_t json_mutex = NULL; 

#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

typedef struct rest_server_context {
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[SCRATCH_BUFSIZE];
} rest_server_context_t;

static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filepath)
{
    const char *type = "text/plain";
    if (CHECK_FILE_EXTENSION(filepath, ".html")) {
        type = "text/html";
    } else if (CHECK_FILE_EXTENSION(filepath, ".js")) {
        type = "application/javascript";
    } else if (CHECK_FILE_EXTENSION(filepath, ".css")) {
        type = "text/css";
    } else if (CHECK_FILE_EXTENSION(filepath, ".png")) {
        type = "image/png";
    } else if (CHECK_FILE_EXTENSION(filepath, ".ico")) {
        type = "image/x-icon";
    } else if (CHECK_FILE_EXTENSION(filepath, ".svg")) {
        type = "text/xml";
    }
    return httpd_resp_set_type(req, type);
}

static esp_err_t init_littlefs(char *partition_name)
{
    ESP_LOGI(TAG, "Starting littlefs...");
    esp_vfs_littlefs_conf_t conf = {
        .base_path = WEB_PAGE_MOUNT_POINT_IN_FS,
        .partition_label = partition_name,
        .format_if_mount_failed = true,
        .dont_mount = false,
    };

    esp_err_t err = esp_vfs_littlefs_register(&conf);
    if (err != ESP_OK) {
        if (err == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (err == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find LittleFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize LittleFS (%s)", esp_err_to_name(err));
        }
        return err;
    }
    
    size_t total = 0, used = 0;
    err = esp_littlefs_info(conf.partition_label, &total, &used);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get LittleFS partition information (%s)", esp_err_to_name(err));
        esp_littlefs_format(conf.partition_label);
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    ESP_LOGI(TAG, "OK");
    return ESP_OK;
}

static esp_err_t system_info_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    cJSON_AddStringToObject(root, "chip", CONFIG_IDF_TARGET);
    cJSON_AddStringToObject(root, "idf_version", IDF_VER);
    cJSON_AddNumberToObject(root, "cores", chip_info.cores);
    const char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    free((void *)sys_info);
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t raw_values_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");

    char json_resposta[JSON_BUFFER_SIZE];

    if (json_mutex && xSemaphoreTake(json_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
        strlcpy(json_resposta, json_raw_values_global, sizeof(json_resposta));
        xSemaphoreGive(json_mutex);
    } else {
        strlcpy(json_resposta, "{\"erro\":\"dados_temporariamente_indisponiveis\"}", sizeof(json_resposta));
    }

    httpd_resp_sendstr(req, json_resposta);
    return ESP_OK;
}


static esp_err_t fft_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");

    char json_resposta[JSON_BUFFER_SIZE];

    if (json_mutex && xSemaphoreTake(json_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
        strlcpy(json_resposta, json_fft_global, sizeof(json_resposta));
        xSemaphoreGive(json_mutex);
    } else {
        strlcpy(json_resposta, "{\"erro\":\"dados_temporariamente_indisponiveis\"}", sizeof(json_resposta));
    }

    httpd_resp_sendstr(req, json_resposta);
    return ESP_OK;
}

static esp_err_t rest_common_get_handler(httpd_req_t *req)
{
    char filepath[FILE_PATH_MAX];

    rest_server_context_t *rest_context = (rest_server_context_t *)req->user_ctx;
    strlcpy(filepath, rest_context->base_path, sizeof(filepath));
    if (req->uri[strlen(req->uri) - 1] == '/') {
        strlcat(filepath, "/index.html", sizeof(filepath));
    } else {
        strlcat(filepath, req->uri, sizeof(filepath));
    }
    int fd = open(filepath, O_RDONLY, 0);
    if (fd == -1) {
        ESP_LOGE(TAG, "Failed to open file : %s", filepath);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }

    set_content_type_from_file(req, filepath);

    char *chunk = rest_context->scratch;
    ssize_t read_bytes;
    do {
        /* Read file in chunks into the scratch buffer */
        read_bytes = read(fd, chunk, SCRATCH_BUFSIZE);
        if (read_bytes == -1) {
            ESP_LOGE(TAG, "Failed to read file : %s", filepath);
        } else if (read_bytes > 0) {
            /* Send the buffer contents as HTTP response chunk */
            if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK) {
                close(fd);
                ESP_LOGE(TAG, "File sending failed!");
                /* Abort sending file */
                httpd_resp_sendstr_chunk(req, NULL);
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
                return ESP_FAIL;
            }
        }
    } while (read_bytes > 0);
    /* Close file after sending complete */
    close(fd);
    ESP_LOGI(TAG, "File sending complete");
    /* Respond with an empty chunk to signal HTTP response completion */
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

esp_err_t update_server_data_values(const char *json_raw_values, const char *json_fft){
    if (json_mutex == NULL) {
        ESP_LOGE(TAG, "Json mutex is not working.");
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(json_mutex, pdMS_TO_TICKS(500)) == pdTRUE) {
        if (json_raw_values) strlcpy(json_raw_values_global, json_raw_values, sizeof(json_raw_values_global));
        if (json_fft) strlcpy(json_fft_global, json_fft, sizeof(json_fft_global));
        xSemaphoreGive(json_mutex);
        
        return ESP_OK;
    }

    return ESP_ERR_TIMEOUT;
}

esp_err_t server_setup(char* partition_name, char* host_name, char* instance_name, SemaphoreHandle_t mutex){

    if(mutex == NULL){
        ESP_LOGE(TAG,"Semaphore is not valid");
        return ESP_ERR_INVALID_STATE;
    }else{
        json_mutex = mutex;
    }

    esp_err_t ret = init_littlefs(partition_name);
    if(ret != ESP_OK){
        ESP_LOGE(TAG, "Failer to register littlefs: %s", esp_err_to_name(ret));
        return ret;
    }

    
    ESP_LOGI(TAG,"Incializando o servidor https...");
    ret = mdns_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG,"Failet to init mDNS: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = mdns_hostname_set(host_name);
    if(ret != ESP_OK){
        ESP_LOGE(TAG,"Failet to set host name: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = mdns_instance_name_set(instance_name);
    if(ret != ESP_OK){
        ESP_LOGE(TAG,"Failet to set instance name: %s", esp_err_to_name(ret));
        return ret;
    }

    mdns_txt_item_t serviceTxtData[] = {
        {"chip", CONFIG_IDF_TARGET},
        {"path", "/"}
    };

    ret = mdns_service_add("ESP32-WebServer", "_https", "_tcp", 443, serviceTxtData,
                                     sizeof(serviceTxtData) / sizeof(serviceTxtData[0]));
    if(ret != ESP_OK){
        ESP_LOGE(TAG, "Failed to add mdns service: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_RETURN_ON_FALSE(WEB_PAGE_MOUNT_POINT_IN_FS && strlen(WEB_PAGE_MOUNT_POINT_IN_FS) < ESP_VFS_PATH_MAX, ESP_ERR_INVALID_ARG, TAG, "Invalid base path");
    rest_server_context_t *rest_context = calloc(1, sizeof(rest_server_context_t));
    ESP_RETURN_ON_FALSE(rest_context, ESP_ERR_NO_MEM, TAG, "No memory for rest context");
    strlcpy(rest_context->base_path, WEB_PAGE_MOUNT_POINT_IN_FS, sizeof(rest_context->base_path));

    httpd_handle_t server = NULL;
    httpd_ssl_config_t config = HTTPD_SSL_CONFIG_DEFAULT();

    config.servercert = server_cert_pem_start;
    config.servercert_len = server_cert_pem_end - server_cert_pem_start;
    config.prvtkey_pem = prvtkey_pem_start;
    config.prvtkey_len = prvtkey_pem_end - prvtkey_pem_start;

    config.httpd.uri_match_fn = httpd_uri_match_wildcard;

    ESP_GOTO_ON_ERROR(httpd_ssl_start(&server, &config), err, TAG, "Failed to start http server");
    
    httpd_uri_t system_info_get_uri = {
        .uri = "/api/v1/system/info",
        .method = HTTP_GET,
        .handler = system_info_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &system_info_get_uri);

    httpd_uri_t raw_values_get_uri = {
        .uri = "/api/v1/dados",
        .method = HTTP_GET,
        .handler = raw_values_get_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &raw_values_get_uri);

    httpd_uri_t fft_get_uri = {
        .uri = "/api/v1/fft",
        .method = HTTP_GET,
        .handler = fft_get_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &fft_get_uri);
    
    httpd_uri_t common_get_uri = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = rest_common_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &common_get_uri);

    ESP_LOGI(TAG, "OK");

    return ESP_OK;

    err:
    if (rest_context) {
        free(rest_context);
    }
    return ret;
}
