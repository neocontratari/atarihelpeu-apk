void *retro_get_memory_data(unsigned id)
{
   switch (id)
   {
   case RETRO_MEMORY_SYSTEM_RAM:
      return psxRegs.ptrs.psxM;
   case RETRO_MEMORY_SAVE_RAM:
      if (memcard_type[0] == MEMCARDTYPE_LIBRETRO)
         return Mcd1Data;
   }
   return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
   switch (id)
   {
   case RETRO_MEMORY_SYSTEM_RAM:
      return 0x200000;
   case RETRO_MEMORY_SAVE_RAM:
      if (memcard_type[0] == MEMCARDTYPE_LIBRETRO)
         return MCD_SIZE;
   }
   return 0;
}
