#include "MagneticField/Engine/interface/MagneticField.h"
#include "FastSimulation/ParticlePropagator/interface/MagneticFieldMap.h"
#include "FastSimulation/TrackerSetup/interface/TrackerInteractionGeometry.h"

#include "TH1.h"
#include "TAxis.h"
#include <iostream>
#include <string>

MagneticFieldMap*
MagneticFieldMap::myself=0; 

MagneticFieldMap* 
MagneticFieldMap::instance(const MagneticField* pMF,
			   TrackerInteractionGeometry* myGeo)
{
  if (!myself) myself = new MagneticFieldMap(pMF,myGeo);
  myself->initialize();
  return myself;
}

MagneticFieldMap* 
MagneticFieldMap::instance() {
  return myself;
}

MagneticFieldMap::MagneticFieldMap(const MagneticField* pMF,
				   TrackerInteractionGeometry* myGeo) : 
  pMF_(pMF), 
  geometry_(myGeo), 
  fieldBarrelHistos(200,static_cast<TH1*>(0)),
  fieldEndcapHistos(200,static_cast<TH1*>(0)),
  fieldBarrelBinWidth(200,static_cast<double>(0)),
  fieldBarrelZMin(200,static_cast<double>(0)),
  fieldEndcapBinWidth(200,static_cast<double>(0)),
  fieldEndcapRMin(200,static_cast<double>(0))
{;}

void
MagneticFieldMap::initialize()
{
  
  std::list<TrackerLayer>::iterator cyliter;
  std::list<TrackerLayer>::iterator cylitBeg=geometry_->cylinderBegin();
  std::list<TrackerLayer>::iterator cylitEnd=geometry_->cylinderEnd();

  gROOT->cd();
  
  // Prepare the histograms
  std::cout << "Prepare magnetic field local database for FAMOS speed-up" << std::endl;
  for ( cyliter=cylitBeg; cyliter != cylitEnd; ++cyliter ) {
    int layer = cyliter->layerNumber();
    //    cout << " Fill Histogram " << hist << endl;

    // Cylinder bounds
    double zmin = 0.;
    double zmax; 
    double rmin = 0.;
    double rmax; 
    if ( cyliter->forward() ) {
      zmax = cyliter->disk()->position().z();
      rmax = cyliter->disk()->outerRadius();
    } else {
      zmax = cyliter->cylinder()->bounds().length()/2.;
      rmax = cyliter->cylinder()->bounds().width()/2.
	   - cyliter->cylinder()->bounds().thickness()/2.;
    }

    // Histograms
    int bins=101;
    double step;

    // Disk histogram characteristics
    std::string histEndcap = Form("LayerEndCap_%u",layer);
    step = (rmax-rmin)/(bins-1);
    fieldEndcapHistos[layer] = 
      new TH1D(histEndcap.c_str(),"",bins,rmin,rmax+step);
    fieldEndcapBinWidth[layer] = step;
    fieldEndcapRMin[layer] = rmin;

    // Fill the histo
    for ( double radius=rmin+step/2.; radius<rmax+step; radius+=step ) {
      double field = inTeslaZ(GlobalPoint(radius,0.,zmax));
      fieldEndcapHistos[layer]->Fill(radius,field);
    }

    // Barrel Histogram characteritics
    std::string histBarrel = Form("LayerBarrel_%u",layer);
    step = (zmax-zmin)/(bins-1);
    fieldBarrelHistos[layer] = 
      new TH1D(histBarrel.c_str(),"",bins,zmin,zmax+step);
    fieldBarrelBinWidth[layer] = step;
    fieldBarrelZMin[layer] = zmin;

    // Fill the histo
    for ( double zed=zmin+step/2.; zed<zmax+step; zed+=step ) {
      double field = inTeslaZ(GlobalPoint(rmax,0.,zed));
      fieldBarrelHistos[layer]->Fill(zed,field);
    }
  }
}


const GlobalVector
MagneticFieldMap::inTesla( const GlobalPoint& gp) const {

  if (!instance()) {
    return GlobalVector( 0., 0., 4.);
  } else {
    return pMF_->inTesla(gp);
  }

}

const GlobalVector
MagneticFieldMap::inTesla(const TrackerLayer& aLayer, double coord, int success) const {

  if (!instance()) {
    return GlobalVector( 0., 0., 4.);
  } else {
    return GlobalVector(0.,0.,inTeslaZ(aLayer,coord,success));
  }

}

const GlobalVector 
MagneticFieldMap::inKGauss( const GlobalPoint& gp) const {
  
  return inTesla(gp) * 10.;

}

const GlobalVector
MagneticFieldMap::inInverseGeV( const GlobalPoint& gp) const {
  
  return inKGauss(gp) * 2.99792458e-4;

} 

double 
MagneticFieldMap::inTeslaZ(const GlobalPoint& gp) const {

    return instance() ? pMF_->inTesla(gp).z() : 4.0;

}

double
MagneticFieldMap::inTeslaZ(const TrackerLayer& aLayer, double coord, int success) const 
{

  if (!instance()) {
    return 4.;
  } else {
    // Find the relevant histo
    TH1* theHisto; 
    double theBinWidth;
    double theXMin;
    unsigned layer = aLayer.layerNumber();

    if ( success == 1 ) { 
      theHisto = fieldBarrelHistos[layer];
      theBinWidth = fieldBarrelBinWidth[layer];
      theXMin = fieldBarrelZMin[layer];
    } else {
      theHisto = fieldEndcapHistos[layer];
      theBinWidth = fieldEndcapBinWidth[layer];
      theXMin = fieldEndcapRMin[layer];
    }
    
    // Find the relevant bin
    double x = fabs(coord);
    unsigned bin = (unsigned) ((x-theXMin)/theBinWidth) + 1; // TH1: bin 0 == underflows 
    double x1 = theXMin + (bin-0.5)*theBinWidth;
    double x2 = x1+theBinWidth;      

    // Determine the field
    double field1 = theHisto->GetBinContent(bin);
    double field2 = theHisto->GetBinContent(bin+1);

    return field1 + (field2-field1) * (x-x1)/(x2-x1);

  }

}

double 
MagneticFieldMap::inKGaussZ(const GlobalPoint& gp) const {

    return inTeslaZ(gp)/10.;

}

double 
MagneticFieldMap::inInverseGeVZ(const GlobalPoint& gp) const {

   return inKGaussZ(gp) * 2.99792458e-4;

}

