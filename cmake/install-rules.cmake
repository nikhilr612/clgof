install(
    TARGETS clgof_exe
    RUNTIME COMPONENT clgof_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
