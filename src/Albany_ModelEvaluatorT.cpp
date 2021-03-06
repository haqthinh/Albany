/********************************************************************\
*            Albany, Copyright (2010) Sandia Corporation             *
*                                                                    *
* Notice: This computer software was prepared by Sandia Corporation, *
* hereinafter the Contractor, under Contract DE-AC04-94AL85000 with  *
* the Department of Energy (DOE). All rights in the computer software*
* are reserved by DOE on behalf of the United States Government and  *
* the Contractor as provided in the Contract. You are authorized to  *
* use this computer software for Governmental purposes but it is not *
* to be released or distributed to the public. NEITHER THE GOVERNMENT*
* NOR THE CONTRACTOR MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR      *
* ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE. This notice    *
* including this sentence must appear on any copies of this software.*
*    Questions to Andy Salinger, agsalin@sandia.gov                  *
\********************************************************************/

//IK, 9/12/14: has Epetra_Comm! No other Epetra.

#include "Albany_ModelEvaluatorT.hpp"
#include "Albany_DistributedParameterDerivativeOpT.hpp"
#include "Teuchos_ScalarTraits.hpp"
#include "Teuchos_TestForException.hpp"
#include "Tpetra_ConfigDefs.hpp"

Albany::ModelEvaluatorT::ModelEvaluatorT(
    const Teuchos::RCP<Albany::Application>& app_,
    const Teuchos::RCP<Teuchos::ParameterList>& appParams)
