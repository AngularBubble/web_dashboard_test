idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS ".")

littlefs_create_partition_image(web_data ../main/data FLASH_IN_PROJECT)

set(WEB_SRC_DIR "../main/web_page")
if(EXISTS ${WEB_SRC_DIR}/dist)
    littlefs_create_partition_image(www ${WEB_SRC_DIR}/dist FLASH_IN_PROJECT)
else()
    message(FATAL_ERROR "'${WEB_SRC_DIR}/dist' doesn't exist. Please run 'pnpm build' under '${WEB_SRC_DIR}'")
endif()