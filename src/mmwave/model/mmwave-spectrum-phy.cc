/*
 * mmwave-spectrum-phy.cc
 *
 *  Created on: Nov 5, 2014
 *      Author: sourjya
 */

#include <ns3/object-factory.h>
#include <ns3/log.h>
#include <ns3/ptr.h>
#include <ns3/boolean.h>
#include <cmath>
#include <ns3/simulator.h>
#include <ns3/trace-source-accessor.h>
#include <ns3/antenna-model.h>
#include "mmwave-spectrum-phy.h"
#include "mmwave-phy-mac-common.h"
#include <ns3/mmwave-enb-net-device.h>
#include <ns3/mmwave-ue-net-device.h>
#include <ns3/mmwave-ue-phy.h>
#include "mmwave-radio-bearer-tag.h"
#include <stdio.h>
#include <ns3/double.h>
#include <ns3/mmwave-mi-error-model.h>


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MmWaveSpectrumPhy");

NS_OBJECT_ENSURE_REGISTERED (MmWaveSpectrumPhy);

static const double EffectiveCodingRate[29] = {
  0.08,
  0.1,
  0.11,
  0.15,
  0.19,
  0.24,
  0.3,
  0.37,
  0.44,
  0.51,
  0.3,
  0.33,
  0.37,
  0.42,
  0.48,
  0.54,
  0.6,
  0.43,
  0.45,
  0.5,
  0.55,
  0.6,
  0.65,
  0.7,
  0.75,
  0.8,
  0.85,
  0.89,
  0.92
};

MmWaveSpectrumPhy::MmWaveSpectrumPhy()
	:m_state(IDLE)
{
	m_interferenceData = CreateObject<mmWaveInterference> ();
	m_random = CreateObject<UniformRandomVariable> ();
	m_random->SetAttribute ("Min", DoubleValue (0.0));
	m_random->SetAttribute ("Max", DoubleValue (1.0));
}
MmWaveSpectrumPhy::~MmWaveSpectrumPhy()
{

}

TypeId
MmWaveSpectrumPhy::GetTypeId(void)
{
	static TypeId
		tid =
	    TypeId ("ns3::MmWaveSpectrumPhy")
	    .SetParent<NetDevice> ()
		.AddTraceSource ("ReportEnbTxRxPacketCount",
						 "The no. of packets received and transmitted by the Base Station",
						 MakeTraceSourceAccessor (&MmWaveSpectrumPhy::m_reportEnbPacketCount))
		.AddTraceSource ("ReportUeTxRxPacketCount",
						 "The no. of packets received and transmitted by the User Device",
						 MakeTraceSourceAccessor (&MmWaveSpectrumPhy::m_reportUePacketCount))
		.AddAttribute ("DataErrorModelEnabled",
										"Activate/Deactivate the error model of data (TBs of PDSCH and PUSCH) [by default is active].",
										BooleanValue (true),
									 MakeBooleanAccessor (&MmWaveSpectrumPhy::m_dataErrorModelEnabled),
										MakeBooleanChecker ())
		;

	return tid;
}
void
MmWaveSpectrumPhy::DoDispose()
{

}

void
MmWaveSpectrumPhy::SetDevice(Ptr<NetDevice> d)
{
	m_device = d;

	Ptr<MmWaveEnbNetDevice> enbNetDev =
			DynamicCast<MmWaveEnbNetDevice> (GetDevice ());

	if (enbNetDev != 0)
	{
		m_isEnb = true;
	}
	else
	{
		m_isEnb = false;
	}
}

Ptr<NetDevice>
MmWaveSpectrumPhy::GetDevice()
{
	return m_device;
}

void
MmWaveSpectrumPhy::SetMobility (Ptr<MobilityModel> m)
{
	m_mobility = m;
}

Ptr<MobilityModel>
MmWaveSpectrumPhy::GetMobility ()
{
	return m_mobility;
}

void
MmWaveSpectrumPhy::SetChannel (Ptr<SpectrumChannel> c)
{
	m_channel = c;
}

Ptr<const SpectrumModel>
MmWaveSpectrumPhy::GetRxSpectrumModel () const
{
	return m_rxSpectrumModel;
}

Ptr<AntennaModel>
MmWaveSpectrumPhy::GetRxAntenna ()
{
	return m_anetnna;
}

void
MmWaveSpectrumPhy::SetAntenna (Ptr<AntennaModel> a)
{
	m_anetnna = a;
}

