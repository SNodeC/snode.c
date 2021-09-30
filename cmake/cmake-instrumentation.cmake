option(CXX_INSTRUMENT_METHODS "Instrument methods and functions")
mark_as_advanced(FORCE CXX_INSTRUMENT_METHODS)

if(CXX_INSTRUMENT_METHODS)
    add_compile_options(-finstrument-functions)
endif(CXX_INSTRUMENT_METHODS)
