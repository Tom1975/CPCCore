#pragma once

#include <string>
#include <vector>

class IComponent
{
public:
   virtual void Tick() = 0;
};

class BusLine : public IComponent
{
public:
   BusLine();
   virtual ~BusLine();

   void AddComponent(IComponent* component);

   void Tick() override;
   bool GetLevel();

protected:

   std::vector<IComponent*> component_list_;
   bool up_;
};

class SingleLineSample
{
public:
   SingleLineSample(std::string label, BusLine* line);

   void Clear();
   std::string GetSample();

   std::string label_;
   BusLine* line_;
   std::vector<bool> samples_;
};

class GateArray : public IComponent
{
public:
   GateArray();
   virtual ~GateArray();

   void CreateGateArray(BusLine* line_4, BusLine* line_1);

   void Tick() override;

protected:
   unsigned int counter;
   BusLine *line_4_mhz_;
   BusLine *line_1_mhz_;
};

class Motherboard
{
public:

   ///////////////////////////////////////
   // CTor / DTor
   Motherboard();
   virtual ~Motherboard();

   ///////////////////////////////////////
   // Creation
   void Create();

   ///////////////////////////////////////
   // Run
   void Tick();

   ///////////////////////////////////////
   // Sample
   void StartSample();
   std::string StopSample();

protected:
   ///////////////////////////////////////
   // Sample
   bool sample_;

   std::vector<SingleLineSample> samples_;

   ///////////////////////////////////////
   // Inner components
   BusLine line_16_mhz_;
   BusLine line_4_mhz_;
   BusLine line_1_mhz_;

   GateArray gate_array_;
};
