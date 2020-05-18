#include "stdafx.h"
#include "Motherboard.h"

Motherboard::Motherboard(SoundMixer* sound_mixer, IKeyboardHandler* keyboard_handler):
   generic_breakpoint_(nullptr),
   supervisor_(nullptr),
   plus_(false),
   address_bus_(16), data_bus_(8),
   memory_(&monitor_), 
   keyboardhandler_(keyboard_handler),
   psg_(sound_mixer, keyboardhandler_),
   netlist_int_(&signals_), 
   netlist_nmi_(&signals_.nmi_),
   play_city_(&netlist_int_, &netlist_nmi_, sound_mixer),
   cursor_line_(&play_city_)
{
   breakpoint_index_ = 0;
   memset(breakpoint_list_, 0, sizeof(breakpoint_list_));
}

Motherboard::~Motherboard()
{

}

void Motherboard::InitMotherbard(ILog *log, IPlayback * sna_handler, IDisplay* display, IFdcNotify* notifier, IDirectories * directories, IConfiguration * configuration_manager)
{
   // Initialisation des donnes interne
   z80_.Init(&memory_, &signals_, log);

   signals_.address_bus_ = &address_bus_;
   signals_.data_bus_ = &data_bus_;
   signals_.ppi_ = &ppi_;
   signals_.fdc_ = &fdc_;
   signals_.crtc_ = &crtc_;
   signals_.vga_ = &vga_;
   signals_.z80_ = &z80_;
   signals_.memory_ = &memory_;
   signals_.asic_ = &asic_;

   // CRTC
   //crtc_.SetBus ( &address_bus_, &data_bus_ );
   crtc_.SetSig(&signals_);
   crtc_.SetGateArray(&vga_);
   crtc_.SetPPI(&ppi_);
   crtc_.SetPlayback(sna_handler);

   // Gate array
   monitor_.SetCRTC(&crtc_);
   monitor_.SetVGA(&vga_);
   monitor_.SetScreen(display);
   monitor_.SetPlayback(sna_handler);


   vga_.SetMonitor(&monitor_);
   vga_.SetDMA(dma_);

   dma_[0].Init(0, &memory_, &memory_, memory_.GetDMARegister(0), &psg_, &signals_);
   dma_[1].Init(1, &memory_, &memory_, memory_.GetDMARegister(1), &psg_, &signals_);
   dma_[2].Init(2, &memory_, &memory_, memory_.GetDMARegister(2), &psg_, &signals_);

   vga_.SetCRTC(&crtc_);
   vga_.SetBus(&address_bus_, &data_bus_);
   vga_.SetSig(&signals_);
   vga_.SetMemory(&memory_);
   //vga_.SetScreen ( Display_P );

   asic_.Init(&vga_, &crtc_, log);

   // Printer
   printer_ = &default_printer_;
   signals_.printer_port_ = printer_;


   // FDC ?

   fdc_.SetLog(log);
   fdc_.Init(&disk_type_manager_);
   fdc_.SetNotifier(notifier);
   tape_.SetLog(log);

   tape_.SetVGA(&vga_);
   crtc_.SetLog(log);

   // PPI et cfg standard
      // Type :
   typedef enum
   {
      ISP = 0x0,
      TRIUMPH = 0x2,
      SAIPHO = 0x4,
      SOLAVOX = 0x6,
      AWA = 0x8,
      SCHNEIDER = 0xA,
      ORION = 0xC,
      AMSTRAD = 0xE,
   } Amstrad_Type;

   ppi_.SetAmstradType(AMSTRAD);
   ppi_.SetPsg(&psg_);
   ppi_.SetSig(&signals_);
   ppi_.SetTape(&tape_);
   tape_.Init(directories, &ppi_, configuration_manager);

   psg_.SetDirectories(directories);
   psg_.SetLog(log);
   //keyboardhandler_.SetDirectories(directories);
//   keyboardhandler_.SetConfigurationManager(configuration_manager);
//   keyboardhandler_.LoadKeyboardMap("FRENCH");

   //UpdateComputer ();

   // Expansion
   multiface2_.Init(&signals_);

   crtc_.SetCursorLine(&cursor_line_);

   // Plug it to the signal
   UpdateExternalDevices();

   if (plus_)
   {
      InitStartOptimizedPlus();
   }
   else
   {
      InitStartOptimized();
   }
}

