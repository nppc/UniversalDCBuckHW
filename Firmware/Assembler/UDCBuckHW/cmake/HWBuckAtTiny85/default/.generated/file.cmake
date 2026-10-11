# The following variables contains the files used by the different stages of the build process.
set(HWBuckAtTiny85_default_default_AVR_ASM2_FILE_TYPE_assemble "${CMAKE_CURRENT_SOURCE_DIR}/../../../main.asm")
set_source_files_properties(${HWBuckAtTiny85_default_default_AVR_ASM2_FILE_TYPE_assemble} PROPERTIES LANGUAGE ASM)

# For assembly files, add "." to the include path for each file so that .include with a relative path works
foreach(source_file ${HWBuckAtTiny85_default_default_AVR_ASM2_FILE_TYPE_assemble})
        set_source_files_properties(${source_file} PROPERTIES INCLUDE_DIRECTORIES "$<PATH:NORMAL_PATH,$<PATH:REMOVE_FILENAME,${source_file}>>")
endforeach()

set(HWBuckAtTiny85_default_image_name "default.hex")
set(HWBuckAtTiny85_default_image_base_name "default")

# The output directory of the final image.
set(HWBuckAtTiny85_default_output_dir "${CMAKE_CURRENT_SOURCE_DIR}/../../../out/HWBuckAtTiny85")

# The full path to the final image.
set(HWBuckAtTiny85_default_full_path_to_image ${HWBuckAtTiny85_default_output_dir}/${HWBuckAtTiny85_default_image_name})
