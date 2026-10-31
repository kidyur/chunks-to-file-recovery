add_library(file_collector file_collector.cpp)
target_link_libraries(file_collector
    PRIVATE
        ordseg
)