void Motherboard::OnOff()
{
   asic_.HardReset();

   memory_.Initialisation();
   memory_.SetRmr2(0);

   psg_.Reset();
   signals_.Reset();

   for (int i = 0; i < signals_.nb_expansion_; i++)
   {
      signals_.exp_list_[i]->Reset();
   }
}

void Motherboard::SetPlus(bool plus)
{
   plus_ = plus;

   // Set component to "plus"
   signals_.SetPlus(plus_);
   ppi_.SetPlus(plus_);
   vga_.SetPlus(plus_);
   memory_.SetPlus(plus_);
}


void Motherboard::UpdateExternalDevices()
{
   signals_.nb_expansion_ = 0;
   /*
   if (current_settings_->GetMultiface2())
   {
      m_Sig.m_ExpList[m_Sig.m_NbExpansionPlugged++] = &multiface2_;
   }
   if (current_settings_->GetPlayCity())
   {
      m_Sig.m_ExpList[m_Sig.m_NbExpansionPlugged++] = play_city;
   }*/
   //GetSig()->exp_list_[GetSig()->nb_expansion_++] = &play_city_;
}

void Motherboard::InitStartOptimized()
{
   // Set the components call file
   // Get from settings, all components
   // Place them in the file
   memset(component_elapsed_time_, 0, sizeof(component_elapsed_time_));
   nb_components_ = 0;

   // PSG
   component_elapsed_time_[nb_components_] = 15;
   component_list_[nb_components_] = &psg_;
   component_list_[nb_components_]->this_tick_time_ = 15;
   nb_components_++;

   // CPU : Z80
   z80_index_ = nb_components_;
   component_elapsed_time_[nb_components_] = 0;
   component_list_[nb_components_] = &z80_;
   component_list_[nb_components_]->this_tick_time_ = 0;
   component_list_[nb_components_]->elapsed_time_ = component_list_[nb_components_]->this_tick_time_;
   nb_components_++;
   
   // Tape ?
   component_elapsed_time_[nb_components_] = 1;
   component_list_[nb_components_] = &tape_;
   component_list_[nb_components_]->this_tick_time_ = 1;
   nb_components_++;
   
   // CRTC
   component_elapsed_time_[nb_components_] = 11;
   component_list_[nb_components_] = &crtc_;
   component_list_[nb_components_]->this_tick_time_ = 11;
   nb_components_++;

   // FDC ?
   component_elapsed_time_[nb_components_] = 8;
   component_list_[nb_components_] = &fdc_;
   component_list_[nb_components_]->this_tick_time_ = 8;

   // SET sur 6128 ou si DDI ( dependant de la présenc du FDC, donc ?)
   ppi_.SetExpSignal(true);

   nb_components_++;

   for (int j = 0; j < nb_components_; j++)
   {
      if (component_list_[j] != NULL)
      {
         component_list_[j]->SetTickForcer(this);
      }
   }
}