void
MmWaveSpectrumPhy::SetState (State newState)
{
  ChangeState (newState);
}

void
MmWaveSpectrumPhy::ChangeState (State newState)
{
  NS_LOG_LOGIC (this << " state: " << m_state << " -> " << newState);
  m_state = newState;
}


void
MmWaveSpectrumPhy::SetNoisePowerSpectralDensity(Ptr<const SpectrumValue> noisePsd)
{
	NS_LOG_FUNCTION (this << noisePsd);
	NS_ASSERT (noisePsd);
	m_rxSpectrumModel = noisePsd->GetSpectrumModel ();
	m_interferenceData->SetNoisePowerSpectralDensity (noisePsd);

}

void
MmWaveSpectrumPhy::SetTxPowerSpectralDensity (Ptr<SpectrumValue> TxPsd)
{
	m_txPsd = TxPsd;
}

void
MmWaveSpectrumPhy::SetPhyRxDataEndOkCallback (MmWavePhyRxDataEndOkCallback c)
{
	m_phyRxDataEndOkCallback = c;
}


void
MmWaveSpectrumPhy::SetPhyRxCtrlEndOkCallback (MmWavePhyRxCtrlEndOkCallback c)
{
  m_phyRxCtrlEndOkCallback = c;
}

void
MmWaveSpectrumPhy::AddExpectedTb (uint16_t rnti, uint8_t ndi, uint16_t size, uint8_t mcs, std::vector<int> map, uint8_t harqId, uint8_t rv, bool downlink)
{
	//layer = layer;
	ExpectedTbMap_t::iterator it;
	it = m_expectedTbs.find (rnti);
	if (it != m_expectedTbs.end ())
	{
		m_expectedTbs.erase (it);
	}
	// insert new entry
	ExpectedTbInfo_t tbInfo = {ndi, size, mcs, map, harqId, rv, 0.0, downlink, false, false};
	m_expectedTbs.insert (std::pair<uint16_t, ExpectedTbInfo_t> (rnti,tbInfo));
}

void
MmWaveSpectrumPhy::AddExpectedTb (uint16_t rnti, uint16_t size, uint8_t mcs, std::vector<int> map, bool downlink)
{
	//layer = layer;
	ExpectedTbMap_t::iterator it;
	it = m_expectedTbs.find (rnti);
	if (it != m_expectedTbs.end ())
	{
		m_expectedTbs.erase (it);
	}
	// insert new entry
	ExpectedTbInfo_t tbInfo = {1, size, mcs, map, 0, 0, 0.0, downlink, false, false};
	m_expectedTbs.insert (std::pair<uint16_t, ExpectedTbInfo_t> (rnti,tbInfo));
}

void
MmWaveSpectrumPhy::SetPhyDlHarqFeedbackCallback (MmWavePhyDlHarqFeedbackCallback c)
{
  NS_LOG_FUNCTION (this);
  m_phyDlHarqFeedbackCallback = c;
}

void
MmWaveSpectrumPhy::SetPhyUlHarqFeedbackCallback (MmWavePhyUlHarqFeedbackCallback c)
{
  NS_LOG_FUNCTION (this);
  m_phyUlHarqFeedbackCallback = c;
}

void
MmWaveSpectrumPhy::StartRx (Ptr<SpectrumSignalParameters> params)
{

	NS_LOG_FUNCTION(this);

	Ptr<MmWaveEnbNetDevice> EnbTx =
			DynamicCast<MmWaveEnbNetDevice> (params->txPhy->GetDevice ());
	Ptr<MmWaveEnbNetDevice> enbRx =
			DynamicCast<MmWaveEnbNetDevice> (GetDevice ());
	if((EnbTx != 0 && enbRx != 0) || (EnbTx == 0 && enbRx == 0))
	{
		NS_LOG_INFO ("BS to BS or UE to UE transmission neglected.");
		return;
	}

	Ptr<MmwaveSpectrumSignalParametersDataFrame> mmwaveDataRxParams =
			DynamicCast<MmwaveSpectrumSignalParametersDataFrame> (params);

	if (mmwaveDataRxParams!=0)
	{
		bool isAllocated = true;
		Ptr<MmWaveUeNetDevice> ueRx = 0;
		ueRx = DynamicCast<MmWaveUeNetDevice> (GetDevice ());

		if ((ueRx!=0) && (ueRx->GetPhy ()->IsReceptionEnabled () == false))
		{
			isAllocated = false;
		}

		if (isAllocated)
		{
			m_interferenceData->AddSignal (mmwaveDataRxParams->psd, mmwaveDataRxParams->duration);
			if(mmwaveDataRxParams->cellId == m_cellId)
			{
				//m_interferenceData->AddSignal (mmwaveDataRxParams->psd, mmwaveDataRxParams->duration);
				StartRxData (mmwaveDataRxParams);
			}
			/*
			else
			{
				if (ueRx != 0)
				{
					m_interferenceData->AddSignal (mmwaveDataRxParams->psd, mmwaveDataRxParams->duration);
				}
			}
			*/
		}
	}
	else
	{
		Ptr<MmWaveSpectrumSignalParametersDlCtrlFrame> DlCtrlRxParams =
				DynamicCast<MmWaveSpectrumSignalParametersDlCtrlFrame> (params);
		if (DlCtrlRxParams!=0)
		{
			if (DlCtrlRxParams->cellId == m_cellId)
			{
				StartRxCtrl (params);
			}
			else
			{
				// Do nothing
			}
		}
	}
}

