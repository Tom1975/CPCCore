#pragma once

class Iterator
{
public :
   virtual void First() = 0;
   virtual void Next() = 0;
   virtual void CurrentItem() = 0;
   virtual void Ended() = 0;
   
};
