function(forge_apply_warnings target_name)
  if(MSVC)
    target_compile_options(${target_name} PRIVATE /W4 /permissive-)
    if(FORGE_WARNINGS_AS_ERRORS)
      target_compile_options(${target_name} PRIVATE /WX)
    endif()
  else()
    target_compile_options(${target_name} PRIVATE
      -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow
      -Wold-style-cast -Wnon-virtual-dtor -Woverloaded-virtual
    )
    if(FORGE_WARNINGS_AS_ERRORS)
      target_compile_options(${target_name} PRIVATE -Werror)
    endif()
  endif()
endfunction()