void
MmWaveSpectrumPhy::StartRxData (Ptr<MmwaveSpectrumSignalParametersDataFrame> params)
{
	m_interferenceData->StartRx (params->psd);

	NS_LOG_FUNCTION(this);

	Ptr<MmWaveEnbNetDevice> enbRx =
				DynamicCast<MmWaveEnbNetDevice> (GetDevice ());
	Ptr<MmWaveUeNetDevice> ueRx =
				DynamicCast<MmWaveUeNetDevice> (GetDevice ());
	switch(m_state)
	{
	case TX:
		NS_FATAL_ERROR("Cannot receive while transmitting");
		break;
	case RX_CTRL:
			NS_FATAL_ERROR("Cannot receive control in data period");
			break;
	case RX_DATA:
	case IDLE:
	{
		if (params->cellId == m_cellId)
		{
			if (m_rxPacketBurstList.empty())
			{
        NS_ASSERT (m_state == IDLE);
				// first transmission, i.e., we're IDLE and we start RX
				m_firstRxStart = Simulator::Now ();
				m_firstRxDuration = params->duration;
				NS_LOG_LOGIC (this << " scheduling EndRx with delay " << params->duration.GetSeconds () << "s");

				Simulator::Schedule (params->duration, &MmWaveSpectrumPhy::EndRxData, this);
			}
			else
			{
				NS_ASSERT (m_state == RX_DATA);
				// sanity check: if there are multiple RX events, they
				// should occur at the same time and have the same
				// duration, otherwise the interference calculation
				// won't be correct
				NS_ASSERT ((m_firstRxStart == Simulator::Now ()) && (m_firstRxDuration == params->duration));
			}

			ChangeState (RX_DATA);
			if (params->packetBurst && !params->packetBurst->GetPackets ().empty ())
			{
				m_rxPacketBurstList.push_back (params->packetBurst);
			}
			NS_LOG_DEBUG (this << " insert msgs " << params->ctrlMsgList.size ());
			m_rxControlMessageList.insert (m_rxControlMessageList.end (), params->ctrlMsgList.begin (), params->ctrlMsgList.end ());

			NS_LOG_LOGIC (this << " numSimultaneousRxEvents = " << m_rxPacketBurstList.size ());

			/* This section is used for trace */
			if (enbRx)
			{
				EnbPhyPacketCountParameter traceParam;
				traceParam.m_noBytes = (params->packetBurst)?params->packetBurst->GetSize ():0;
				traceParam.m_cellId = params->cellId;
				traceParam.m_isTx = false;
				traceParam.m_subframeno = enbRx->GetPhy ()->GetAbsoluteSubframeNo ();
				m_reportEnbPacketCount (traceParam);
			}
			else if (ueRx)
			{
				UePhyPacketCountParameter traceParam;
				traceParam.m_noBytes = (params->packetBurst)?params->packetBurst->GetSize ():0;
				traceParam.m_imsi = ueRx->GetImsi ();
				traceParam.m_isTx = false;
				traceParam.m_subframeno = ueRx->GetPhy ()->GetAbsoluteSubframeNo ();
				m_reportUePacketCount (traceParam);
			}
		}
		else
		{
			NS_LOG_LOGIC (this << " not in sync with this signal (cellId="
			              << params->cellId  << ", m_cellId=" << m_cellId << ")");
		}
	}
	break;
	default:
		NS_FATAL_ERROR("Programming Error: Unknown State");
	}
}

