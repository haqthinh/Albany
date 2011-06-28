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


#include "QCAD_MaterialDatabase.hpp"
#include "Teuchos_XMLParameterListHelpers.hpp"
#include "Teuchos_TestForException.hpp"
#include "Albany_Utils.hpp"

QCAD::MaterialDatabase::
MaterialDatabase( const std::string& inputFile,
		  const Teuchos::RCP<const Epetra_Comm>& ecomm)
  : data_("Material Parameters")
{
  if(inputFile.length() == 0) {
    pMaterialsList_ = NULL;
    pEBList_        = NULL;
    pNSList_        = NULL;
    return;
  }

  const Albany_MPI_Comm& mcomm = Albany::getMpiCommFromEpetraComm(*ecomm);
  Teuchos::RCP<Teuchos::Comm<int> > tcomm = Albany::createTeuchosCommFromMpiComm(mcomm);

  std::cout << "Initializing material database from " << inputFile << std::endl;
  Teuchos::updateParametersFromXmlFileAndBroadcast(inputFile, &data_, *tcomm);
  //Teuchos::updateParametersFromXmlFile(inputFile, &data_);
  
  //Check for and Set element block and materials sublists
  TEST_FOR_EXCEPTION(!data_.isSublist("Materials"), Teuchos::Exceptions::InvalidParameter,
	  std::endl << "Material Database Error: Materials sublist required" << std::endl);
  TEST_FOR_EXCEPTION(!data_.isSublist("ElementBlocks"), Teuchos::Exceptions::InvalidParameter,
	  std::endl << "Material Database Error: ElementBlocks sublist required" << std::endl);
  TEST_FOR_EXCEPTION(!data_.isSublist("NodeSets"), Teuchos::Exceptions::InvalidParameter,
	  std::endl << "Material Database Error: NodeSets sublist required" << std::endl);

  pMaterialsList_ = &(data_.sublist("Materials"));
  pEBList_        = &(data_.sublist("ElementBlocks"));
  pNSList_        = &(data_.sublist("NodeSets"));
}

QCAD::MaterialDatabase::
~MaterialDatabase()
{
}


template<typename T> T 
QCAD::MaterialDatabase:: 
getParam(const std::string& paramName)
{
  return data_.get<T>(paramName);
}

template<typename T> T 
QCAD::MaterialDatabase:: 
getParam(const std::string& paramName, T def_value)
{
  return data_.get<T>(paramName, def_value);
}


template<typename T> T 
QCAD::MaterialDatabase:: 
getMaterialParam(const std::string& materialName, const std::string& paramName)
{
  TEST_FOR_EXCEPTION(pMaterialsList_ == NULL, Teuchos::Exceptions::InvalidParameter,
		     std::endl << "MaterialDB Error! param required but no DB." << std::endl);

  TEST_FOR_EXCEPTION(materialName.length() == 0, Teuchos::Exceptions::InvalidParameter,
		     std::endl << "MaterialDB Error! Empty material name" << std::endl);

  TEST_FOR_EXCEPTION(!pMaterialsList_->isSublist(materialName), 
		     Teuchos::Exceptions::InvalidParameter, std::endl 
		     << "MaterialDB Error! Invalid material name " 
		     << materialName <<  std::endl);

  Teuchos::ParameterList& subList = pMaterialsList_->sublist(materialName);
  return subList.get<T>(paramName);
}

template<typename T> T 
QCAD::MaterialDatabase:: 
getMaterialParam(const std::string& materialName, const std::string& paramName, T def_value)
{
  if(pMaterialsList_ == NULL) return def_value;

  TEST_FOR_EXCEPTION(materialName.length() == 0, Teuchos::Exceptions::InvalidParameter,
		     std::endl << "MaterialDB Error! Empty material name" << std::endl);

  TEST_FOR_EXCEPTION(!pMaterialsList_->isSublist(materialName), 
		     Teuchos::Exceptions::InvalidParameter, std::endl 
		     << "MaterialDB Error! Invalid material name " 
		     << materialName <<  std::endl);

  Teuchos::ParameterList& subList = pMaterialsList_->sublist(materialName);
  return subList.get<T>(paramName, def_value);
}


