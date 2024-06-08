idf_component_register(SRCS "app_main.c" "rest_server.c" "wifi.c"
                    INCLUDE_DIRS ".")

set(WEB_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../front/web-provision")
if(EXISTS ${WEB_SRC_DIR}/dist)
    file(DOWNLOAD
        https://unpkg.com/petite-vue
        ${WEB_SRC_DIR}/dist/petite-vue.min.js
        EXPECTED_HASH MD5=a7db3244c9a6704a3159a38c82207f66
    )
    file(DOWNLOAD
        https://unpkg.com/axios/dist/axios.min.js
        ${WEB_SRC_DIR}/dist/axios.min.js
        EXPECTED_HASH MD5=3b5b3d36fde8ffe8ed76b1efbfc65410
    )
    file(DOWNLOAD
        https://cdn.jsdelivr.net/npm/purecss@3.0.0/build/pure-min.css
        ${WEB_SRC_DIR}/dist/pure-min.css
        EXPECTED_HASH MD5=b249b72c296243049da303cfb44e409b
    )
    spiffs_create_partition_image(www ${WEB_SRC_DIR}/dist FLASH_IN_PROJECT)
else()
    message(FATAL_ERROR "${WEB_SRC_DIR}/dist doesn't exit. Please run 'npm run build' in ${WEB_SRC_DIR}")
endif()