: app(app_)
{

  Teuchos::RCP<Teuchos::FancyOStream> out =
    Teuchos::VerboseObjectBase::getDefaultOStream();

  // Parameters (e.g., for sensitivities, SG expansions, ...)
  Teuchos::ParameterList& problemParams = appParams->sublist("Problem");
  Teuchos::ParameterList& parameterParams =
    problemParams.sublist("Parameters");

  num_param_vecs =
    parameterParams.get("Number of Parameter Vectors", 0);
  bool using_old_parameter_list = false;
  if (parameterParams.isType<int>("Number")) {
    int numParameters = parameterParams.get<int>("Number");
    if (numParameters > 0) {
      num_param_vecs = 1;
      using_old_parameter_list = true;
    }
  }

  *out << "Number of parameter vectors  = " << num_param_vecs << std::endl;

  Teuchos::ParameterList& responseParams =
    problemParams.sublist("Response Functions");

  int num_response_vecs =
     responseParams.get("Number of Response Vectors", 0);
  bool using_old_response_list = false;
  if (responseParams.isType<int>("Number")) {
    int numParameters = responseParams.get<int>("Number");
    if (numParameters > 0) {
      num_response_vecs = 1;
      using_old_response_list = true;
    }
  }

  param_names.resize(num_param_vecs);
  for (int l = 0; l < num_param_vecs; ++l) {
    const Teuchos::ParameterList* pList =
      using_old_parameter_list ?
      &parameterParams :
      &(parameterParams.sublist(Albany::strint("Parameter Vector", l)));

    const int numParameters = pList->get<int>("Number");
    TEUCHOS_TEST_FOR_EXCEPTION(
      numParameters == 0,
      Teuchos::Exceptions::InvalidParameter,
      std::endl << "Error!  In Albany::ModelEvaluatorT constructor:  " <<
      "Parameter vector " << l << " has zero parameters!" << std::endl);

    param_names[l] = Teuchos::rcp(new Teuchos::Array<std::string>(numParameters));
    for (int k = 0; k < numParameters; ++k) {
      (*param_names[l])[k] =
        pList->get<std::string>(Albany::strint("Parameter", k));
    }

    *out << "Number of parameters in parameter vector "
      << l << " = " << numParameters << std::endl;
  }
  
  // Setup distributed parameters
  distParamLib = app->getDistParamLib();
  Teuchos::ParameterList& distParameterParams =
    problemParams.sublist("Distributed Parameters");
  num_dist_param_vecs =
    distParameterParams.get("Number of Parameter Vectors", 0);
  dist_param_names.resize(num_dist_param_vecs);
  *out << "Number of distributed parameters vectors  = " << num_dist_param_vecs
       << std::endl;
  for (int i=0; i<num_dist_param_vecs; i++) {
    std::string name =
      distParameterParams.get<std::string>(Albany::strint("Parameter",i));
    TEUCHOS_TEST_FOR_EXCEPTION(
      !distParamLib->has(name),
      Teuchos::Exceptions::InvalidParameter,
      std::endl << "Error!  In Albany::ModelEvaluator constructor:  " <<
      "Invalid distributed parameter name " << name << std::endl);
    dist_param_names[i] = name;
  }

  Teuchos::Array<Teuchos::RCP<Teuchos::Array<std::string> > > response_names;
  response_names.resize(num_response_vecs);
  for (int l = 0; l < num_response_vecs; ++l) {
    const Teuchos::ParameterList* pList =
      using_old_response_list ?
      &responseParams :
      &(responseParams.sublist(Albany::strint("Response Vector", l)));

    bool number_exists = pList->getEntryPtr("Number");

    if(number_exists){

      const int numParameters = pList->get<int>("Number");
      TEUCHOS_TEST_FOR_EXCEPTION(
        numParameters == 0,
        Teuchos::Exceptions::InvalidParameter,
        std::endl << "Error!  In Albany::ModelEvaluatorT constructor:  " <<
        "Response vector " << l << " has zero parameters!" << std::endl);

      response_names[l] = Teuchos::rcp(new Teuchos::Array<std::string>(numParameters));
      for (int k = 0; k < numParameters; ++k) {
        (*response_names[l])[k] =
          pList->get<std::string>(Albany::strint("Response", k));
      }
    }
  }

  *out << "Number of response vectors  = " << num_response_vecs << std::endl;

  sacado_param_vec.resize(num_param_vecs);
  tpetra_param_vec.resize(num_param_vecs);
  tpetra_param_map.resize(num_param_vecs);
  thyra_response_vec.resize(num_response_vecs);

  Teuchos::RCP<const Teuchos::Comm<int> > commT = app->getComm(); 
   for (int l = 0; l < tpetra_param_vec.size(); ++l) {
     try {
       // Initialize Sacado parameter vector
       // The following call will throw, and it is often due to an incorrect input line in the "Parameters" PL
       // in the input file. Give the user a hint about what might be happening
       app->getParamLib()->fillVector<PHAL::AlbanyTraits::Residual>(
         *(param_names[l]), sacado_param_vec[l]);
     }
     catch (const std::logic_error& le) {

       *out << "Error: exception thrown from ParamLib fillVector in file " << __FILE__ << " line " << __LINE__ << std::endl;
       *out << "This is probably due to something incorrect in the \"Parameters\" list in the input file, one of the lines:" 
            << std::endl;
       for (int k = 0; k < param_names[l]->size(); ++k) 
         *out << "      " << (*param_names[l])[k] << std::endl;

       throw le; // rethrow to shut things down
     }

     // Create Tpetra map for parameter vector
     Tpetra::LocalGlobal lg = Tpetra::LocallyReplicated;
     tpetra_param_map[l] =
       Teuchos::rcp(new Tpetra_Map(sacado_param_vec[l].size(), 0, commT, lg));

     // Create Tpetra vector for parameters
     tpetra_param_vec[l] = Teuchos::rcp(new Tpetra_Vector(tpetra_param_map[l]));
     for (unsigned int k = 0; k < sacado_param_vec[l].size(); ++k) {
       const Teuchos::ArrayRCP<ST> tpetra_param_vec_nonConstView = tpetra_param_vec[l]->get1dViewNonConst();
       tpetra_param_vec_nonConstView[k] = sacado_param_vec[l][k].baseValue;
     }
   }

   for (int l = 0; l < app->getNumResponses(); ++l) {

     // Create Thyra vector for responses
     Teuchos::RCP<const Tpetra_Map> mapT = app->getResponse(l)->responseMapT();
     Teuchos::RCP<const Thyra::VectorSpaceBase<ST> > gT_space = Thyra::createVectorSpace<ST>(mapT);
     thyra_response_vec[l] = Thyra::createMember(gT_space);

   }

  // Setup nominal values
  {
    nominalValues = this->createInArgsImpl();

    // All the ME vectors are unallocated here
    allocateVectors();

    // TODO: Check if correct nominal values for parameters
    for (int l = 0; l < num_param_vecs; ++l) {
      Teuchos::RCP<const Tpetra_Map> map = tpetra_param_map[l];
      Teuchos::RCP<const Thyra::VectorSpaceBase<ST> > tpetra_param_space = Thyra::createVectorSpace<ST>(map);
      nominalValues.set_p(l, Thyra::createVector(tpetra_param_vec[l], tpetra_param_space));
    }
  }

  timer = Teuchos::TimeMonitor::getNewTimer("Albany: **Total Fill Time**");

}