template<typename T> T 
QCAD::MaterialDatabase:: 
getNodeSetParam(const std::string& nsName, const std::string& paramName)
{
  TEST_FOR_EXCEPTION(pNSList_ == NULL, Teuchos::Exceptions::InvalidParameter,
		     std::endl << "MaterialDB Error! param required but no DB." << std::endl);

  TEST_FOR_EXCEPTION(nsName.length() == 0, Teuchos::Exceptions::InvalidParameter,
		     std::endl << "MaterialDB Error! Empty nodeset name" << std::endl);

  TEST_FOR_EXCEPTION(!pNSList_->isSublist(nsName), 
		     Teuchos::Exceptions::InvalidParameter, std::endl 
		     << "MaterialDB Error! Invalid nodeset name " 
		     << nsName <<  std::endl);

  Teuchos::ParameterList& subList = pNSList_->sublist(nsName);
  return subList.get<T>(paramName);
}

template<typename T> T 
QCAD::MaterialDatabase:: 
getNodeSetParam(const std::string& nsName, const std::string& paramName, T def_value)
{
  if(pNSList_ == NULL) return def_value;

  TEST_FOR_EXCEPTION(nsName.length() == 0, Teuchos::Exceptions::InvalidParameter,
		     std::endl << "MaterialDB Error! Empty nodeset name" << std::endl);

  TEST_FOR_EXCEPTION(!pNSList_->isSublist(nsName), 
		     Teuchos::Exceptions::InvalidParameter, std::endl 
		     << "MaterialDB Error! Invalid nodeset name " 
		     << nsName <<  std::endl);
  
  Teuchos::ParameterList& subList = pNSList_->sublist(nsName);
  return subList.get<T>(paramName, def_value);
}


template<typename T> T 
QCAD::MaterialDatabase:: 
getElementBlockParam(const std::string& ebName, const std::string& paramName)
{
  TEST_FOR_EXCEPTION(pEBList_ == NULL, Teuchos::Exceptions::InvalidParameter,
		     std::endl << "MaterialDB Error! param required but no DB." << std::endl);

  TEST_FOR_EXCEPTION(ebName.length() == 0, Teuchos::Exceptions::InvalidParameter,
		     std::endl << "MaterialDB Error! Empty element block name" << std::endl);

  TEST_FOR_EXCEPTION(!pEBList_->isSublist(ebName), Teuchos::Exceptions::InvalidParameter,
		     std::endl << "MaterialDB Error! Invalid element block name " 
		     << ebName << std::endl);

  Teuchos::ParameterList& subList = pEBList_->sublist(ebName);

  if( subList.isParameter(paramName) )
    return subList.get<T>(paramName);

  //check if related material exists (it always should)
  TEST_FOR_EXCEPTION(!subList.isParameter("material"), 
		     Teuchos::Exceptions::InvalidParameter, std::endl 
		     << "MaterialDB Error! Param " << paramName
		     << " not found in " << ebName << " list and there"
		     << " is no related material." << std::endl);

  //Parameter not directly in element block sublist, so try related material
  std::string materialName = subList.get<std::string>("material");
  TEST_FOR_EXCEPTION(!pMaterialsList_->isSublist(materialName), 
		     Teuchos::Exceptions::InvalidParameter, std::endl 
		     << "MaterialDB Error! Param " << paramName
		     << " not found in " << ebName << " list, and related"
		     << " material " << materialName << "is invalid." << std::endl);

  Teuchos::ParameterList& matSubList = pMaterialsList_->sublist(materialName);
  TEST_FOR_EXCEPTION(!matSubList.isParameter(paramName), 
		     Teuchos::Exceptions::InvalidParameter, std::endl 
		     << "MaterialDB Error! Param " << paramName
		     << " not found in " << ebName << " list or related"
		     << " material " << materialName << "list." << std::endl);
  return matSubList.get<T>(paramName);
}


template<typename T> T 
QCAD::MaterialDatabase:: 
getElementBlockParam(const std::string& ebName, const std::string& paramName, T def_value)
{
  if(pEBList_ == NULL) return def_value;

  TEST_FOR_EXCEPTION(ebName.length() == 0, Teuchos::Exceptions::InvalidParameter,
		     std::endl << "MaterialDB Error! Empty element block name" << std::endl);

  //check if element block exists - if not return default
  if(!pEBList_->isSublist(ebName)) return def_value;

  Teuchos::ParameterList& subList = pEBList_->sublist(ebName);

  if( subList.isParameter(paramName) )
    return subList.get<T>(paramName);

  //check if related material exists - if not return default
  if(!subList.isParameter("material")) return def_value;

  //Parameter not directly in element block sublist, so try related material
  std::string materialName = subList.get<std::string>("material");
  TEST_FOR_EXCEPTION(!pMaterialsList_->isSublist(materialName), 
		     Teuchos::Exceptions::InvalidParameter, std::endl 
		     << "MaterialDB Error! Param " << paramName
		     << " not found in " << ebName << " list, and related"
		     << " material " << materialName << "is invalid." << std::endl);

  Teuchos::ParameterList& matSubList = pMaterialsList_->sublist(materialName);
  return matSubList.get<T>(paramName, def_value);
}




