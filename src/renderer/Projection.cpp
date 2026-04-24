// 3D projection -- sub_046CAE (E1:9530), sub_046DA6 (E1:9675)

#include "renderer/Projection.h"
#include "renderer/LogMath.h"

// Outdoor projection (sub_046DA6, E1:9675)
// 3x3 matrix multiply -> perspective divide -> FOV scale -> screen coords

static ProjectedVertex ProjectVertexLogFloatIndoor(uint32_t lfX, uint32_t lfY,
												   uint32_t lfZ,
												   const Camera &cam);

ProjectedVertex ProjectVertex(int32_t worldX, int32_t worldY, int32_t worldZ,
							  const Camera &cam)
{
	// sub_046CAE: camera-relative + IntToLogFloat
	int32_t dx = worldX - cam.posX;
	int32_t dy = worldY - cam.posY;
	int32_t dz = worldZ - cam.posZ;

	uint32_t lfX = IntToLogFloat(dx);
	uint32_t lfY = IntToLogFloat(dy);
	uint32_t lfZ = IntToLogFloat(dz);

	// E1:9676-9679: sub_046DA6 dispatch -- indoor branch
	if (cam.renderMode != 0)
		return ProjectVertexLogFloatIndoor(lfX, lfY, lfZ, cam);

	ProjectedVertex result;
	result.screenX = 0;
	result.screenY = 0;
	result.visFlags = 0x8000; // default: behind camera
	result.camX = 0;
	result.camY = 0;
	result.camZ = 0;
	result.xzRatio = 0;
	result.yzRatio = 0;
	result.xzSign = 0;
	result.yzSign = 0;

	// sub_046DA6: outdoor matrix multiply
	// Compute ALL THREE camera-space coordinates unconditionally

	// Camera-space Z (depth): M00*dX + M01*dY + M02*dZ
	uint32_t t0 = LogMultiply(cam.matrix[0], lfX);
	uint32_t t1 = LogMultiply(cam.matrix[1], lfY);
	uint32_t cZ = LogFloatAdd(t0, t1);
	uint32_t t2 = LogMultiply(cam.matrix[2], lfZ);
	cZ = LogFloatAdd(cZ, t2);

	// Camera-space X (horizontal): M10*dX + M11*dY + M12*dZ
	t0 = LogMultiply(cam.matrix[3], lfX);
	t1 = LogMultiply(cam.matrix[4], lfY);
	uint32_t cX = LogFloatAdd(t0, t1);
	t2 = LogMultiply(cam.matrix[5], lfZ);
	cX = LogFloatAdd(cX, t2);

	// Camera-space Y (vertical): M20*dX + M21*dY + M22*dZ
	t0 = LogMultiply(cam.matrix[6], lfX);
	t1 = LogMultiply(cam.matrix[7], lfY);
	uint32_t cY = LogFloatAdd(t0, t1);
	t2 = LogMultiply(cam.matrix[8], lfZ);
	cY = LogFloatAdd(cY, t2);

	result.camX = cX;
	result.camY = cY;
	result.camZ = cZ;

	// E1:9818: ANDI.W #$8000,D0 -- behind-camera check
	int16_t camZMant = static_cast<int16_t>(cZ & 0xFFFF);
	if (camZMant < 0)
	{
		result.visFlags = 0x8000;
		return result;
	}

	// E1:9819: store visibility = $0000 (visible so far)
	result.visFlags = 0x0000;

	// Perspective divide (log-float ratios)
	uint32_t perspX = LogDivide(cX, cZ);
	uint32_t perspY = LogDivide(cY, cZ);
	result.xzRatio = perspX;
	result.yzRatio = perspY;

	// E1:10780-10785: store sign words (mantissa of perspX/perspY
	// before FOV scale, used by rendering attribute key)
	result.xzSign = static_cast<uint16_t>(perspX);
	result.yzSign = static_cast<uint16_t>(perspY);

	// FOV scale + log-to-screen + centre offset (X)
	// E1:9975-9989: the original's BMI at E1:9977 jumps directly to
	// ADDQ #1 (set vis flag) when LogFloatToScreen overflows, skipping
	// the screen coord store.  We must do the same -- an overflow value
	// would wrap to a plausible-looking coordinate after truncation
	perspX += 0x00070000;
	int32_t sx = LogFloatToScreen(perspX);
	if (sx == 0x7FFFFFFF)
	{
		result.visFlags |= 0x0001;
	}
	else
	{
		sx += cam.screenCenterX;
		result.screenX = static_cast<int16_t>(sx);
		if (static_cast<uint16_t>(sx) > 0x013F)
			result.visFlags |= 0x0001;
	}

	// FOV scale + log-to-screen + centre offset (Y)
	perspY += 0x00070000;
	int32_t sy = LogFloatToScreen(perspY);
	if (sy == 0x7FFFFFFF)
	{
		result.visFlags |= 0x0002;
	}
	else
	{
		sy += cam.screenCenterY;
		result.screenY = static_cast<int16_t>(sy);
		if (static_cast<uint16_t>(sy) > 0x0087)
			result.visFlags |= 0x0002;
	}

	return result;
}

