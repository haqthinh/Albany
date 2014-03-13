//*****************************************************************//
//    Albany 2.0:  Copyright 2012 Sandia Corporation               //
//    This Software is released under the BSD license detailed     //
//    in the file "license.txt" in the top-level Albany directory  //
//*****************************************************************//

#ifndef TOTAL_CONCENTRATION_HPP
#define TOTAL_CONCENTRATION_HPP

#include "Phalanx_ConfigDefs.hpp"
#include "Phalanx_Evaluator_WithBaseImpl.hpp"
#include "Phalanx_Evaluator_Derived.hpp"
#include "Phalanx_MDField.hpp"

namespace LCM {
/** \brief

    This evaluator computes the sum of the
    hydrogen concentration at trapped and lattice site
*/

template<typename EvalT, typename Traits>
class TotalConcentration : public PHX::EvaluatorWithBaseImpl<Traits>,
	       public PHX::EvaluatorDerived<EvalT, Traits>  {

public:

  TotalConcentration(const Teuchos::ParameterList& p);

  void postRegistrationSetup(typename Traits::SetupData d,
			     PHX::FieldManager<Traits>& vm);

  void evaluateFields(typename Traits::EvalData d);

private:

  typedef typename EvalT::ScalarT ScalarT;
  typedef typename EvalT::MeshScalarT MeshScalarT;

  // Input:
  PHX::MDField<ScalarT,Cell,QuadPoint> Clattice;
  PHX::MDField<ScalarT,Cell,QuadPoint> Ctrapped;

  // Output:
  PHX::MDField<ScalarT,Cell,QuadPoint,Dim,Dim> Ctotal;

  unsigned int numQPs;
  unsigned int numDims;
};
}

#endif