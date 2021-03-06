/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 TELEMATICS LAB, DEE - Politecnico di Bari
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Giuseppe Piro  <g.piro@poliba.it>
 *         Nicola Baldo <nbaldo@cttc.es>
 */


#include <map>
#include <cmath>
#include <ns3/log.h>
#include <ns3/fatal-error.h>
#include <ns3/string.h>
#include <ns3/abort.h>

#include "mmwave-spectrum-value-helper.h"

namespace std {

ostream&
operator << (ostream& os, const vector<int>& v)
{
  vector<int>::const_iterator it = v.begin ();
  while (it != v.end ())
    {
      os << *it << " ";
      ++it;
    }
  os << endl;
  return os;
}

}
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("mmWaveSpectrumValueHelper");

Ptr<SpectrumModel>
mmWaveSpectrumValueHelper::GetSpectrumModel (Ptr<MmWavePhyMacCommon> ptrConfig)
{
  NS_LOG_FUNCTION (ptrConfig->GetCentreFrequency() << (uint32_t) ptrConfig->GetTotalNumChunk());
  Ptr<SpectrumModel> model;

  double fc = ptrConfig->GetCentreFrequency ();
  double f = 0.00;

  NS_ASSERT_MSG (fc != 0 , "The carrier frequency cannot be set to 0");

  f = fc - (ptrConfig->GetTotalNumChunk() * ptrConfig->GetChunkWidth() / 2.0);

  Bands rbs; // A vector representing each resource block
  for (uint8_t numrb = 0; numrb < ptrConfig->GetTotalNumChunk(); ++numrb)
  {
	  BandInfo rb;
	  rb.fl = f;
	  f += ptrConfig->GetChunkWidth()/2;
	  rb.fc = f;
	  f += ptrConfig->GetChunkWidth()/2;
	  rb.fh = f;

	  rbs.push_back (rb);
  }
  model = Create<SpectrumModel> (rbs);
  return model;
}

Ptr<SpectrumValue> 
mmWaveSpectrumValueHelper::CreateTxPowerSpectralDensity (Ptr<MmWavePhyMacCommon> ptrConfig, double powerTx, std::vector <int> activeRbs)
{
    Ptr<SpectrumModel> model = GetSpectrumModel (ptrConfig);
    Ptr<SpectrumValue> txPsd = Create <SpectrumValue> (model);

    double powerTxW = std::pow (10., (powerTx - 30) / 10);

    double txPowerDensity = 0;
    txPowerDensity = (powerTxW / (ptrConfig->GetSystemBandwidth()));

    for (std::vector <int>::iterator it = activeRbs.begin (); it != activeRbs.end (); it++)
    {
        int rbId = (*it);
        (*txPsd)[rbId] = txPowerDensity;
    }

    NS_LOG_LOGIC (*txPsd);

    return txPsd;

}

Ptr<SpectrumValue>
mmWaveSpectrumValueHelper::CreateTxPowerSpectralDensity (Ptr<MmWavePhyMacCommon> ptrConfig, double powerTx, std::map<int, double> powerTxMap, std::vector <int> activeRbs)
{
	Ptr<SpectrumValue> dummy;
	 return dummy;
}



Ptr<SpectrumValue>
mmWaveSpectrumValueHelper::CreateNoisePowerSpectralDensity (Ptr<MmWavePhyMacCommon> ptrConfig, double noiseFigure)
{
  Ptr<SpectrumModel> model = GetSpectrumModel (ptrConfig);
  Ptr<SpectrumValue> noisePsd = CreateNoisePowerSpectralDensity (noiseFigure, model);
  return noisePsd;
}

Ptr<SpectrumValue>
mmWaveSpectrumValueHelper::CreateNoisePowerSpectralDensity (double noiseFigureDb, Ptr<SpectrumModel> spectrumModel)
{
  NS_LOG_FUNCTION (noiseFigureDb << spectrumModel);
  const double kT_dBm_Hz = -174.0;  // dBm/Hz
  double kT_W_Hz = std::pow (10.0, (kT_dBm_Hz - 30) / 10.0);
  double noiseFigureLinear = std::pow (10.0, noiseFigureDb / 10.0);
  double noisePowerSpectralDensity =  kT_W_Hz * noiseFigureLinear;

  Ptr<SpectrumValue> noisePsd = Create <SpectrumValue> (spectrumModel);
  (*noisePsd) = noisePowerSpectralDensity;
  return noisePsd;
}

} // namespace ns3