// Indoor projection (loc_0471FE, E1:10166-10393)
// Yaw-only 2D rotation using sinHeading/cosHeading

static inline uint32_t NegMantissa(uint32_t lf)
{
	// Matches the 68000 NEG.W on the low 16 bits, leaving the
	// exponent high word untouched
	int16_t mant = static_cast<int16_t>(lf & 0xFFFFu);
	return (lf & 0xFFFF0000u) |
		   static_cast<uint16_t>(static_cast<int16_t>(-mant));
}

static ProjectedVertex ProjectVertexLogFloatIndoor(uint32_t lfX, uint32_t lfY,
												   uint32_t lfZ,
												   const Camera &cam)
{
	ProjectedVertex result{};
	result.screenX = 0;
	result.screenY = 0;
	result.visFlags = 0x8000;

	// E1:10167-10240: Z_cam = -(sinHeading*X + cosHeading*Z)
	//   D0 = constB*X, D1 = constA*Z, sort so D0 >= D1, LogFloatAdd,
	//   AND $FFFE, NEG.W
	uint32_t zB = LogMultiply(cam.sinHeading, lfX);
	uint32_t zA = LogMultiply(cam.cosHeading, lfZ);
	uint32_t cZ = LogFloatAdd(zB, zA);
	cZ &= 0xFFFFFFFEu;
	cZ = (cZ & 0xFFFF0000u) | static_cast<uint16_t>(static_cast<int16_t>(
								  -static_cast<int16_t>(cZ & 0xFFFF)));

	result.camZ = cZ;

	// E1:10241-10242: A4 = cZ & $8000 -- behind-camera sign flag
	bool behind = (cZ & 0x8000u) != 0;

	// E1:10243-10319: X_cam = mirror_flip(cosHeading*X - sinHeading*Z)
	//   constA*X, then constB*Z with NEG.W, LogFloatAdd, AND $FFFE,
	//   EOR/SUB mirrorMask
	uint32_t xA = LogMultiply(cam.cosHeading, lfX);
	uint32_t xBneg = NegMantissa(LogMultiply(cam.sinHeading, lfZ));
	uint32_t cX = LogFloatAdd(xA, xBneg);
	cX &= 0xFFFFFFFEu;
	uint16_t mirror = cam.mirrorMask;
	uint16_t cxLow = static_cast<uint16_t>(cX);
	cxLow ^= mirror;
	cxLow = static_cast<uint16_t>(cxLow - mirror);
	cX = (cX & 0xFFFF0000u) | cxLow;
	result.camX = cX;

	// E1:10355: NEG.W D4 -- convert world-up to screen-down
	uint32_t cY = NegMantissa(lfY);
	result.camY = cY;

	if (behind)
	{
		result.visFlags = 0x8000;
		return result;
	}

	// Screen X: perspX = cX / cZ + $062332 depth bias, then
	// log-to-linear, then + screenCenterX
	uint32_t perspX = LogDivide(cX, cZ);
	result.xzRatio = perspX;
	result.xzSign = static_cast<uint16_t>(perspX);
	perspX += 0x00070000;
	int32_t sx = LogFloatToScreen(perspX);
	result.visFlags = 0;
	if (sx == 0x7FFFFFFF)
	{
		result.visFlags |= 0x0001;
	}
	else
	{
		sx += cam.screenCenterX;
		result.screenX = static_cast<int16_t>(sx);
		if (static_cast<uint16_t>(sx) > 0x013F)
			result.visFlags |= 0x0001;
	}

	// Screen Y
	uint32_t perspY = LogDivide(cY, cZ);
	result.yzRatio = perspY;
	result.yzSign = static_cast<uint16_t>(perspY);
	perspY += 0x00070000;
	int32_t sy = LogFloatToScreen(perspY);
	if (sy == 0x7FFFFFFF)
	{
		result.visFlags |= 0x0002;
	}
	else
	{
		sy += cam.screenCenterY;
		result.screenY = static_cast<int16_t>(sy);
		if (static_cast<uint16_t>(sy) > 0x0087)
			result.visFlags |= 0x0002;
	}

	return result;
}