void
Albany::ModelEvaluatorT::allocateVectors()
    {

     const Teuchos::RCP<const Teuchos::Comm<int> > commT = app->getComm();

      // Create Tpetra objects to be wrapped in Thyra
      const Teuchos::RCP<const Tpetra_Vector> xT_init = app->getInitialSolutionT();
      const Teuchos::RCP<const Tpetra_Vector> x_dotT_init = app->getInitialSolutionDotT();
      const Teuchos::RCP<const Tpetra_Vector> x_dotdotT_init =
            Teuchos::rcp(new const Tpetra_Vector(app->getInitialSolutionDotT()->getMap()));
      const Teuchos::RCP<const Tpetra_Map> map = app->getMapT();
      const Teuchos::RCP<const Thyra::VectorSpaceBase<ST> > xT_space = Thyra::createVectorSpace<ST>(map);

      // Create non-const versions of xT_init and x_dotT_init
      const Teuchos::RCP<Tpetra_Vector> xT_init_nonconst = Teuchos::rcp(new Tpetra_Vector(*xT_init));
      const Teuchos::RCP<Tpetra_Vector> x_dotT_init_nonconst = Teuchos::rcp(new Tpetra_Vector(*x_dotT_init));
      const Teuchos::RCP<Tpetra_Vector> x_dotdotT_init_nonconst = Teuchos::rcp(new Tpetra_Vector(*x_dotdotT_init));

      nominalValues.set_x(Thyra::createVector(xT_init_nonconst, xT_space));
      nominalValues.set_x_dot(Thyra::createVector(x_dotT_init_nonconst, xT_space));

    }


// Overridden from Thyra::ModelEvaluator<ST>

Teuchos::RCP<const Thyra::VectorSpaceBase<ST> >
Albany::ModelEvaluatorT::get_x_space() const
{
  Teuchos::RCP<const Tpetra_Map> map = app->getMapT();
  Teuchos::RCP<const Thyra::VectorSpaceBase<ST> > x_space = Thyra::createVectorSpace<ST>(map);
  return x_space;
}


Teuchos::RCP<const Thyra::VectorSpaceBase<ST> >
Albany::ModelEvaluatorT::get_f_space() const
{
  Teuchos::RCP<const Tpetra_Map> map = app->getMapT();
  Teuchos::RCP<const Thyra::VectorSpaceBase<ST> > f_space = Thyra::createVectorSpace<ST>(map);
  return f_space;
}


Teuchos::RCP<const Thyra::VectorSpaceBase<ST> >
Albany::ModelEvaluatorT::get_p_space(int l) const
{
  TEUCHOS_TEST_FOR_EXCEPTION(
    l >= num_param_vecs + num_dist_param_vecs || l < 0,
    Teuchos::Exceptions::InvalidParameter,
    std::endl <<
    "Error!  Albany::ModelEvaluatorT::get_p_space():  " <<
    "Invalid parameter index l = " << l << std::endl);
  Teuchos::RCP<const Tpetra_Map> map; 
  if (l < num_param_vecs)
    map = tpetra_param_map[l];  
  //IK, 7/1/14: commenting this out for now
  //map = distParamLib->get(dist_param_names[l-num_param_vecs])->map(); 
  Teuchos::RCP<const Thyra::VectorSpaceBase<ST> > tpetra_param_space = Thyra::createVectorSpace<ST>(map);
  return tpetra_param_space;
}


