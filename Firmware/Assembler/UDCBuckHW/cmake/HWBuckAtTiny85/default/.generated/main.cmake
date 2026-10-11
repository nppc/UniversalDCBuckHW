include("${CMAKE_CURRENT_LIST_DIR}/rule.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/file.cmake")

set(HWBuckAtTiny85_default_library_list )

# Handle files with suffix (asm|ASM|Asm), for group default-AVR-ASM2
if(HWBuckAtTiny85_default_default_AVR_ASM2_FILE_TYPE_assemble)
add_custom_target(HWBuckAtTiny85_default_default_AVR_ASM2_assemble ALL)
    HWBuckAtTiny85_default_default_AVR_ASM2_assemble_rule(HWBuckAtTiny85_default_default_AVR_ASM2_assemble)
    list(APPEND HWBuckAtTiny85_default_library_list HWBuckAtTiny85_default_default_AVR_ASM2_assemble)
endif()


# Custom build (no link stage)
add_custom_target(
    HWBuckAtTiny85_default_image_Xu_e3k8E ALL
    DEPENDS ${HWBuckAtTiny85_default_library_list}
    )

set_target_properties(HWBuckAtTiny85_default_image_Xu_e3k8E PROPERTIES
    OUTPUT_NAME "default"
    SUFFIX ".hex"
    RUNTIME_OUTPUT_DIRECTORY "${HWBuckAtTiny85_default_output_dir}")



