macro(PhantomChat_configure_linker project_name)
  set(PhantomChat_USER_LINKER_OPTION
    "DEFAULT"
      CACHE STRING "Linker to be used")
    set(PhantomChat_USER_LINKER_OPTION_VALUES "DEFAULT" "SYSTEM" "LLD" "GOLD" "BFD" "MOLD" "SOLD" "APPLE_CLASSIC" "MSVC")
  set_property(CACHE PhantomChat_USER_LINKER_OPTION PROPERTY STRINGS ${PhantomChat_USER_LINKER_OPTION_VALUES})
  list(
    FIND
    PhantomChat_USER_LINKER_OPTION_VALUES
    ${PhantomChat_USER_LINKER_OPTION}
    PhantomChat_USER_LINKER_OPTION_INDEX)

  if(${PhantomChat_USER_LINKER_OPTION_INDEX} EQUAL -1)
    message(
      STATUS
        "Using custom linker: '${PhantomChat_USER_LINKER_OPTION}', explicitly supported entries are ${PhantomChat_USER_LINKER_OPTION_VALUES}")
  endif()

  set_target_properties(${project_name} PROPERTIES LINKER_TYPE "${PhantomChat_USER_LINKER_OPTION}")
endmacro()