Teuchos::RCP<const Thyra::VectorSpaceBase<ST> >
Albany::ModelEvaluatorT::get_g_space(int l) const
{
  TEUCHOS_TEST_FOR_EXCEPTION(
      l >= app->getNumResponses() || l < 0,
      Teuchos::Exceptions::InvalidParameter,
      std::endl <<
      "Error!  Albany::ModelEvaluatorT::get_g_space():  " <<
      "Invalid response index l = " << l << std::endl);

  Teuchos::RCP<const Tpetra_Map> mapT = app->getResponse(l)->responseMapT();
  Teuchos::RCP<const Thyra::VectorSpaceBase<ST> > gT_space = Thyra::createVectorSpace<ST>(mapT);
  return gT_space;
}


Teuchos::RCP<const Teuchos::Array<std::string> >
Albany::ModelEvaluatorT::get_p_names(int l) const
{
  TEUCHOS_TEST_FOR_EXCEPTION(
    l >= num_param_vecs + num_dist_param_vecs || l < 0,
    Teuchos::Exceptions::InvalidParameter,
    std::endl <<
    "Error!  Albany::ModelEvaluatorT::get_p_names():  " <<
    "Invalid parameter index l = " << l << std::endl);

  if (l < num_param_vecs)
    return param_names[l];
  return Teuchos::rcp(new Teuchos::Array<std::string>(1, dist_param_names[l-num_param_vecs]));
}


Thyra::ModelEvaluatorBase::InArgs<ST>
Albany::ModelEvaluatorT::getNominalValues() const
{
  return nominalValues;
}


Thyra::ModelEvaluatorBase::InArgs<ST>
Albany::ModelEvaluatorT::getLowerBounds() const
{
  return Thyra::ModelEvaluatorBase::InArgs<ST>(); // Default value
}


Thyra::ModelEvaluatorBase::InArgs<ST>
Albany::ModelEvaluatorT::getUpperBounds() const
{
  return Thyra::ModelEvaluatorBase::InArgs<ST>(); // Default value
}


Teuchos::RCP<Thyra::LinearOpBase<ST> >
Albany::ModelEvaluatorT::create_W_op() const
{
  const Teuchos::RCP<Tpetra_Operator> W =
    Teuchos::rcp(new Tpetra_CrsMatrix(app->getJacobianGraphT()));
  return Thyra::createLinearOp(W);
}


Teuchos::RCP<Thyra::PreconditionerBase<ST> >
Albany::ModelEvaluatorT::create_W_prec() const
{
  // TODO: Analog of EpetraExt::ModelEvaluator::Preconditioner does not exist in Thyra yet!
  const bool W_prec_not_supported = true;
  TEUCHOS_TEST_FOR_EXCEPT(W_prec_not_supported);
  return Teuchos::null;
}

Teuchos::RCP<Thyra::LinearOpBase<ST> >
Albany::ModelEvaluatorT::create_DfDp_op_impl(int j) const
{
  TEUCHOS_TEST_FOR_EXCEPTION(
    j >= num_param_vecs+num_dist_param_vecs || j < num_param_vecs,
    Teuchos::Exceptions::InvalidParameter,
    std::endl <<
    "Error!  Albany::ModelEvaluatorT::create_DfDp_op_impl():  " <<
    "Invalid parameter index j = " << j << std::endl);

  const Teuchos::RCP<Tpetra_Operator> DfDp = Teuchos::rcp(new DistributedParameterDerivativeOpT(
                      app, dist_param_names[j-num_param_vecs]));

  return Thyra::createLinearOp(DfDp); 
}


