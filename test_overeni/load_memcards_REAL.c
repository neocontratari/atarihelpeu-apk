static void load_memcards(void)
{
   struct retro_variable var = { NULL, };
   const char *dir = NULL;
   char buf[128];
   int c;

   if (!environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &dir) || !dir)
      LogErr("Could not get save directory! Memory card saving might not work.");
   else if (strlen(dir) + strlen("XXXX-00000_1.mcd") + 2 > sizeof(Config.Mcd1)) {
      LogErr("Path '%s' is too long. Memory card saving might not work.", dir);
      dir = NULL;
   }

   for (c = 1; c <= 2; c++) {
      char *mcdpath = (c == 1) ? Config.Mcd1 : Config.Mcd2;
      snprintf(buf, sizeof(buf), "pcsx_rearmed_memcard%d", c);
      var.key = buf;
      if (!environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) || !var.value) {
         LogErr("non memcard%d config?", c);
         continue;
      }
      if (!strcmp(var.value, "libretro")) {
         memcard_type[c - 1] = MEMCARDTYPE_LIBRETRO;
         mcdpath[0] = 0;
         SysPrintf("memcard %d is libretro-managed\n", c);
      }
      else if (!strcmp(var.value, "serial") && dir && CdromId[0]) {
         memcard_type[c - 1] = MEMCARDTYPE_SERIAL;
         get_dash_serial(buf, sizeof(buf));
         snprintf(mcdpath, sizeof(Config.Mcd1), "%s/%s_%d.mcd", dir, buf, c);
      }
      else if (!strcmp(var.value, "shared") && dir) {
         memcard_type[c - 1] = MEMCARDTYPE_SHARED;
         snprintf(mcdpath, sizeof(Config.Mcd1), "%s/pcsx-card%d.mcd", dir, c);
      }
      else {
         memcard_type[c - 1] = MEMCARDTYPE_NONE;
         snprintf(mcdpath, sizeof(Config.Mcd1), "none");
         SysPrintf("memcard %d is disabled\n", c);
      }
      LoadMcd(c, mcdpath);
   }
}