void
MmWaveSpectrumPhy::StartRxCtrl (Ptr<SpectrumSignalParameters> params)
{
	NS_LOG_FUNCTION (this);
	// RDF: method currently supports Downlink control only!
	switch (m_state)
	{
		case TX:
			NS_FATAL_ERROR ("Cannot RX while TX: according to FDD channel access, the physical layer for transmission cannot be used for reception");
			break;
		case RX_DATA:
			NS_FATAL_ERROR ("Cannot RX data while receiving control");
			break;
		case RX_CTRL:
		case IDLE:
		{
			// the behavior is similar when we're IDLE or RX because we can receive more signals
			// simultaneously (e.g., at the eNB).
			Ptr<MmWaveSpectrumSignalParametersDlCtrlFrame> dlCtrlRxParams = \
					DynamicCast<MmWaveSpectrumSignalParametersDlCtrlFrame> (params);
			// To check if we're synchronized to this signal, we check for the CellId
			uint16_t cellId = 0;
			if (dlCtrlRxParams != 0)
			{
				cellId = dlCtrlRxParams->cellId;
			}
			else
			{
				NS_LOG_ERROR ("SpectrumSignalParameters type not supported");
			}
			// check presence of PSS for UE measuerements
			/*if (dlCtrlRxParams->pss == true)
			{
				SpectrumValue pssPsd = *params->psd;
				if (!m_phyRxPssCallback.IsNull ())
				{
					m_phyRxPssCallback (cellId, params->psd);
				}
			}*/
			if (cellId  == m_cellId)
			{
				if(m_state == RX_CTRL)
				{
					Ptr<MmWaveUeNetDevice> ueRx =
									DynamicCast<MmWaveUeNetDevice> (GetDevice ());
					if (ueRx)
					{
						NS_FATAL_ERROR ("UE already receiving control data from serving cell");
					}
					NS_ASSERT ((m_firstRxStart == Simulator::Now ())
					           && (m_firstRxDuration == params->duration));
				}
				NS_LOG_LOGIC (this << " synchronized with this signal (cellId=" << cellId << ")");
				if (m_state == IDLE)
				{
					// first transmission, i.e., we're IDLE and we start RX
					NS_ASSERT (m_rxControlMessageList.empty ());
					m_firstRxStart = Simulator::Now ();
					m_firstRxDuration = params->duration;
					NS_LOG_LOGIC (this << " scheduling EndRx with delay " << params->duration);
					// store the DCIs
					m_rxControlMessageList = dlCtrlRxParams->ctrlMsgList;
					Simulator::Schedule (params->duration, &MmWaveSpectrumPhy::EndRxCtrl, this);
					ChangeState (RX_CTRL);
				}
				else
				{
					m_rxControlMessageList.insert (m_rxControlMessageList.end (), dlCtrlRxParams->ctrlMsgList.begin (), dlCtrlRxParams->ctrlMsgList.end ());
				}

			}
			break;
	}
	default:
	{
	    NS_FATAL_ERROR ("unknown state");
	    break;
	}
	}
}

