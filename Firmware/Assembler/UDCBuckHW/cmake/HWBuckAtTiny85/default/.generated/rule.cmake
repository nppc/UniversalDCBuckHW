# The following functions contains all the flags passed to the different build stages.

set(PACK_REPO_PATH "C:/Users/Pavel/.mchp_packs" CACHE PATH "Path to the root of a pack repository.")

function(HWBuckAtTiny85_default_default_AVR_ASM2_assemble_rule target)
add_custom_command(
    OUTPUT ${HWBuckAtTiny85_default_output_dir}/${HWBuckAtTiny85_default_image_base_name}.hex
    COMMAND "${MP_AS}" -fI -W+ie -m mem.map -d ${HWBuckAtTiny85_default_image_base_name}.obj -l ${HWBuckAtTiny85_default_image_base_name}.lss -S ${HWBuckAtTiny85_default_image_base_name}.tmp ${HWBuckAtTiny85_default_default_AVR_ASM2_FILE_TYPE_assemble} -o "${HWBuckAtTiny85_default_output_dir}/${HWBuckAtTiny85_default_image_base_name}.hex"
    WORKING_DIRECTORY ${HWBuckAtTiny85_default_output_dir})
add_custom_target(
    ${target}_build ALL
    DEPENDS ${HWBuckAtTiny85_default_output_dir}/${HWBuckAtTiny85_default_image_base_name}.hex
    COMMENT "Assembling into ${HWBuckAtTiny85_default_output_dir}/${HWBuckAtTiny85_default_image_base_name}.hex")
endfunction()
