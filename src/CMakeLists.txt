

add_library(dynaudnorm MODULE DynamicAudioNormalizer.cpp  dynaudnorm.cpp  FrameBuffer.cpp  GaussianFilter.cpp  Logging.cpp  Version.cpp)

set_target_properties(dynaudnorm PROPERTIES LINK_FLAGS "-nostartfiles")
install(TARGETS dynaudnorm  LIBRARY DESTINATION lib/ladspa)

