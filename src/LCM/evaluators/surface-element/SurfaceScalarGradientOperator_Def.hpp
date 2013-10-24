//*****************************************************************//
//    albany 2.0:  Copyright 2012 Sandia Corporation               //
//    This Software is released under the BSD license detailed     //
//    in the file "license.txt" in the top-level Albany directory  //
//*****************************************************************//

#include <Intrepid_MiniTensor.h>
#include "Intrepid_FunctionSpaceTools.hpp"

#include "Teuchos_TestForException.hpp"
#include "Phalanx_DataLayout.hpp"

#include "Intrepid_RealSpaceTools.hpp"

namespace LCM {

  //**********************************************************************
  template<typename EvalT, typename Traits>
  SurfaceScalarGradientOperator<EvalT, Traits>::
  SurfaceScalarGradientOperator(const Teuchos::ParameterList& p,
                        const Teuchos::RCP<Albany::Layouts>& dl) :
    thickness      (p.get<double>("thickness")), 
    cubature       (p.get<Teuchos::RCP<Intrepid::Cubature<RealType> > >("Cubature")), 
    intrepidBasis  (p.get<Teuchos::RCP<Intrepid::Basis<RealType, Intrepid::FieldContainer<RealType> > > >("Intrepid Basis")),
    refDualBasis   (p.get<std::string>("Reference Dual Basis Name"),dl->qp_tensor),
    refNormal      (p.get<std::string>("Reference Normal Name"),dl->qp_vector),
    val_node   (p.get<std::string>("Nodal Scalar Name"),dl->node_scalar),
    surface_Grad_BF     (p.get<std::string>("Surface Scalar Gradient Operator Name"),dl->node_qp_gradient),
    grad_val_qp (p.get<std::string>   ("Surface Scalar Gradient Name"), dl->qp_gradient)
  {
    this->addDependentField(refDualBasis);
    this->addDependentField(refNormal);
    this->addDependentField(val_node);

    // Output fields
    this->addEvaluatedField(surface_Grad_BF);
    this->addEvaluatedField(grad_val_qp);


    this->setName("Surface Scalar Gradient"+PHX::TypeString<EvalT>::value);

    std::vector<PHX::DataLayout::size_type> dims;
    dl->node_qp_gradient->dimensions(dims);
    worksetSize = dims[0];
    numNodes = dims[1];
    numDims = dims[3];

    numQPs = cubature->getNumPoints();

    numPlaneNodes = numNodes / 2;
    numPlaneDims = numDims - 1;

#ifdef ALBANY_VERBOSE
    std::cout << "in Surface Gradient Jump" << std::endl;
    std::cout << " numPlaneNodes: " << numPlaneNodes << std::endl;
    std::cout << " numPlaneDims: " << numPlaneDims << std::endl;
    std::cout << " numQPs: " << numQPs << std::endl;
    std::cout << " cubature->getNumPoints(): " << cubature->getNumPoints() << std::endl;
    std::cout << " cubature->getDimension(): " << cubature->getDimension() << std::endl;
#endif


    // Allocate Temporary FieldContainers
    refValues.resize(numPlaneNodes, numQPs);
    refGrads.resize(numPlaneNodes, numQPs, numPlaneDims);
    refPoints.resize(numQPs, numPlaneDims);
    refWeights.resize(numQPs);

    // Pre-Calculate reference element quantitites
    cubature->getCubature(refPoints, refWeights);
    intrepidBasis->getValues(refValues, refPoints, Intrepid::OPERATOR_VALUE);
    intrepidBasis->getValues(refGrads, refPoints, Intrepid::OPERATOR_GRAD);
  }

  //**********************************************************************
  template<typename EvalT, typename Traits>
  void SurfaceScalarGradientOperator<EvalT, Traits>::
  postRegistrationSetup(typename Traits::SetupData d,
                        PHX::FieldManager<Traits>& fm)
  {
    this->utils.setFieldData(refDualBasis,fm);
    this->utils.setFieldData(refNormal,fm);
    this->utils.setFieldData(val_node,fm);
    this->utils.setFieldData(surface_Grad_BF,fm);
    this->utils.setFieldData(grad_val_qp,fm);


  }