//explicit instantiation of functions above; otherwise they never
// seems to get intantiated by compiler

//double
template double QCAD::MaterialDatabase:: 
getParam<double>(const std::string& paramName);
template double QCAD::MaterialDatabase:: 
getParam<double>(const std::string& paramName, double def_val);

template double QCAD::MaterialDatabase:: 
getMaterialParam<double>(const std::string& materialName, const std::string& paramName);
template double QCAD::MaterialDatabase:: 
getMaterialParam<double>(const std::string& materialName, const std::string& paramName, double def_val);

template double QCAD::MaterialDatabase:: 
getNodeSetParam<double>(const std::string& nsName, const std::string& paramName);
template double QCAD::MaterialDatabase:: 
getNodeSetParam<double>(const std::string& nsName, const std::string& paramName, double def_val);

template double QCAD::MaterialDatabase:: 
getElementBlockParam<double>(const std::string& materialName, const std::string& paramName);
template double QCAD::MaterialDatabase:: 
getElementBlockParam<double>(const std::string& materialName, const std::string& paramName, double def_val);

//int
template int QCAD::MaterialDatabase:: 
getParam<int>(const std::string& paramName);
template int QCAD::MaterialDatabase:: 
getParam<int>(const std::string& paramName, int def_val);

template int QCAD::MaterialDatabase:: 
getMaterialParam<int>(const std::string& materialName, const std::string& paramName);
template int QCAD::MaterialDatabase:: 
getMaterialParam<int>(const std::string& materialName, const std::string& paramName, int def_val);

template int QCAD::MaterialDatabase:: 
getNodeSetParam<int>(const std::string& nsName, const std::string& paramName);
template int QCAD::MaterialDatabase:: 
getNodeSetParam<int>(const std::string& nsName, const std::string& paramName, int def_val);

template int QCAD::MaterialDatabase:: 
getElementBlockParam<int>(const std::string& materialName, const std::string& paramName);
template int QCAD::MaterialDatabase:: 
getElementBlockParam<int>(const std::string& materialName, const std::string& paramName, int def_val);


//bool
template bool QCAD::MaterialDatabase:: 
getParam<bool>(const std::string& paramName);
template bool QCAD::MaterialDatabase:: 
getParam<bool>(const std::string& paramName, bool def_val);

template bool QCAD::MaterialDatabase:: 
getMaterialParam<bool>(const std::string& materialName, const std::string& paramName);
template bool QCAD::MaterialDatabase:: 
getMaterialParam<bool>(const std::string& materialName, const std::string& paramName, bool def_val);

template bool QCAD::MaterialDatabase:: 
getNodeSetParam<bool>(const std::string& nsName, const std::string& paramName);
template bool QCAD::MaterialDatabase:: 
getNodeSetParam<bool>(const std::string& nsName, const std::string& paramName, bool def_val);

template bool QCAD::MaterialDatabase:: 
getElementBlockParam<bool>(const std::string& materialName, const std::string& paramName);
template bool QCAD::MaterialDatabase:: 
getElementBlockParam<bool>(const std::string& materialName, const std::string& paramName, bool def_val);

//string
template std::string QCAD::MaterialDatabase:: 
getParam<std::string>(const std::string& paramName);
template std::string QCAD::MaterialDatabase:: 
getParam<std::string>(const std::string& paramName, std::string def_val);

template std::string QCAD::MaterialDatabase:: 
getMaterialParam<std::string>(const std::string& materialName, const std::string& paramName);
template std::string QCAD::MaterialDatabase:: 
getMaterialParam<std::string>(const std::string& materialName, const std::string& paramName, std::string def_val);

template std::string QCAD::MaterialDatabase:: 
getNodeSetParam<std::string>(const std::string& nsName, const std::string& paramName);
template std::string QCAD::MaterialDatabase:: 
getNodeSetParam<std::string>(const std::string& nsName, const std::string& paramName, std::string def_val);

template std::string QCAD::MaterialDatabase:: 
getElementBlockParam<std::string>(const std::string& materialName, const std::string& paramName);
template std::string QCAD::MaterialDatabase:: 
getElementBlockParam<std::string>(const std::string& materialName, const std::string& paramName, std::string def_val);

