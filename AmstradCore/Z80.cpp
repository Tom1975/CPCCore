
#include <iostream>
#include <sstream>

#include "Z80.h"

///////////////////////////////////////
// Z80
//
Z80::Z80() 
{
   
}
Z80::~Z80()
{
   
}

void Z80::Create()
{
   Reset();
}

void Z80::Reset()
{
   
}

void Z80::TickDown()
{
   // Forget it : It will probably not be used 
}


void Z80::TickUp()
{
   ++counter_;
   (this->*(tick_functions_)[machine_cycle_ | t_])();
}