void Motherboard::InitStartOptimizedPlus()
{
   // Set the components call file
   // Get from settings, all components
   // Place them in the file
   memset(component_elapsed_time_, 0, sizeof(component_elapsed_time_));
   nb_components_ = 0;

   // PSG
   component_elapsed_time_[nb_components_] = 15;
   component_list_[nb_components_] = &psg_;
   component_list_[nb_components_]->this_tick_time_ = 15;
   nb_components_++;

   // CPU : Z80
   z80_index_ = nb_components_;
   component_elapsed_time_[nb_components_] = 0;
   component_list_[nb_components_] = &z80_;
   component_list_[nb_components_]->this_tick_time_ = 0;
   component_list_[nb_components_]->elapsed_time_ = component_list_[nb_components_]->this_tick_time_;
   nb_components_++;

   // Tape ?
   component_elapsed_time_[nb_components_] = 1;
   component_list_[nb_components_] = &tape_;
   component_list_[nb_components_]->this_tick_time_ = 1;
   nb_components_++;

   // FDC ?
   component_elapsed_time_[nb_components_] = 8;
   component_list_[nb_components_] = &fdc_;
   component_list_[nb_components_]->this_tick_time_ = 8;

   // ASIC
   component_elapsed_time_[nb_components_] = 11;
   component_list_[nb_components_] = &asic_;
   component_list_[nb_components_]->this_tick_time_ = 11;
   nb_components_++;

   // SET sur 6128 ou si DDI ( dependant de la présenc du FDC, donc ?)
   ppi_.SetExpSignal(true);

   for (int j = 0; j < nb_components_; j++)
   {
      if (component_list_[j] != NULL)
      {
         component_list_[j]->SetTickForcer(this);
      }
   }

   //
   // Create dynamic function :
   // - https://blogs.oracle.com/nike/entry/simple_jit_compiler_for_your
   // 1- Compute how many cycle we want to do in a single loop :
   //     - It has to be a multiple of the PPCM of the whole components - call this X time
   // 2 - Create the code
   /*if (generated_loop_.y != NULL)
   {
      VirtualFreeEx(GetCurrentProcess(), generated_loop_.y, 0, MEM_RELEASE);
   }

   byte* buf = (byte*)VirtualAllocEx(GetCurrentProcess(), 0, 1 << 16, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

   if (buf == 0) return;

   byte* p = buf;


   *p++ = 0xC3; // ret


   generated_loop_.y = buf;*/
}

void Motherboard::Resync()
{
   // Set the components call file
   // Get from settings, all components
   // Place them in the file

   // PSG
   ForceTick(&psg_, 0);

   // z80
   ForceTick(&z80_, 0);

   // CRTC§Asic
   if (plus_)
   {
      ForceTick(&asic_, 11);

      ForceTick(&dma_[0], 11);
      ForceTick(&dma_[1], 11);
      ForceTick(&dma_[2], 11);
   }
   else
   {
      ForceTick(&crtc_, 8);

   }

   // Tape ?
   ForceTick(&tape_, 1);

   // FDC ?
   ForceTick(&fdc_, 8);

   // Gate Array
   ForceTick(&vga_, 2);

}

void Motherboard::ForceTick(IComponent* component, int ticks)
{
   // Look for this item
   for (int i = 0; i < nb_components_; i++)
   {
      if (component_list_[i] == component)
      {
         // Found ? Just set the "current" value to 0
         if (ticks == 0)
         {
            component_list_[i]->this_tick_time_ -= (component_elapsed_time_[i] - 1);
            component_elapsed_time_[i] = 1;
         }
         else
         {
            component_list_[i]->this_tick_time_ = ticks;
            component_elapsed_time_[i] = ticks;
         }

         return;
      }
   }

}