void
MmWaveSpectrumPhy::EndRxData ()
{
	m_interferenceData->EndRx();

	NS_ASSERT(m_state = RX_DATA);
	ExpectedTbMap_t::iterator itTb = m_expectedTbs.begin ();
	while (itTb != m_expectedTbs.end ())
	{
		if ((m_dataErrorModelEnabled)&&(m_rxPacketBurstList.size ()>0))
		{
			HarqProcessInfoList_t harqInfoList;
			if ((*itTb).second.ndi == 0)
			{
				// TB retxed: retrieve HARQ history
				uint16_t ulHarqId = 0;
				if ((*itTb).second.downlink)
				{
					harqInfoList = m_harqPhyModule->GetHarqProcessInfoDl ((*itTb).second.harqProcessId, 0);
				}
				else
				{
					harqInfoList = m_harqPhyModule->GetHarqProcessInfoUl ((*itTb).first, ulHarqId);
				}
			}

			TbStats_t tbStats = MmWaveMiErrorModel::GetTbDecodificationStats (m_sinrPerceived,
					(*itTb).second.rbBitmap, (*itTb).second.size, (*itTb).second.mcs, harqInfoList);
			(*itTb).second.mi = tbStats.mi;
			(*itTb).second.corrupt = m_random->GetValue () > tbStats.tbler ? false : true;
			NS_LOG_DEBUG (this << " RNTI " << (*itTb).first << " size " << (*itTb).second.size << " mcs " << (uint32_t)(*itTb).second.mcs << " bitmap " << (*itTb).second.rbBitmap.size () << " layer " << 0 << " TBLER " << tbStats.tbler << " corrupted " << (*itTb).second.corrupt);
		}
		itTb++;
	}

	Ptr<MmWaveEnbNetDevice> enbRx =
			DynamicCast<MmWaveEnbNetDevice> (GetDevice ());
	Ptr<MmWaveUeNetDevice> ueRx =
			DynamicCast<MmWaveUeNetDevice> (GetDevice ());

	std::map <uint16_t, DlHarqInfo> harqDlInfoMap;
	for (std::list<Ptr<PacketBurst> >::const_iterator i = m_rxPacketBurstList.begin ();
			i != m_rxPacketBurstList.end (); ++i)
	{
		for (std::list<Ptr<Packet> >::const_iterator j = (*i)->Begin (); j != (*i)->End (); ++j)
		{
			LteRadioBearerTag tag;

			if((*j)->PeekPacketTag (tag) == false)
			{
				if (enbRx)
				{
					NS_FATAL_ERROR ("No radio bearer tag found (eNB dest)");
				}
				else if (ueRx)
				{
					NS_FATAL_ERROR ("No radio bearer tag found (UE dest)");
				}
			}
			uint16_t rnti = tag.GetRnti ();
			uint16_t layer = 0;
			itTb = m_expectedTbs.find (rnti);
			if(itTb != m_expectedTbs.end ())
			{
				if (!(*itTb).second.corrupt)
				{
					m_phyRxDataEndOkCallback (*j);
				}
				else
				{
					//Drop Packet
					//					Time t =  Simulator::Now ();
					NS_LOG_INFO ("TB failed");
				}

				// send HARQ feedback (if not already done for this TB)
				if (!(*itTb).second.harqFeedbackSent)
				{
					(*itTb).second.harqFeedbackSent = true;
					if (!(*itTb).second.downlink)
					{
						UlHarqInfo harqUlInfo;
						harqUlInfo.m_rnti = rnti;
						harqUlInfo.m_tpc = 0;
						if ((*itTb).second.corrupt)
						{
							harqUlInfo.m_receptionStatus = UlHarqInfo::NotOk;
							NS_LOG_DEBUG (this << " RNTI " << rnti << " send UL-HARQ-NACK");
							m_harqPhyModule->UpdateUlHarqProcessStatus (rnti, (*itTb).second.mi, (*itTb).second.size, (*itTb).second.size / EffectiveCodingRate [(*itTb).second.mcs]);
						}
						else
						{
							harqUlInfo.m_receptionStatus = UlHarqInfo::Ok;
							NS_LOG_DEBUG (this << " RNTI " << rnti << " send UL-HARQ-ACK");
							m_harqPhyModule->ResetUlHarqProcessStatus (rnti, (*itTb).second.harqProcessId);
						}
						if (!m_phyUlHarqFeedbackCallback.IsNull ())
						{
							m_phyUlHarqFeedbackCallback (harqUlInfo);
						}
					}
					else
					{
						std::map <uint16_t, DlHarqInfo>::iterator itHarq = harqDlInfoMap.find (rnti);
						if (itHarq==harqDlInfoMap.end ())
						{
							DlHarqInfo harqDlInfo;
							harqDlInfo.m_harqStatus.resize (1, DlHarqInfo::NACK);
							harqDlInfo.m_rnti = rnti;
							harqDlInfo.m_harqProcessId = (*itTb).second.harqProcessId;
							if ((*itTb).second.corrupt)
							{
								harqDlInfo.m_harqStatus.at (0) = DlHarqInfo::NACK;
								NS_LOG_DEBUG (this << " RNTI " << rnti << " harqId " << (uint16_t)(*itTb).second.harqProcessId << " layer " <<(uint16_t)layer << " send DL-HARQ-NACK");
								m_harqPhyModule->UpdateDlHarqProcessStatus ((*itTb).second.harqProcessId, 0, (*itTb).second.mi, (*itTb).second.size, (*itTb).second.size / EffectiveCodingRate [(*itTb).second.mcs]);
							}
							else
							{

								harqDlInfo.m_harqStatus.at (0) = DlHarqInfo::ACK;
								NS_LOG_DEBUG (this << " RNTI " << rnti << " harqId " << (uint16_t)(*itTb).second.harqProcessId << " layer " <<(uint16_t)layer << " size " << (*itTb).second.size << " send DL-HARQ-ACK");
								m_harqPhyModule->ResetDlHarqProcessStatus ((*itTb).second.harqProcessId);
							}
							harqDlInfoMap.insert (std::pair <uint16_t, DlHarqInfo> (rnti, harqDlInfo));
						}
						else
						{
							if ((*itTb).second.corrupt)
							{
								(*itHarq).second.m_harqStatus.at (layer) = DlHarqInfo::NACK;
								NS_LOG_DEBUG (this << " RNTI " << rnti << " harqId " << (uint16_t)(*itTb).second.harqProcessId << " layer " <<(uint16_t)layer << " size " << (*itHarq).second.m_harqStatus.size () << " send DL-HARQ-NACK");
								m_harqPhyModule->UpdateDlHarqProcessStatus ((*itTb).second.harqProcessId, layer, (*itTb).second.mi, (*itTb).second.size, (*itTb).second.size / EffectiveCodingRate [(*itTb).second.mcs]);
							}
							else
							{
								NS_ASSERT_MSG (layer < (*itHarq).second.m_harqStatus.size (), " layer " << (uint16_t)layer);
								(*itHarq).second.m_harqStatus.at (layer) = DlHarqInfo::ACK;
								NS_LOG_DEBUG (this << " RNTI " << rnti << " harqId " << (uint16_t)(*itTb).second.harqProcessId << " layer " << (uint16_t)layer << " size " << (*itHarq).second.m_harqStatus.size () << " send DL-HARQ-ACK");
								m_harqPhyModule->ResetDlHarqProcessStatus ((*itTb).second.harqProcessId);
							}
						}
					} // end if ((*itTb).second.downlink) HARQ
				} // end if (!(*itTb).second.harqFeedbackSent)
			}
			else
			{
				//				NS_FATAL_ERROR ("End of the tbMap");
				// Packet is for other device
			}


		}
	}

	// send DL HARQ feedback to LtePhy
	std::map <uint16_t, DlHarqInfo>::iterator itHarq;
	for (itHarq = harqDlInfoMap.begin (); itHarq != harqDlInfoMap.end (); itHarq++)
	{
		if (!m_phyDlHarqFeedbackCallback.IsNull ())
		{
			m_phyDlHarqFeedbackCallback ((*itHarq).second);
		}
	}
	// forward control messages of this frame to MmWavePhy

	if (!m_rxControlMessageList.empty () && !m_phyRxCtrlEndOkCallback.IsNull ())
	{
		m_phyRxCtrlEndOkCallback (m_rxControlMessageList);
	}

	m_state = IDLE;
	m_rxPacketBurstList.clear ();
	m_expectedTbs.clear ();
	m_rxControlMessageList.clear ();
}

