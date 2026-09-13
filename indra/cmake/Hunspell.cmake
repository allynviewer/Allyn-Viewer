# -*- cmake -*-
include(Prebuilt)

if (STANDALONE)
	include(FindHunSpell)
else (STANDALONE)
  use_prebuilt_binary(libhunspell)

  set(HUNSPELL_LIBRARY libhunspell)
  add_compile_definitions(HUNSPELL_STATIC)
  set(HUNSPELL_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include/hunspell)
  use_prebuilt_binary(dictionaries)
endif (STANDALONE)
