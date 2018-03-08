//
// Created by kuroneko on 2/03/2018.
//

#ifndef XPMPPLANE_H
#define XPMPPLANE_H

#include "XPMPMultiplayerVars.h"
#include "PlaneType.h"
#include "CullInfo.h"

class XPMPPlane {
private:
	// world state
	PlaneType			mPlaneType;

	int 				mPosAge;
	XPMPPlanePosition_t	mPosition;

	int 				mSurfaceAge;
	XPMPPlaneSurfaces_t	mSurface;

	int 					mSurveillanceAge;
	XPMPPlaneSurveillance_t	mSurveillance;

	// rendering data
	CSL *				mCSL;
	int					mMatchQuality;

	friend void	XPMP2DefaultPlaneRenderer(int is_blend);
	friend void Render_PrepLists();
public:
	XPMPPlane();
	virtual ~XPMPPlane();

	void setType(const PlaneType &type);
	void setCSL(CSL *csl);
	void setCSL(const PlaneType &type);
	void updateCSL();
	/** upgradeCSL works mostly like setCSL, only it only takes hold if the new
	 * CSL is a higher quality match than the old one.
	 *
	 * @param type
	 * @return true if the type was changed, false otherwise.
	 */
	bool upgradeCSL(const PlaneType &type);
	int  getMatchQuality();

	void updatePosition(const XPMPPlanePosition_t &newPosition);
	void updateSurfaces(const XPMPPlaneSurfaces_t &newSurfaces);
	void updateSurveillance(const XPMPPlaneSurveillance_t &newSurveillance);

	/** Updates the specific plane's instance data and prepares it's tcas
	 * (and culling flags for selfrendered models)
	 *
	 * @param gl_camera the CullInfo from the rendering loop
	 * @returns the square of the distance from the camera
	 */
	float doInstanceUpdate(const CullInfo &gl_camera);

	// instanceData is public for the convenience of the main render loop only.
	CSLInstanceData *	mInstanceData;
};


#endif //XPMPPLANE_H