Teuchos::RCP<const Thyra::LinearOpWithSolveFactoryBase<ST> >
Albany::ModelEvaluatorT::get_W_factory() const
{
  return Teuchos::null;
}


Thyra::ModelEvaluatorBase::InArgs<ST>
Albany::ModelEvaluatorT::createInArgs() const
{

  return this->createInArgsImpl();

}


void
Albany::ModelEvaluatorT::reportFinalPoint(
    const Thyra::ModelEvaluatorBase::InArgs<ST>& finalPoint,
    const bool wasSolved)
{
  // TODO
  TEUCHOS_TEST_FOR_EXCEPTION(true,
     Teuchos::Exceptions::InvalidParameter,
     "Calling reportFinalPoint in Albany_ModelEvaluatorT.cpp line 296" << std::endl);

}


Teuchos::RCP<Thyra::LinearOpBase<ST> >
Albany::ModelEvaluatorT::create_DgDx_op_impl(int j) const
{
  TEUCHOS_TEST_FOR_EXCEPTION(
      j >= app->getNumResponses() || j < 0,
      Teuchos::Exceptions::InvalidParameter,
      std::endl <<
      "Error!  Albany::ModelEvaluatorT::create_DgDx_op_impl():  " <<
      "Invalid response index j = " << j << std::endl);

  return Thyra::createLinearOp(app->getResponse(j)->createGradientOpT());
}

// AGS: x_dotdot time integrators not imlemented in Thyra ME yet
//Teuchos::RCP<Thyra::LinearOpBase<ST> >
//Albany::ModelEvaluatorT::create_DgDx_dotdot_op_impl(int j) const
//{
//  TEUCHOS_TEST_FOR_EXCEPTION(
//    j >= app->getNumResponses() || j < 0,
//    Teuchos::Exceptions::InvalidParameter,
//    std::endl <<
//    "Error!  Albany::ModelEvaluatorT::create_DgDx_dotdot_op():  " <<
//    "Invalid response index j = " << j << std::endl);
//
//  return Thyra::createLinearOp(app->getResponse(j)->createGradientOpT());
//}



Teuchos::RCP<Thyra::LinearOpBase<ST> >
Albany::ModelEvaluatorT::create_DgDx_dot_op_impl(int j) const
{
  TEUCHOS_TEST_FOR_EXCEPTION(
      j >= app->getNumResponses() || j < 0,
      Teuchos::Exceptions::InvalidParameter,
      std::endl <<
      "Error!  Albany::ModelEvaluatorT::create_DgDx_dot_op_impl():  " <<
      "Invalid response index j = " << j << std::endl);

  return Thyra::createLinearOp(app->getResponse(j)->createGradientOpT());
}


