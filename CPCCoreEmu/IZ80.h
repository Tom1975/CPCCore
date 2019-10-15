#pragma once


class IZ80
{
public:
   virtual ~IZ80() {}

   virtual void ReinitProc () = 0;
   virtual unsigned int GetCurrentOpcode () = 0;
   virtual unsigned char GetOpcodeSize (unsigned short addr){return 0;};
   virtual void TurnLogOn (bool on){};

   virtual unsigned short GetPC() { return pc_; };

   virtual void Copy ( IZ80* source)
   {
      source->af_.w = af_.w;
      source->bc_.w = bc_.w;
      source->de_.w = de_.w;
      source->hl_.w = hl_.w;
      source->af_p_.w = af_p_.w;
      source->bc_p_.w = bc_p_.w;
      source->de_p_.w = de_p_.w;
      source->hl_p_.w = hl_p_.w;
      source->ix_.w = ix_.w;
      source->iy_.w = iy_.w;
      source->ir_.b.h = ir_.b.h;
      source->ir_.b.l = ir_.b.l;
      source->pc_ = pc_;
      source->sp_ = sp_;
      source->q_  = q_;
      source->iff1_ = iff1_ ;
      source->iff2_ = iff2_;
      source->mem_ptr_.w = mem_ptr_.w;

   };

   virtual bool Compare(IZ80* source)
   {
      // Copy what's needed to this
      if (source->pc_ == pc_)
      {
         if (
            source->af_.w != af_.w
         || source->bc_.w != bc_.w
         || source->de_.w != de_.w
         || source->hl_.w != hl_.w
         || source->af_p_.w != af_p_.w
         || source->bc_p_.w != bc_p_.w
         || source->de_p_.w != de_p_.w
         || source->hl_p_.w != hl_p_.w
         || source->ix_.w != ix_.w
         || source->iy_.w != iy_.w
         || source->ir_.b.h != ir_.b.h
         || source->ir_.b.l != ir_.b.l
         || source->sp_ != sp_
         || source->q_ != q_
         || source->iff1_ != iff1_ 
         || source->iff2_ != iff2_
         || source->mem_ptr_.w != mem_ptr_.w
         )
            return false;
      }
      else
      {
         if (pc_ - source->pc_ > 1 )   
            return false;
      }
      return true;
   }

   /////////////////////////////////////////////////
   // Z80 datas
   /////////////////////////////////////////////////

   // Registres
   union Register
   {
      struct {
         unsigned char l;
         unsigned char h;
      } b;
      unsigned short w;
   };

   Register af_;
   Register bc_;
   Register de_;
   Register hl_;

   Register af_p_;
   Register bc_p_;
   Register de_p_;
   Register hl_p_;

   Register ix_;    // IX
   Register iy_;    // IY

   Register ir_;

   unsigned short sp_;
   unsigned short pc_;

   bool iff1_;
   bool iff2_;

   unsigned char q_;
   Register mem_ptr_;

   // Mode d'interruption
   char interrupt_mode_;

};

