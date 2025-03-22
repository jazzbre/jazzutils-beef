project "jazzutils"
   kind "StaticLib"
   windowstargetplatformversion("10.0")

   defines {
      "MCO_API",
      "MINIZ_NO_TIME",
   }

   includedirs {
      path.join(SOURCE_DIR, "miniz_export"),
   }
      
   files {
      path.join(SOURCE_DIR, "gifenc/gifenc.*"),
      path.join(SOURCE_DIR, "miniz/miniz*.c"),
      path.join(SOURCE_DIR, "miniz/miniz*.h"),
      path.join(SOURCE_DIR, "enkiTS/src/*.cpp"),
      path.join(SOURCE_DIR, "enkiTS/src/*.h"),
   }

   configuration {}  