  //**********************************************************************
  template<typename EvalT, typename Traits>
  void SurfaceScalarGradientOperator<EvalT, Traits>::
  evaluateFields(typename Traits::EvalData workset)
  {

    //Intrepid::Vector<ScalarT> Transformed_Grad_plus(3);
	//Intrepid::Vector<ScalarT> Transformed_Grad_minor(3);

	Intrepid::Vector<ScalarT> Parent_Grad_plus(3);
	Intrepid::Vector<ScalarT> Parent_Grad_minor(3);


    for (std::size_t cell=0; cell < workset.numCells; ++cell) {
      for (std::size_t pt=0; pt < numQPs; ++pt) {

       // Need to inverse basis [G_0 ; G_1; G_2] and none of them should be normalized
        Intrepid::Tensor<ScalarT> gBasis(3, &refDualBasis(cell, pt, 0, 0));

        Intrepid::Vector<ScalarT> N(3, &refNormal(cell, pt, 0));

        // This map the position vector from parent to current configuration in R^3
        gBasis = Intrepid::transpose(gBasis);

        // in-plane (parallel) contribution
        for (int node(0); node < numPlaneNodes; ++node) {
          int topNode = node + numPlaneNodes;

          // the parallel-to-the-plane term
          for (int i(0); i < numPlaneDims; ++i ){
        	  Parent_Grad_plus(i) = 0.5*refGrads(node, pt, i);
        	  Parent_Grad_minor(i) = 0.5*refGrads(node, pt, i);
          }

          // the orthogonal-to-the-plane term
          Parent_Grad_plus(numPlaneDims) = refValues(node,pt)/thickness;
          Parent_Grad_minor(numPlaneDims) = -refValues(node,pt)/thickness;

          // Mapping from parent to the physical domain
          Intrepid::Vector<ScalarT> Transformed_Grad_plus(Intrepid::dot(gBasis, Parent_Grad_plus));
          Intrepid::Vector<ScalarT> Transformed_Grad_minor(Intrepid::dot(gBasis,Parent_Grad_minor));

          // assign components to MDfield ScalarGrad
         for (int j(0); j < numDims; ++j ){
        	surface_Grad_BF(cell, topNode, pt, j) = Transformed_Grad_plus(j);
        	surface_Grad_BF(cell, node, pt, j) = Transformed_Grad_minor(j);
      //  	 surface_Grad_BF(cell, topNode, pt, j) = N(j)*refValues(node,pt)/thickness;
      //     	 surface_Grad_BF(cell, node, pt, j) = -N(j)*refValues(node,pt)/thickness;
         }
        }
      }
    }

    for (std::size_t cell=0; cell < workset.numCells; ++cell) {
    	  for (std::size_t pt=0; pt < numQPs; ++pt) {
    		  for (int k(0); k< numDims; ++k){
    			  grad_val_qp(cell, pt, k) = 0;
    				 for (int node(0); node < numNodes; ++node) {
    					 grad_val_qp(cell, pt, k) += surface_Grad_BF(cell, node, pt, k)*
    							                                         val_node(cell,node);
    				 }
    		  }
    	  }
    }

// NOTE: The following code will not work for the localization element. Don't use it. -SWCS
//    Intrepid::FunctionSpaceTools::
//           evaluate<ScalarT>(grad_val_qp, val_node, surface_Grad_BF);

/*
  std::cout << grad_val_qp(1, 1, 1) << std::endl;
  std::cout << grad_val_qp(1, 1, 2) << std::endl;
  std::cout << grad_val_qp(1, 1, 3) << std::endl;
  std::cout << grad_val_qp(1, 2, 1) << std::endl;
  std::cout << grad_val_qp(1, 2, 2) << std::endl;
  std::cout << grad_val_qp(1, 2, 3) << std::endl;
  std::cout << grad_val_qp(1, 3, 1) << std::endl;
  std::cout << grad_val_qp(1, 3, 2) << std::endl;
  std::cout << grad_val_qp(1, 3, 3) << std::endl;
  std::cout << grad_val_qp(1, 4, 1) << std::endl;
  std::cout << grad_val_qp(1, 4, 2) << std::endl;
  std::cout << grad_val_qp(1, 4, 3) << std::endl;


    std::cout << surface_Grad_BF(1, 1,1, 1) << std::endl;
    std::cout << surface_Grad_BF(1, 1,1, 2) << std::endl;
    std::cout << surface_Grad_BF(1, 1,1, 3) << std::endl;
    std::cout << surface_Grad_BF(1, 2,1, 1) << std::endl;
    std::cout << surface_Grad_BF(1, 2,1, 2) << std::endl;
    std::cout << surface_Grad_BF(1, 2,1, 3) << std::endl;
    std::cout << surface_Grad_BF(1, 3,1, 1) << std::endl;
    std::cout << surface_Grad_BF(1, 3,1, 2) << std::endl;
    std::cout << surface_Grad_BF(1, 3,1, 3) << std::endl;
    std::cout << surface_Grad_BF(1, 4,1, 1) << std::endl;
    std::cout << surface_Grad_BF(1, 4,1, 2) << std::endl;
    std::cout << surface_Grad_BF(1, 4,1, 3) << std::endl;
*/


  }
  //**********************************************************************  
  }