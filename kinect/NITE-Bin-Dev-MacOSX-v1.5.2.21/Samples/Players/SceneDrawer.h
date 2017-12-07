/*******************************************************************************
*                                                                              *
*   PrimeSense NITE 1.3 - Players Sample                                       *
*   Copyright (C) 2010 PrimeSense Ltd.                                         *
*                                                                              *
*******************************************************************************/

#ifndef XNV_POINT_DRAWER_H_
#define XNV_POINT_DRAWER_H_

#include <XnCppWrapper.h>
#include <string>
#include <fstream>
#include <ctime>
#include <vector>
#include "math.h"

static int count = 0;
static int classifyCount = 0;
static long eventTime = 0;

static XnSkeletonJointPosition prevHand;
static XnSkeletonJointPosition prevFoot;

static bool first = true;
static bool handAboveHip = false;
static bool handVelocity = false;
static bool footVelocity = false;

static std::vector<std::string> filesInUse;


void DrawDepthMap(const xn::DepthMetaData& dmd, const xn::SceneMetaData& smd, XnUserID player);

void DrawSkeleton(XnUserID player, float depth, std::string& label);

#endif
