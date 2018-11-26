add_executable(dev-factory main.cc)
target_link_libraries(dev-factory aktualizr_static_lib ${AKTUALIZR_EXTERNAL_LIBS})

aktualizr_source_file_checks(main.cc)

install(TARGETS dev-factory RUNTIME DESTINATION bin COMPONENT dev-factory)

# vim: set tabstop=4 shiftwidth=4 expandtab:
