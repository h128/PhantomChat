macro(phantomchat_configure_linker project_name)
  set(phantomchat_USER_LINKER_OPTION
    "DEFAULT"
      CACHE STRING "Linker to be used")
    set(phantomchat_USER_LINKER_OPTION_VALUES "DEFAULT" "SYSTEM" "LLD" "GOLD" "BFD" "MOLD" "SOLD" "APPLE_CLASSIC" "MSVC")
  set_property(CACHE phantomchat_USER_LINKER_OPTION PROPERTY STRINGS ${phantomchat_USER_LINKER_OPTION_VALUES})
  list(
    FIND
    phantomchat_USER_LINKER_OPTION_VALUES
    ${phantomchat_USER_LINKER_OPTION}
    phantomchat_USER_LINKER_OPTION_INDEX)

  if(${phantomchat_USER_LINKER_OPTION_INDEX} EQUAL -1)
    message(
      STATUS
        "Using custom linker: '${phantomchat_USER_LINKER_OPTION}', explicitly supported entries are ${phantomchat_USER_LINKER_OPTION_VALUES}")
  endif()

  set_target_properties(${project_name} PROPERTIES LINKER_TYPE "${phantomchat_USER_LINKER_OPTION}")
endmacro()