// New
int Motherboard::DebugOpcodes( unsigned int& nb_opcodes )
{
   unsigned int next_cycle;
   unsigned int index = 0;
   unsigned int elapsed_time_psg = next_cycle = component_elapsed_time_[index++];
   unsigned int elapsed_time_z80 = component_elapsed_time_[index++];
   unsigned int elapsed_time_tape = component_elapsed_time_[index++];
   unsigned int elapsed_time_crtc = component_elapsed_time_[index++];
   unsigned int elapsed_time_fdc = component_elapsed_time_[index++];

   unsigned int elapsed_components[16];
   for (int i = 0; i < signals_.nb_expansion_; i++)
   {
      elapsed_components[i] = component_elapsed_time_[index++];
   }

   unsigned int* elapsed = component_elapsed_time_;
   for (unsigned int i = 0; i < index; ++i)
   {
      if (*elapsed < next_cycle)
      {
         next_cycle = *elapsed;
      }
      ++elapsed;
   }

   run_ = true;

   next_cycle = 0;

   int old_counter = next_cycle;
   while (run_ && nb_opcodes > 0)
   {
      if (elapsed_time_psg == next_cycle)
      {
         elapsed_time_psg += (psg_).Tick();
      }
      RUN_COMPOSANT_N(tape_, elapsed_time_tape);
      RUN_COMPOSANT_N(crtc_, elapsed_time_crtc);
      RUN_COMPOSANT_N(fdc_, elapsed_time_fdc);
      RUN_COMPOSANT_N(z80_, elapsed_time_z80);

      for (int i = 0; i < signals_.nb_expansion_; i++)
      {
         if (elapsed_components[i] <= next_cycle) elapsed_components[i] += signals_.exp_list_[i]->Tick();
      }

      signals_.Propagate();
      ++next_cycle;

      if ((z80_.t_ == 1 &&
         (z80_.machine_cycle_ == Z80::M_M1_NMI
            || z80_.machine_cycle_ == Z80::M_M1_INT)
         )
         || (z80_.machine_cycle_ == Z80::M_FETCH
            && z80_.t_ == 4
            && ((z80_.current_opcode_ & 0xFF00) == 0)
            && elapsed_time_z80 == next_cycle
            )
         )
      {
         counter_ += (component_elapsed_time_[z80_index_] - old_counter + 1);
         old_counter = component_elapsed_time_[z80_index_];
         {
            nb_opcodes--;
            if (generic_breakpoint_)
            {
               run_ = generic_breakpoint_->IsBreak() ? false : true;
               memory_.ResetStockAddress();
            }
            for (unsigned int i = 0; i < breakpoint_index_; i++)
            {
               if (breakpoint_list_[i] == z80_.GetPC())
               {
                  // Break !
                  run_ = false;
               }
            }
         }
      }
   }

   index = 0;
   component_elapsed_time_[index++] = elapsed_time_psg - next_cycle;
   component_elapsed_time_[index++] = elapsed_time_z80 - next_cycle;
   component_elapsed_time_[index++] = elapsed_time_tape - next_cycle;
   component_elapsed_time_[index++] = elapsed_time_crtc - next_cycle;
   component_elapsed_time_[index++] = elapsed_time_fdc - next_cycle;

   for (int i = 0; i < signals_.nb_expansion_; i++)
   {
      component_elapsed_time_[index++] = elapsed_components[i] - next_cycle;
   }

   if (run_ &&  step_)
   {
      remember_step_ = true;
   }

   if (supervisor_ != NULL)
   {
      if (!run_) supervisor_->EmulationStopped();
      if (counter_ >= 4000000)
      {
         supervisor_->SetEfficience(GetSpeed());
         supervisor_->RefreshRunningData();
         // Refresh ?
      }
   }
   return next_cycle;
}

