#ifndef DEF_APPSTATE_HPP
#define DEF_APPSTATE_HPP

#include "Marshalable.h"
#include "SampleState.h"
#include "TaxonomyManager.h"
#include "UserInterfaceState.h"

class AppState : public Marshalable {
 public:
  AppState();
  ~AppState();
  void setSample(int id, SampleState*);
  SampleState* getSample(int id);
  TaxonomyManager& getTaxonomy();
  std::string Marshal();
  Marshalable* Unmarshal();
  UserInterfaceState& getUiState();
  void setUiState(UserInterfaceState);

 private:
  std::vector<SampleState*> sampleStates;
  TaxonomyManager taxonomy;
  UserInterfaceState uiState;
};

#endif