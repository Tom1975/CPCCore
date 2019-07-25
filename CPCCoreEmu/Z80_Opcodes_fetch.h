
Begin:

   if (((current_opcode_ & 0xFF00) == 0xFD00)
      || ((current_opcode_ & 0xFF00) == 0xDD00))
   {
      current_opcode_ &= 0xFF;
      goto Begin;
   }
   else
   {
      if ((current_opcode_ & 0xFF00) != 0xED00)
      {
         // Assert !
      }
      /*#ifdef _WIN32
               _CrtDbgBreak();
      #endif*/
   }

   NEXT_INSTR;