void
MmWaveSpectrumPhy::EndRxCtrl ()
{
	NS_ASSERT(m_state = RX_CTRL);

	// control error model not supported
  // forward control messages of this frame to LtePhy
  if (!m_rxControlMessageList.empty ())
    {
      if (!m_phyRxCtrlEndOkCallback.IsNull ())
        {
          m_phyRxCtrlEndOkCallback (m_rxControlMessageList);
        }
    }

	m_state = IDLE;
	m_rxControlMessageList.clear ();
}

bool
MmWaveSpectrumPhy::StartTxDataFrames (Ptr<PacketBurst> pb, std::list<Ptr<MmWaveControlMessage> > ctrlMsgList, Time duration, uint8_t slotInd)
{
	switch (m_state)
	{
	case RX_DATA:
  case RX_CTRL:
		NS_FATAL_ERROR ("cannot TX while RX: Cannot transmit while receiving");
		break;
	case TX:
		NS_FATAL_ERROR ("cannot TX while already Tx: Cannot transmit while a transmission is still on");
		break;
	case IDLE:
	{
		NS_ASSERT(m_txPsd);

		m_state = TX;
		Ptr<MmwaveSpectrumSignalParametersDataFrame> txParams = new MmwaveSpectrumSignalParametersDataFrame ();
		txParams->duration = duration;
		txParams->txPhy = this->GetObject<SpectrumPhy> ();
		txParams->psd = m_txPsd;
		txParams->packetBurst = pb;
		txParams->cellId = m_cellId;
		txParams->ctrlMsgList = ctrlMsgList;
		txParams->slotInd = slotInd;
		txParams->txAntenna = m_anetnna;

		NS_LOG_DEBUG ("ctrlMsgList.size () == " << txParams->ctrlMsgList.size ());

		/* This section is used for trace */
		Ptr<MmWaveEnbNetDevice> enbTx =
					DynamicCast<MmWaveEnbNetDevice> (GetDevice ());
		Ptr<MmWaveUeNetDevice> ueTx =
					DynamicCast<MmWaveUeNetDevice> (GetDevice ());
		if (enbTx)
		{
			EnbPhyPacketCountParameter traceParam;
			traceParam.m_noBytes = (txParams->packetBurst)?txParams->packetBurst->GetSize ():0;
			traceParam.m_cellId = txParams->cellId;
			traceParam.m_isTx = true;
			traceParam.m_subframeno = enbTx->GetPhy ()->GetAbsoluteSubframeNo ();
			m_reportEnbPacketCount (traceParam);
		}
		else if (ueTx)
		{
			UePhyPacketCountParameter traceParam;
			traceParam.m_noBytes = (txParams->packetBurst)?txParams->packetBurst->GetSize ():0;
			traceParam.m_imsi = ueTx->GetImsi ();
			traceParam.m_isTx = true;
			traceParam.m_subframeno = ueTx->GetPhy ()->GetAbsoluteSubframeNo ();
			m_reportUePacketCount (traceParam);
		}
		/**********************************/

		m_channel->StartTx (txParams);

		Simulator::Schedule (duration, &MmWaveSpectrumPhy::EndTx, this);
	}
	break;
	default:
		NS_LOG_FUNCTION (this<<"Programming Error. Code should not reach this point");
	}
	return true;
}

