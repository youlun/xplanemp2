//
// Created by kuroneko on 2/03/2018.
//

#include <algorithm>

#include <XPLMGraphics.h>
#include <XPLMProcessing.h>
#include <XPLMPlanes.h>
#include <XPLMDataAccess.h>

#include "XPMPMultiplayer.h"

#include "XPMPPlane.h"
#include "PlaneType.h"
#include "Renderer.h"
#include "CullInfo.h"
#include "TCASHack.h"
#include "CSLLibrary.h"

using namespace std;

XPMPPlane::XPMPPlane() :
	mPlaneType("", "", "")
{
	mCSL = nullptr;
	mInstanceData = nullptr;
	mPosAge = -1;
	mSurfaceAge = -1;
	mSurveillanceAge = -1;
}

XPMPPlane::~XPMPPlane()
{
	setCSL(nullptr);
}

void
XPMPPlane::setType(const PlaneType &type)
{
	if (mPlaneType != type) {
		mPlaneType = type;
		setCSL(nullptr);
	}
}

void
XPMPPlane::setCSL(CSL *csl)
{
	if (mCSL != csl) {
		if (mInstanceData) {
			delete mInstanceData;
			mInstanceData = nullptr;
		}
		mCSL = csl;
	}
}

void
XPMPPlane::updatePosition(const XPMPPlanePosition_t &newPosition)
{
	mPosAge = XPLMGetCycleNumber();
	memcpy(&mPosition, &newPosition, min(newPosition.size, sizeof(mPosition)));
}

void
XPMPPlane::updateSurfaces(const XPMPPlaneSurfaces_t &newSurfaces)
{
	mSurfaceAge = XPLMGetCycleNumber();
	memcpy(&mSurface, &newSurfaces, min(newSurfaces.size, sizeof(mSurface)));
}

void
XPMPPlane::updateSurveillance(const XPMPPlaneSurveillance_t &newSurveillance)
{
	mSurveillanceAge = XPLMGetCycleNumber();
	memcpy(&mSurveillance, &newSurveillance, min(newSurveillance.size, sizeof(mSurveillance)));
}

float
XPMPPlane::doInstanceUpdate(const CullInfo &gl_camera)
{
	if (mCSL) {
		double	lx,ly,lz;

		XPLMWorldToLocal(mPosition.lat, mPosition.lon, mPosition.elevation * kFtToMeters, &lx, &ly, &lz);
		XPLMPlaneDrawState_t planeState = {};

		planeState.structSize = sizeof(planeState);
		planeState.gearPosition = mSurface.gearPosition;
		planeState.flapRatio = mSurface.flapRatio;
		planeState.spoilerRatio = mSurface.spoilerRatio;
		planeState.speedBrakeRatio = mSurface.speedBrakeRatio;
		planeState.slatRatio = mSurface.slatRatio;
		planeState.wingSweep = mSurface.wingSweep;
		planeState.thrust = mSurface.thrust;
		planeState.yokePitch = mSurface.yokePitch;
		planeState.yokeHeading = mSurface.yokeHeading;
		planeState.yokeRoll = mSurface.yokeRoll;

		mCSL->updateInstance(
			gl_camera,
			lx,
			ly,
			lz,
			mPosition.roll,
			mPosition.heading,
			mPosition.pitch,
			&planeState,
			mSurface.lights,
			mInstanceData);

		if (mInstanceData == nullptr) {
			return 0.0;
		}
		// apply surveillance mode related masking to the TCAS inclusion record.
		if (mSurveillance.mode == xpmpTransponderMode_Standby) {
			mInstanceData->mTCAS = false;
		}
		// check for altitude - if difference exceeds a preconfigured limit, don't show
		double acft_alt = XPLMGetDatad(TCAS::gAltitudeRef) / kFtToMeters;
		double alt_diff = mPosition.elevation - acft_alt;
		if(alt_diff < 0) alt_diff *= -1;
		if(mSurveillance.mode != xpmpTransponderMode_Mode3A && alt_diff > MAX_TCAS_ALTDIFF) {
			mInstanceData->mTCAS = false;
		}
		if (mInstanceData->mTCAS) {
			// populate the global TCAS list
			TCAS::addPlane(mInstanceData->mDistanceSqr, lx, ly, lz, mSurveillance.mode != xpmpTransponderMode_Mode3A);
		}

		// do labels.
		if (!mInstanceData->mCulled && mInstanceData->mDistanceSqr <= (Render_LabelDistance * Render_LabelDistance)) {
			float tx, ty;

			gl_camera.ConvertTo2D(Render_LabelViewport, lx, ly, lz, 1.0, &tx, &ty);
			gLabelList.emplace_back(Label{
				static_cast<int>(tx), static_cast<int>(ty),	
				mInstanceData->mDistanceSqr,
				string(mPosition.label) 
			});
		}
		return mInstanceData->mDistanceSqr;
	}
	return 0.0;
}


void
XPMPPlane::setCSL(const PlaneType &type)
{
	setCSL(CSL_MatchPlane(type, &mMatchQuality, true));
}

void
XPMPPlane::updateCSL()
{
	setCSL(mPlaneType);
}

bool
XPMPPlane::upgradeCSL(const PlaneType &type)
{
	int local_matchquality;
	auto newCSL = CSL_MatchPlane(type, &local_matchquality, false);
	if (local_matchquality >= 0 && local_matchquality < mMatchQuality) {
		setCSL(newCSL);
		mMatchQuality = local_matchquality;
		return true;
	}
	return false;
}

int
XPMPPlane::getMatchQuality()
{
	return mMatchQuality;
}
