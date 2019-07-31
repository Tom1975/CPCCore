#pragma once

#include "MachineSettings.h"

class ISettings
{
public:
   virtual HWND CreateModelessConfigDlg (HWND parent) = 0;
   virtual void InitDlg (MachineSettings* settings) = 0;
   virtual void Apply () = 0;
   virtual void Cancel () = 0;

   virtual void Update (MachineSettings* settings) = 0;
};

