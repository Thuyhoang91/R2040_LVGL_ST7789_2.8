# Find all the .c files in this directory
file(GLOB_RECURSE SOURCES "*.c")
# Find all the .h files in this directory
file(GLOB_RECURSE HEADERS "*.h")


# Create a library target for the UI code
add_library(ui STATIC ${SOURCES})
# Create an alias for the library so it can be referenced as ui_lib
add_library(ui::ui ALIAS ui)

# Make the headers in this directory available publicly
target_include_directories(ui PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include/ui>
)

# Link the ui_lib with lvgl
target_link_libraries(ui PUBLIC lvgl)
