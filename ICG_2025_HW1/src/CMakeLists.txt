add_executable(ICG_2025_HW1
"main.cpp"
"std_image.cpp"
) #列所有的cpp

target_link_libraries(ICG_2025_HW1
glfw
glm::glm
glad
tinyobjloader
)

# Copy files after build
add_custom_command(TARGET ICG_2025_HW1 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders ${CMAKE_CURRENT_BINARY_DIR}/shaders)

add_custom_command(TARGET ICG_2025_HW1 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory  
    ${CMAKE_CURRENT_SOURCE_DIR}/asset ${CMAKE_CURRENT_BINARY_DIR}/asset)