//--------------------------------------------------------------------------------------
// pch.h
//
// Header for standard system include files.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <WinSDKVer.h>
#define _WIN32_WINNT 0x0A00
#include <SDKDDKVer.h>

// Use the C++ standard templated min/max
#define NOMINMAX







#include "..\..\..\AmazeUI\AmazeUI-D12\UI\UIApplication.h"
#include "..\..\..\AmazeUI\AmazeUI-D12\UI\UIDXFoundation.h"
#include "..\..\..\AmazeUI\AmazeUI-D12\UI\UIWindow.h"
#include "..\..\..\AmazeUI\AmazeUI-D12\UI\UIElement.h"
#include "..\..\..\AmazeUI\AmazeUI-D12\UI\UIWidget.h"


#include "..\..\..\AmazeUI\AmazeUI-D12\ThirdParty\json.hpp"


#define VECTOR_DOUBLE std::vector<double>
#define VECTOR_INT std::vector<int>
#define VECTOR_STRING std::vector<std::string>