// Projection from pre-computed log-floats
// Used by the road drawer (sub_0444A2) which pre-computes the
// camera-relative log-float coordinates externally

ProjectedVertex ProjectVertexLogFloat(uint32_t lfX, uint32_t lfY, uint32_t lfZ,
									  const Camera &cam)
{
	// E1:9676-9679: sub_046DA6 dispatches to loc_0471FE when renderMode
	// is set or buildingID is negative.  Our outdoor loop never hits
	// the latter case, so only the renderMode branch is ported here
	if (cam.renderMode != 0)
		return ProjectVertexLogFloatIndoor(lfX, lfY, lfZ, cam);

	ProjectedVertex result;
	result.screenX = 0;
	result.screenY = 0;
	result.visFlags = 0x8000;
	result.camX = 0;
	result.camY = 0;
	result.camZ = 0;
	result.xzRatio = 0;
	result.yzRatio = 0;
	result.xzSign = 0;
	result.yzSign = 0;

	uint32_t t0 = LogMultiply(cam.matrix[0], lfX);
	uint32_t t1 = LogMultiply(cam.matrix[1], lfY);
	uint32_t cZ = LogFloatAdd(t0, t1);
	uint32_t t2 = LogMultiply(cam.matrix[2], lfZ);
	cZ = LogFloatAdd(cZ, t2);

	t0 = LogMultiply(cam.matrix[3], lfX);
	t1 = LogMultiply(cam.matrix[4], lfY);
	uint32_t cX = LogFloatAdd(t0, t1);
	t2 = LogMultiply(cam.matrix[5], lfZ);
	cX = LogFloatAdd(cX, t2);

	t0 = LogMultiply(cam.matrix[6], lfX);
	t1 = LogMultiply(cam.matrix[7], lfY);
	uint32_t cY = LogFloatAdd(t0, t1);
	t2 = LogMultiply(cam.matrix[8], lfZ);
	cY = LogFloatAdd(cY, t2);

	result.camX = cX;
	result.camY = cY;
	result.camZ = cZ;

	int16_t camZMant = static_cast<int16_t>(cZ & 0xFFFF);
	if (camZMant < 0)
	{
		result.visFlags = 0x8000;
		return result;
	}

	result.visFlags = 0x0000;

	uint32_t perspX = LogDivide(cX, cZ);
	uint32_t perspY = LogDivide(cY, cZ);
	result.xzRatio = perspX;
	result.yzRatio = perspY;
	result.xzSign = static_cast<uint16_t>(perspX);
	result.yzSign = static_cast<uint16_t>(perspY);

	perspX += 0x00070000;
	int32_t sx = LogFloatToScreen(perspX);
	if (sx == 0x7FFFFFFF)
	{
		result.visFlags |= 0x0001;
	}
	else
	{
		sx += cam.screenCenterX;
		result.screenX = static_cast<int16_t>(sx);
		if (static_cast<uint16_t>(sx) > 0x013F)
			result.visFlags |= 0x0001;
	}

	perspY += 0x00070000;
	int32_t sy = LogFloatToScreen(perspY);
	if (sy == 0x7FFFFFFF)
	{
		result.visFlags |= 0x0002;
	}
	else
	{
		sy += cam.screenCenterY;
		result.screenY = static_cast<int16_t>(sy);
		if (static_cast<uint16_t>(sy) > 0x0087)
			result.visFlags |= 0x0002;
	}

	return result;
}