bool
MmWaveSpectrumPhy::StartTxDlControlFrames (std::list<Ptr<MmWaveControlMessage> > ctrlMsgList, Time duration)
{
	NS_LOG_LOGIC (this << " state: " << m_state);

	switch (m_state)
	{
  case RX_DATA:
	case RX_CTRL:
		NS_FATAL_ERROR ("cannot TX while RX: Cannot transmit while receiving");
		break;
	case TX:
		NS_FATAL_ERROR ("cannot TX while already Tx: Cannot transmit while a transmission is still on");
		break;
	case IDLE:
	{
		NS_ASSERT(m_txPsd);

		m_state = TX;

		Ptr<MmWaveSpectrumSignalParametersDlCtrlFrame> txParams = Create<MmWaveSpectrumSignalParametersDlCtrlFrame> ();
		txParams->duration = duration;
		txParams->txPhy = GetObject<SpectrumPhy> ();
		txParams->psd = m_txPsd;
		txParams->cellId = m_cellId;
		txParams->pss = true;
		txParams->ctrlMsgList = ctrlMsgList;
		txParams->txAntenna = m_anetnna;
		m_channel->StartTx (txParams);
		Simulator::Schedule (duration, &MmWaveSpectrumPhy::EndTx, this);
	}
	}
	return false;
}

void
MmWaveSpectrumPhy::EndTx()
{
	NS_ASSERT (m_state == TX);

	m_state = IDLE;
}

Ptr<SpectrumChannel>
MmWaveSpectrumPhy::GetSpectrumChannel ()
{
	return m_channel;
}

void
MmWaveSpectrumPhy::SetCellId (uint16_t cellId)
{
	m_cellId = cellId;
}


void
MmWaveSpectrumPhy::AddDataPowerChunkProcessor (Ptr<mmWaveChunkProcessor> p)
{
	m_interferenceData->AddPowerChunkProcessor (p);
}

void
MmWaveSpectrumPhy::AddDataSinrChunkProcessor (Ptr<mmWaveChunkProcessor> p)
{
	m_interferenceData->AddSinrChunkProcessor (p);
}

void
MmWaveSpectrumPhy::UpdateSinrPerceived (const SpectrumValue& sinr)
{
	NS_LOG_FUNCTION (this << sinr);
	m_sinrPerceived = sinr;
}

void
MmWaveSpectrumPhy::SetHarqPhyModule (Ptr<MmWaveHarqPhy> harq)
{
  m_harqPhyModule = harq;
}


}
