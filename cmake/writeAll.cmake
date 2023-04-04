# function to build an "all.hpp" header that includes all the headers from a library
#
# Inputs:
#   dir - the base directory name, e.g. "qle"
#   output - the output file name, e.g. "quantext.hpp"
#   autoLink - the name of the auto link file, that only gets included on windows
#   headers - list of all headers, relative to ${dir}
function(writeAll dir output autoLink headers)
  list(REMOVE_ITEM headers "${output}")
  list(REMOVE_ITEM headers "${autoLink}")
  list(SORT headers)
  file(WRITE  ${output} "// Autogenerated by cmake\n")
  file(APPEND ${output} "// Do not edit\n")
  file(APPEND ${output} "\n")
  file(APPEND ${output} "#ifdef BOOST_MSVC\n")
  file(APPEND ${output} "#include <${dir}/${autoLink}>\n")
  file(APPEND ${output} "#endif\n")
  file(APPEND ${output} "\n")
  foreach(file ${headers})
    file(APPEND ${output} "#include <${dir}/${file}>\n")
    get_filename_component(extension ${file} EXT)
    if(NOT "${extension}" STREQUAL ".hpp" AND NOT "${extension}" STREQUAL ".h")
      message(FATAL_ERROR "${file} does not end on hpp or h, this can not be written to ${output}")
    endif()
  endforeach(file)
endfunction()