int Motherboard::DebugNew(unsigned int nb_cycles)
{
   unsigned int next_cycle = nb_cycles;
   unsigned int index = 0;
   unsigned int elapsed_time_psg = component_elapsed_time_[index++];
   unsigned int elapsed_time_z80 = component_elapsed_time_[index++];
   unsigned int elapsed_time_tape = component_elapsed_time_[index++];
   unsigned int elapsed_time_crtc = component_elapsed_time_[index++];
   unsigned int elapsed_time_fdc = component_elapsed_time_[index++];

   unsigned int elapsed_components[16];
   for (int i = 0; i < signals_.nb_expansion_; i++)
   {
      elapsed_components[i] = component_elapsed_time_[index++];
   }

   unsigned int* elapsed = component_elapsed_time_;
   for (unsigned int i = 0; i < index; ++i)
   {
      if (*elapsed < next_cycle)
      {
         next_cycle = *elapsed;
      }
      ++elapsed;
   }

   // Verifier que c'est un call, sinon, comme stepin
   if (step_)
   {
      // Compute next PC to stop
      if (z80_.IsCallInstruction(z80_.GetPC()))
      {
         stop_pc_ = z80_.GetPC() + z80_.GetOpcodeSize(z80_.GetPC());
      }
      else
      {
         if (!remember_step_)
         {
            // Step in !
            step_ = false;
            step_in_ = true;
         }
      }
   }
   run_ = true;

   next_cycle = 0;

   int old_counter = next_cycle;

   while (run_ && next_cycle < nb_cycles)
   {
      if (elapsed_time_psg == next_cycle)
      {
         elapsed_time_psg += (psg_).Tick();
      }
      RUN_COMPOSANT_N(tape_, elapsed_time_tape);
      RUN_COMPOSANT_N(crtc_, elapsed_time_crtc);
      RUN_COMPOSANT_N(fdc_, elapsed_time_fdc);
      RUN_COMPOSANT_N(z80_, elapsed_time_z80);

      for (int i = 0; i < signals_.nb_expansion_; i++)
      {
         if (elapsed_components[i] <= next_cycle) elapsed_components[i] += signals_.exp_list_[i]->Tick();
      }

      signals_.Propagate();
      ++next_cycle;

      if (  (  z80_.t_ == 1 &&
               (z80_.machine_cycle_ == Z80::M_M1_NMI
               || z80_.machine_cycle_ == Z80::M_M1_INT)
             )
          || (  z80_.machine_cycle_ == Z80::M_FETCH 
             && z80_.t_ == 4 
             && ((z80_.current_opcode_ & 0xFF00) == 0) 
             && elapsed_time_z80 == next_cycle
             )
         )
      {
         counter_ += (component_elapsed_time_[z80_index_] - old_counter + 1);
         old_counter = component_elapsed_time_[z80_index_];
         {
            if (generic_breakpoint_)
            {
               run_ = generic_breakpoint_->IsBreak() ? false : true;
               memory_.ResetStockAddress();
            }
            for (unsigned int i = 0; i < breakpoint_index_; i++)
            {
               if (breakpoint_list_[i] == z80_.GetPC())
               {
                  // Break !
                  run_ = false;
               }
            }
            if (step_)
            {
               if (z80_.GetPC() == stop_pc_)
               {
                  step_ = false;
                  run_ = false;
                  remember_step_ = false;
               }
            }
            if (step_in_)
            {
               step_in_ = false;
               run_ = false;
            }
         }
      }
   }
   
   index = 0;
   component_elapsed_time_[index++] = elapsed_time_psg - next_cycle;
   component_elapsed_time_[index++] = elapsed_time_z80 - next_cycle;
   component_elapsed_time_[index++] = elapsed_time_tape - next_cycle;
   component_elapsed_time_[index++] = elapsed_time_crtc - next_cycle;
   component_elapsed_time_[index++] = elapsed_time_fdc - next_cycle;

   for (int i = 0; i < signals_.nb_expansion_; i++)
   {
      component_elapsed_time_[index++] = elapsed_components[i] - next_cycle;
   }

   if (run_ &&  step_)
   {
      remember_step_ = true;
   }

   if (supervisor_ != NULL)
   {
      if (!run_) supervisor_->EmulationStopped();
      if (counter_ >= 4000000)
      {
         supervisor_->SetEfficience(GetSpeed());
         supervisor_->RefreshRunningData();
         // Refresh ?
      }
   }
   return next_cycle;
}

void Motherboard::AddBreakpoint(unsigned short addr)
{

   // Add breakpoint to list
   if (breakpoint_index_ < NB_BP_MAX)
   {
      breakpoint_list_[breakpoint_index_++] = addr;
   }
}

void Motherboard::ChangeBreakpoint(unsigned short  oldBP, unsigned short newBP)
{
   for (unsigned int i = 0; i < breakpoint_index_; i++)
   {
      if (breakpoint_list_[i] == oldBP)
      {
         breakpoint_list_[i] = newBP;
         return;
      }
   }
}

void Motherboard::RemoveBreakpoint(unsigned short addr)
{
   for (unsigned int i = 0; i < breakpoint_index_; i++)
   {
      if (breakpoint_list_[i] == addr)
      {
         breakpoint_list_[i] = 0;
         for (unsigned int j = i; j < breakpoint_index_ - 1; j++)
         {
            breakpoint_list_[j] = breakpoint_list_[j + 1];
         }
         breakpoint_index_--;
         return;
      }
   }
}