Thyra::ModelEvaluatorBase::OutArgs<ST>
Albany::ModelEvaluatorT::createOutArgsImpl() const
{
  Thyra::ModelEvaluatorBase::OutArgsSetup<ST> result;
  result.setModelEvalDescription(this->description());

  const int n_g = app->getNumResponses();
  result.set_Np_Ng(num_param_vecs+num_dist_param_vecs, n_g);

  result.setSupports(Thyra::ModelEvaluatorBase::OUT_ARG_f, true);

  result.setSupports(Thyra::ModelEvaluatorBase::OUT_ARG_W_op, true);
  result.set_W_properties(
      Thyra::ModelEvaluatorBase::DerivativeProperties(
        Thyra::ModelEvaluatorBase::DERIV_LINEARITY_UNKNOWN,
        Thyra::ModelEvaluatorBase::DERIV_RANK_FULL,
        true));

  for (int l = 0; l < num_param_vecs; ++l) {
    result.setSupports(
        Thyra::ModelEvaluatorBase::OUT_ARG_DfDp, l,
        Thyra::ModelEvaluatorBase::DERIV_MV_BY_COL);
  }
  for (int i=0; i<num_dist_param_vecs; i++)
    result.setSupports(
        Thyra::ModelEvaluatorBase::OUT_ARG_DfDp, 
        i+num_param_vecs,
        Thyra::ModelEvaluatorBase::DERIV_LINEAR_OP);

  for (int i = 0; i < n_g; ++i) {
    Thyra::ModelEvaluatorBase::DerivativeSupport dgdx_support;
    if (app->getResponse(i)->isScalarResponse()) {
      dgdx_support = Thyra::ModelEvaluatorBase::DERIV_TRANS_MV_BY_ROW;
    } else {
      dgdx_support = Thyra::ModelEvaluatorBase::DERIV_LINEAR_OP;
    }

    result.setSupports(
        Thyra::ModelEvaluatorBase::OUT_ARG_DgDx, i, dgdx_support);
    result.setSupports(
        Thyra::ModelEvaluatorBase::OUT_ARG_DgDx_dot, i, dgdx_support);
    // AGS: x_dotdot time integrators not imlemented in Thyra ME yet
    //result.setSupports(
    //    Thyra::ModelEvaluatorBase::OUT_ARG_DgDx_dotdot, i, dgdx_support);

    for (int l = 0; l < num_param_vecs; l++)
      result.setSupports(
          Thyra::ModelEvaluatorBase::OUT_ARG_DgDp, i, l,
          Thyra::ModelEvaluatorBase::DERIV_MV_BY_COL);
    
    if (app->getResponse(i)->isScalarResponse()) {
      for (int j=0; j<num_dist_param_vecs; j++)
        result.setSupports(
           Thyra::ModelEvaluatorBase::OUT_ARG_DgDp, i, j+num_param_vecs,
           Thyra::ModelEvaluatorBase::DERIV_TRANS_MV_BY_ROW);
    }
    else {
      for (int j=0; j<num_dist_param_vecs; j++)
        result.setSupports(
           Thyra::ModelEvaluatorBase::OUT_ARG_DgDp, i, j+num_param_vecs,
           Thyra::ModelEvaluatorBase::DERIV_LINEAR_OP);
    }
  }

  return result;
}

