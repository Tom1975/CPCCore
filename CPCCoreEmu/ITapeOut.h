
#pragma once

class ITapeOut
{
public:
   virtual void SetDataRecorderR(bool set) = 0;
   virtual bool GetCassetteWriteData() = 0;
};
