#pragma once

// Selects Mercenary or Second City data based on SECOND_CITY
// Both datasets compile in; SC namespaces are aliased to gen_e*

#include "DevUtils.h"

#if SECOND_CITY

#include "data/Gen_S0Data.h"
#include "data/Gen_S1Data.h"
#include "data/Gen_S2Data.h"
#include "data/Gen_S3Data.h"
#include "data/Gen_S4Data.h"
#include "data/Gen_SecondLoaderData.h"

namespace gen_e0 = gen_s0;
namespace gen_e1 = gen_s1;
namespace gen_e2 = gen_s2;
namespace gen_e3 = gen_s3;
namespace gen_e4 = gen_s4;
namespace gen_loader = gen_second_loader;

#else

#include "data/Gen_E0Data.h"
#include "data/Gen_E1Data.h"
#include "data/Gen_E2Data.h"
#include "data/Gen_E3Data.h"
#include "data/Gen_E4Data.h"
#include "data/Gen_LoaderData.h"

#endif