void
Albany::ModelEvaluatorT::evalModelImpl(
    const Thyra::ModelEvaluatorBase::InArgs<ST>& inArgsT,
    const Thyra::ModelEvaluatorBase::OutArgs<ST>& outArgsT) const
{

  Teuchos::TimeMonitor Timer(*timer); //start timer
  //
  // Get the input arguments
  //
  const Teuchos::RCP<const Tpetra_Vector> xT =
    ConverterT::getConstTpetraVector(inArgsT.get_x());

  const Teuchos::RCP<const Tpetra_Vector> x_dotT =
    Teuchos::nonnull(inArgsT.get_x_dot()) ?
    ConverterT::getConstTpetraVector(inArgsT.get_x_dot()) :
    Teuchos::null;

  // AGS: x_dotdot time integrators not imlemented in Thyra ME yet
  //const Teuchos::RCP<const Tpetra_Vector> x_dotdotT =
  //  Teuchos::nonnull(inArgsT.get_x_dotdot()) ?
  //  ConverterT::getConstTpetraVector(inArgsT.get_x_dotdot()) :
  //  Teuchos::null;
  const Teuchos::RCP<const Tpetra_Vector> x_dotdotT = Teuchos::null;


  const double alpha = (Teuchos::nonnull(x_dotT) || Teuchos::nonnull(x_dotdotT)) ? inArgsT.get_alpha() : 0.0;
  // AGS: x_dotdot time integrators not imlemented in Thyra ME yet
  // const double omega = (Teuchos::nonnull(x_dotT) || Teuchos::nonnull(x_dotdotT)) ? inArgsT.get_omega() : 0.0;
  const double omega = 0.0;
  const double beta = (Teuchos::nonnull(x_dotT) || Teuchos::nonnull(x_dotdotT)) ? inArgsT.get_beta() : 1.0;
  const double curr_time = (Teuchos::nonnull(x_dotT) || Teuchos::nonnull(x_dotdotT)) ? inArgsT.get_t() : 0.0;

  for (int l = 0; l < inArgsT.Np(); ++l) {
    const Teuchos::RCP<const Thyra::VectorBase<ST> > p = inArgsT.get_p(l);
    if (Teuchos::nonnull(p)) {
      const Teuchos::RCP<const Tpetra_Vector> pT = ConverterT::getConstTpetraVector(p);
      const Teuchos::ArrayRCP<const ST> pT_constView = pT->get1dView();

      ParamVec &sacado_param_vector = sacado_param_vec[l];
      for (unsigned int k = 0; k < sacado_param_vector.size(); ++k) {
        sacado_param_vector[k].baseValue = pT_constView[k];
      }
    }
  }

  //
  // Get the output arguments
  //
  const Teuchos::RCP<Tpetra_Vector> fT_out =
    Teuchos::nonnull(outArgsT.get_f()) ?
    ConverterT::getTpetraVector(outArgsT.get_f()) :
    Teuchos::null;

  const Teuchos::RCP<Tpetra_Operator> W_op_outT =
    Teuchos::nonnull(outArgsT.get_W_op()) ?
    ConverterT::getTpetraOperator(outArgsT.get_W_op()) :
    Teuchos::null;

  // Cast W to a CrsMatrix, throw an exception if this fails
  const Teuchos::RCP<Tpetra_CrsMatrix> W_op_out_crsT =
    Teuchos::nonnull(W_op_outT) ?
    Teuchos::rcp_dynamic_cast<Tpetra_CrsMatrix>(W_op_outT, true) :
    Teuchos::null;

  //
  // Compute the functions
  //
  bool f_already_computed = false;

  // W matrix
  if (Teuchos::nonnull(W_op_out_crsT)) {
    app->computeGlobalJacobianT(
        alpha, beta, omega, curr_time, x_dotT.get(), x_dotdotT.get(),  *xT,
        sacado_param_vec, fT_out.get(), *W_op_out_crsT);
    f_already_computed = true;
  }

  // df/dp
  for (int l = 0; l < outArgsT.Np(); ++l) {
    const Teuchos::RCP<Thyra::MultiVectorBase<ST> > dfdp_out =
      outArgsT.get_DfDp(l).getMultiVector();

    const Teuchos::RCP<Tpetra_MultiVector> dfdp_outT =
      Teuchos::nonnull(dfdp_out) ?
      ConverterT::getTpetraMultiVector(dfdp_out) :
      Teuchos::null;

    if (Teuchos::nonnull(dfdp_outT)) {
      const Teuchos::RCP<ParamVec> p_vec = Teuchos::rcpFromRef(sacado_param_vec[l]);

      app->computeGlobalTangentT(
          0.0, 0.0, 0.0, curr_time, false, x_dotT.get(), x_dotdotT.get(), *xT,
          sacado_param_vec, p_vec.get(),
          NULL, NULL, NULL, NULL, fT_out.get(), NULL,
          dfdp_outT.get());

      f_already_computed = true;
    }
  }

  // f
  if (app->is_adjoint) {
    const Thyra::ModelEvaluatorBase::Derivative<ST> f_derivT(
        outArgsT.get_f(),
        Thyra::ModelEvaluatorBase::DERIV_TRANS_MV_BY_ROW);

    const Thyra::ModelEvaluatorBase::Derivative<ST> dummy_derivT;

    const int response_index = 0; // need to add capability for sending this in
    app->evaluateResponseDerivativeT(
        response_index, curr_time, x_dotT.get(), x_dotdotT.get(), *xT,
        sacado_param_vec, NULL,
        NULL, f_derivT, dummy_derivT, dummy_derivT, dummy_derivT);
  } else {
    if (Teuchos::nonnull(fT_out) && !f_already_computed) {
      app->computeGlobalResidualT(
          curr_time, x_dotT.get(), x_dotdotT.get(), *xT,
          sacado_param_vec, *fT_out);
    }
  }

  // Response functions
  for (int j = 0; j < outArgsT.Ng(); ++j) {
    const Teuchos::RCP<Thyra::VectorBase<ST> > g_out = outArgsT.get_g(j);
    Teuchos::RCP<Tpetra_Vector> gT_out =
      Teuchos::nonnull(g_out) ?
      ConverterT::getTpetraVector(g_out) :
      Teuchos::null;

    const Thyra::ModelEvaluatorBase::Derivative<ST> dgdxT_out = outArgsT.get_DgDx(j);
    const Thyra::ModelEvaluatorBase::Derivative<ST> dgdxdotT_out = outArgsT.get_DgDx_dot(j);
    // AGS: x_dotdot time integrators not imlemented in Thyra ME yet
    const Thyra::ModelEvaluatorBase::Derivative<ST> dgdxdotdotT_out;

    // dg/dx, dg/dxdot
    if (!dgdxT_out.isEmpty() || !dgdxdotT_out.isEmpty()) {
      const Thyra::ModelEvaluatorBase::Derivative<ST> dummy_derivT;
      app->evaluateResponseDerivativeT(
          j, curr_time, x_dotT.get(), x_dotdotT.get(), *xT,
          sacado_param_vec, NULL,
          gT_out.get(), dgdxT_out,
          dgdxdotT_out, dgdxdotdotT_out, dummy_derivT);
      // Set gT_out to null to indicate that g_out was evaluated.
      gT_out = Teuchos::null;
    }

    // dg/dp
    for (int l = 0; l < outArgsT.Np(); ++l) {
      const Teuchos::RCP<Thyra::MultiVectorBase<ST> > dgdp_out =
        outArgsT.get_DgDp(j, l).getMultiVector();
      const Teuchos::RCP<Tpetra_MultiVector> dgdpT_out =
        Teuchos::nonnull(dgdp_out) ?
        ConverterT::getTpetraMultiVector(dgdp_out) :
        Teuchos::null;

      if (Teuchos::nonnull(dgdpT_out)) {
        const Teuchos::RCP<ParamVec> p_vec = Teuchos::rcpFromRef(sacado_param_vec[l]);
        app->evaluateResponseTangentT(
            j, alpha, beta, omega, curr_time, false,
            x_dotT.get(), x_dotdotT.get(), *xT,
            sacado_param_vec, p_vec.get(),
            NULL, NULL, NULL, NULL, gT_out.get(), NULL,
            dgdpT_out.get());
        gT_out = Teuchos::null;
      }
    }

    if (Teuchos::nonnull(gT_out)) {
      app->evaluateResponseT(
          j, curr_time, x_dotT.get(), x_dotdotT.get(), *xT,
          sacado_param_vec, *gT_out);
    }
  }
}


Thyra::ModelEvaluatorBase::InArgs<ST>
Albany::ModelEvaluatorT::createInArgsImpl() const
{
  Thyra::ModelEvaluatorBase::InArgsSetup<ST> result;
  result.setModelEvalDescription(this->description());

  result.setSupports(Thyra::ModelEvaluatorBase::IN_ARG_x, true);

  result.setSupports(Thyra::ModelEvaluatorBase::IN_ARG_x_dot, true);
  // AGS: x_dotdot time integrators not imlemented in Thyra ME yet
  //result.setSupports(Thyra::ModelEvaluatorBase::IN_ARG_x_dotdot, true);
  result.setSupports(Thyra::ModelEvaluatorBase::IN_ARG_t, true);
  result.setSupports(Thyra::ModelEvaluatorBase::IN_ARG_alpha, true);
  result.setSupports(Thyra::ModelEvaluatorBase::IN_ARG_beta, true);
  // AGS: x_dotdot time integrators not imlemented in Thyra ME yet
  //result.setSupports(Thyra::ModelEvaluatorBase::IN_ARG_omega, true);

  result.set_Np(num_param_vecs+num_dist_param_vecs);

  return result;
}
