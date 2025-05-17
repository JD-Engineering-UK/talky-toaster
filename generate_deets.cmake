# find_package(Python3)

function(generate_deets target_name)
    if(PYTHON_FOUND)
        add_custom_target(
            TARGET ${target_name}
            PRE_BUILD
            COMMAND
                ${PYTHON_EXECUTABLE} "gen_wifi_deets.py"
            COMMENT "Generate packed WiFi details"
        )
    endif()
endfunction()