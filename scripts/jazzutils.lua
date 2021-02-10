project "jazzutils"
   kind "StaticLib"
   windowstargetplatformversion("10.0")

   defines {
      MCO_API
   }

   includedirs {
   }
      
   files {
      path.join(SOURCE_DIR, "gifenc/gifenc.*"),
   }

   configuration {}  
