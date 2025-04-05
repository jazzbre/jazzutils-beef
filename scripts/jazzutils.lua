project "jazzutils"
   kind "StaticLib"
   windowstargetplatformversion("10.0")

   configuration { "VS*" }
      buildoptions { "/std:c++14" }

   flags {
      "Cpp14",
   }

   defines {
      "MCO_API",
      "MINIZ_NO_TIME",
   }

   includedirs {
      path.join(SOURCE_DIR, "miniz_export"),
      path.join(SOURCE_DIR, "sandbox/include"),
   }
      
   files {
      path.join(SOURCE_DIR, "../csrc/*.*"),
      path.join(SOURCE_DIR, "sandbox/testbsp/bsp*.*"),
      path.join(SOURCE_DIR, "sandbox/testbsp/face*.*"),
      path.join(SOURCE_DIR, "gifenc/gifenc.*"),
      path.join(SOURCE_DIR, "miniz/miniz*.c"),
      path.join(SOURCE_DIR, "miniz/miniz*.h"),
      path.join(SOURCE_DIR, "enkiTS/src/*.cpp"),
      path.join(SOURCE_DIR, "enkiTS/src/*.h"),
   }

   configuration {}  
