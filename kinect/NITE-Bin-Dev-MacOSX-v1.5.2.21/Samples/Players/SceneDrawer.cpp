/*******************************************************************************
*                                                                              *
*   PrimeSense NITE 1.3 - Players Sample                                       *
*   Copyright (C) 2010 PrimeSense Ltd.                                         *
*                                                                              *
*******************************************************************************/

#include "SceneDrawer.h"
#include <string>
#include <iostream>
#include <math.h>
#include <chrono>



#ifdef USE_GLUT
#if (XN_PLATFORM == XN_PLATFORM_MACOSX)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#else
#include "opengles.h"
#endif

extern xn::UserGenerator g_UserGenerator;
extern xn::DepthGenerator g_DepthGenerator;

#define MAX_DEPTH 10000
float g_pDepthHist[MAX_DEPTH];
unsigned int getClosestPowerOfTwo(unsigned int n)
{
	unsigned int m = 2;
	while (m < n) m <<= 1;

	return m;
}
GLuint initTexture(void** buf, int& width, int& height)
{
	GLuint texID = 0;
	glGenTextures(1, &texID);

	width = getClosestPowerOfTwo(width);
	height = getClosestPowerOfTwo(height);
	*buf = new unsigned char[width * height * 4];
	glBindTexture(GL_TEXTURE_2D, texID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	return texID;
}

GLfloat texcoords[8];
void DrawRectangle(float topLeftX, float topLeftY, float bottomRightX, float bottomRightY)
{
	GLfloat verts[8] = {	topLeftX, topLeftY,
	                        topLeftX, bottomRightY,
	                        bottomRightX, bottomRightY,
	                        bottomRightX, topLeftY
	                   };
	glVertexPointer(2, GL_FLOAT, 0, verts);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	//TODO: Maybe glFinish needed here instead - if there's some bad graphics crap
	glFlush();
}
void DrawTexture(float topLeftX, float topLeftY, float bottomRightX, float bottomRightY)
{
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, texcoords);

	DrawRectangle(topLeftX, topLeftY, bottomRightX, bottomRightY);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

XnFloat Colors[][3] =
{
	{0, 1, 1},
	{0, 0, 1},
	{0, 1, 0},
	{1, 1, 0},
	{1, 0, 0},
	{1, .5, 0},
	{.5, 1, 0},
	{0, .5, 1},
	{.5, 0, 1},
	{1, 1, .5},
	{1, 1, 1}
};
XnUInt32 nColors = 10;

void glPrintString(void *font, char *str)
{
	size_t i, l = strlen(str);

	for (i = 0; i < l; i++)
	{
		glutBitmapCharacter(font, *str++);
	}
}

float DrawLimb(XnUserID player, XnSkeletonJoint eJoint1, XnSkeletonJoint eJoint2)
{
	if (!g_UserGenerator.GetSkeletonCap().IsCalibrated(player))
	{
//        printf("not calibrated!\n");
		return 0;
	}
	if (!g_UserGenerator.GetSkeletonCap().IsTracking(player))
	{
		printf("not tracked!\n");
		return 0;
	}

	XnSkeletonJointPosition joint1, joint2;
	g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(player, eJoint1, joint1);
	g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(player, eJoint2, joint2);

	if (joint1.fConfidence < 0.5 || joint2.fConfidence < 0.5)
	{
		return 0;
	}

	XnPoint3D pt[2];
	pt[0] = joint1.position;
	pt[1] = joint2.position;
	//printf("Joint position: %f, %f, %f", pt[0].X, pt[0].Y, pt[0].Z);

	g_DepthGenerator.ConvertRealWorldToProjective(2, pt, pt);
	glVertex3i(pt[0].X, pt[0].Y, 0);
	glVertex3i(pt[1].X, pt[1].Y, 0);
	if (eJoint1 == XN_SKEL_HEAD) {
		// printf("head height: %f ", joint1.position.Y);
		return joint1.position.Y;
	} else if (eJoint2 == XN_SKEL_LEFT_FOOT) {
		float footHeight = joint2.position.Y;
		// printf("foot height: %f ", footHeight);
		// if (footHeight < 0) {
		//   // Child with feet out of bounds. Use knees.
		//   return joint1.position.Y;
		// }
		return footHeight;
	}
	return 0;

	// if (joint1.position.Y > joint2.position.Y) {
	//   return joint2.position.Y;
	// }
	// return joint1.position.Y;
}



void DrawDepthMap(const xn::DepthMetaData& dmd, const xn::SceneMetaData& smd, XnUserID player)
{
	static bool bInitialized = false;
	static GLuint depthTexID;
	static unsigned char* pDepthTexBuf;
	static int texWidth, texHeight;

	float topLeftX;
	float topLeftY;
	float bottomRightY;
	float bottomRightX;
	float texXpos;
	float texYpos;

	if (!bInitialized)
	{

		texWidth =  getClosestPowerOfTwo(dmd.XRes());
		texHeight = getClosestPowerOfTwo(dmd.YRes());

//		printf("Initializing depth texture: width = %d, height = %d\n", texWidth, texHeight);
		depthTexID = initTexture((void**)&pDepthTexBuf, texWidth, texHeight) ;

//		printf("Initialized depth texture: width = %d, height = %d\n", texWidth, texHeight);
		bInitialized = true;

		topLeftX = dmd.XRes();
		topLeftY = 0;
		bottomRightY = dmd.YRes();
		bottomRightX = 0;
		texXpos = (float)dmd.XRes() / texWidth;
		texYpos  = (float)dmd.YRes() / texHeight;

		memset(texcoords, 0, 8 * sizeof(float));
		texcoords[0] = texXpos, texcoords[1] = texYpos, texcoords[2] = texXpos, texcoords[7] = texYpos;

	}
	unsigned int nValue = 0;
	unsigned int nHistValue = 0;
	unsigned int nIndex = 0;
	unsigned int nX = 0;
	unsigned int nY = 0;
	unsigned int nNumberOfPoints = 0;
	XnUInt16 g_nXRes = dmd.XRes();
	XnUInt16 g_nYRes = dmd.YRes();

	unsigned char* pDestImage = pDepthTexBuf;

	const XnDepthPixel* pDepth = dmd.Data();
	const XnLabel* pLabels = smd.Data();

	// Calculate the accumulative histogram
	memset(g_pDepthHist, 0, MAX_DEPTH * sizeof(float));
	for (nY = 0; nY < g_nYRes; nY++)
	{
		for (nX = 0; nX < g_nXRes; nX++)
		{
			nValue = *pDepth;

			if (nValue != 0)
			{
				g_pDepthHist[nValue]++;
				nNumberOfPoints++;
			}

			pDepth++;
		}
	}

	for (nIndex = 1; nIndex < MAX_DEPTH; nIndex++)
	{
		g_pDepthHist[nIndex] += g_pDepthHist[nIndex - 1];
	}
	if (nNumberOfPoints)
	{
		for (nIndex = 1; nIndex < MAX_DEPTH; nIndex++)
		{
			g_pDepthHist[nIndex] = (unsigned int)(256 * (1.0f - (g_pDepthHist[nIndex] / nNumberOfPoints)));
		}
	}

	pDepth = dmd.Data();
	{
		XnUInt32 nIndex = 0;
		// Prepare the texture map
		for (nY = 0; nY < g_nYRes; nY++)
		{
			for (nX = 0; nX < g_nXRes; nX++, nIndex++)
			{
				nValue = *pDepth;
				XnLabel label = *pLabels;
				XnUInt32 nColorID = label % nColors;
				if (label == 0)
				{
					nColorID = nColors;
				}

				if (nValue != 0)
				{
					nHistValue = g_pDepthHist[nValue];

					pDestImage[0] = nHistValue * Colors[nColorID][0];
					pDestImage[1] = nHistValue * Colors[nColorID][1];
					pDestImage[2] = nHistValue * Colors[nColorID][2];
				}
				else
				{
					pDestImage[0] = 0;
					pDestImage[1] = 0;
					pDestImage[2] = 0;
				}

				pDepth++;
				pLabels++;
				pDestImage += 3;
			}

			pDestImage += (texWidth - g_nXRes) * 3;
		}
	}

	glBindTexture(GL_TEXTURE_2D, depthTexID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, pDepthTexBuf);

	// Display the OpenGL texture map
	glColor4f(0.75, 0.75, 0.75, 1);

	glEnable(GL_TEXTURE_2D);
	DrawTexture(dmd.XRes(), dmd.YRes(), 0, 0);
	glDisable(GL_TEXTURE_2D);

	char strLabel[40] = "";
	XnUserID aUsers[15];
	XnUInt16 nUsers = 15;
	g_UserGenerator.GetUsers(aUsers, nUsers);

	for (int i = 0; i < nUsers; ++i)
	{
		XnPoint3D com;
		g_UserGenerator.GetCoM(aUsers[i], com);
		g_DepthGenerator.ConvertRealWorldToProjective(1, &com, &com);

		std::string label = "dummy";


		DrawSkeleton(aUsers[i], com.Z, label);

		if (aUsers[i] == player) {
			sprintf(strLabel, "%d (Player), %s", aUsers[i], label.c_str());
			// printf("xnpoint values: %f, %f, %f\n", com.X, com.Y, com.Z );
		}
		else {
			sprintf(strLabel, "%d, %s", aUsers[i], label.c_str());
		}

		glColor4f(1 - Colors[i % nColors][0], 1 - Colors[i % nColors][1], 1 - Colors[i % nColors][2], 1);

		glRasterPos2i(com.X, com.Y);
		glPrintString(GLUT_BITMAP_HELVETICA_18, strLabel);
	}
}



// Draw skeleton of user
void DrawSkeleton(XnUserID player, float depth, std::string& label) {
	if (player != 0)
	{
		glBegin(GL_LINES);
		glColor4f(1 - Colors[player % nColors][0], 1 - Colors[player % nColors][1], 1 - Colors[player % nColors][2], 1);

		DrawLimb(player, XN_SKEL_HEAD, XN_SKEL_NECK);
		DrawLimb(player, XN_SKEL_NECK, XN_SKEL_LEFT_SHOULDER);
		DrawLimb(player, XN_SKEL_LEFT_SHOULDER, XN_SKEL_LEFT_ELBOW);
		DrawLimb(player, XN_SKEL_LEFT_ELBOW, XN_SKEL_LEFT_HAND);

		DrawLimb(player, XN_SKEL_NECK, XN_SKEL_RIGHT_SHOULDER);
		DrawLimb(player, XN_SKEL_RIGHT_SHOULDER, XN_SKEL_RIGHT_ELBOW);
		DrawLimb(player, XN_SKEL_RIGHT_ELBOW, XN_SKEL_RIGHT_HAND);

		DrawLimb(player, XN_SKEL_LEFT_SHOULDER, XN_SKEL_TORSO);
		DrawLimb(player, XN_SKEL_RIGHT_SHOULDER, XN_SKEL_TORSO);

		DrawLimb(player, XN_SKEL_TORSO, XN_SKEL_LEFT_HIP);
		DrawLimb(player, XN_SKEL_LEFT_HIP, XN_SKEL_LEFT_KNEE);
		DrawLimb(player, XN_SKEL_LEFT_KNEE, XN_SKEL_LEFT_FOOT);

		DrawLimb(player, XN_SKEL_TORSO, XN_SKEL_RIGHT_HIP);
		DrawLimb(player, XN_SKEL_RIGHT_HIP, XN_SKEL_RIGHT_KNEE);
		DrawLimb(player, XN_SKEL_RIGHT_KNEE, XN_SKEL_RIGHT_FOOT);

		XnSkeletonJointPosition rightHand, rightElbow, leftHand, leftElbow,
			head, knee, neck, rightShoulder, leftShoulder, torso, rightHip,
			rightKnee, rightFoot, leftHip, leftKnee, leftFoot;
		g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(player,
			XN_SKEL_LEFT_FOOT, leftFoot);
		g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(player,
			XN_SKEL_LEFT_KNEE, leftKnee);
		g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(player,
			XN_SKEL_LEFT_HIP, leftHip);
		g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(player,
			XN_SKEL_RIGHT_FOOT, rightFoot);
		g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(player,
			XN_SKEL_RIGHT_FOOT, rightFoot);
	  g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(player,
			XN_SKEL_RIGHT_KNEE, rightKnee);
		g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(player,
 			XN_SKEL_RIGHT_HIP, rightHip);
	  g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(player,
			XN_SKEL_TORSO, torso);
		g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(player,
			XN_SKEL_LEFT_SHOULDER, leftShoulder);
		g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(player,
			XN_SKEL_RIGHT_SHOULDER, rightShoulder);
		g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(player,
			XN_SKEL_NECK, neck);
		g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(player,
			XN_SKEL_RIGHT_ELBOW, rightElbow);
		g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(player,
			XN_SKEL_RIGHT_HAND, rightHand);
		g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(player,
			XN_SKEL_LEFT_ELBOW, leftElbow);
		g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(player,
			XN_SKEL_LEFT_HAND, leftHand);
		g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(player,
				XN_SKEL_HEAD, head);
		g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(player,
				XN_SKEL_LEFT_KNEE, knee);



		struct stat st = {0};
		std::string directory = "TestRealtime";

		//create the directory if it doesnt exist
		if (stat(directory.c_str(), &st) == -1) {
		    mkdir(directory.c_str(), 0700);
		}


    if(first) {
        system("rm TestRealtime/*");
    }

		std::string filename = "a21_s00_e";

		//a16_s01_e01
		//a16_s01_e02

		//a16 punch
		//a09 / a17 kick
		//a18 wave
		//a19 throw
		//a20 standing still
        //a21 pointing

		// time_t now = time(0);


		double now = std::chrono::duration<double> (
    	std::chrono::system_clock::now().time_since_epoch()
		).count();
		now = now - 1510627000.0;
        now = now * 2.0;
//        now = now / 2.0;
		int now2 = (int) (now);

		double eventFrequency = 1;

		if(now2/eventFrequency != eventTime) {

			count++;

//         std::cout << "\n-------\n START event #" << std::to_string(count) << "\n-------------" << std::endl;

			eventTime = now2/eventFrequency;

			filesInUse.push_back(filename + std::to_string(count) + ".txt");

			// n + 1 files
            if(filesInUse.size() == 5) {
//             if(filesInUse.size() == 2) {
				filesInUse.erase(filesInUse.begin());

				//real time classification
                if(handAboveHip && handVelocity) {
                    
                    std::string script1 = "~/Github/GrtWork/GrtWork/realtime_classification.sh " + std::to_string(count - 4) + " " + std::to_string(classifyCount) + " classify &";
                    system(script1.c_str());
                    handAboveHip = false;
                    handVelocity = false;
                    footVelocity = false;
                    classifyCount++;
                }
                else if(footVelocity) {
                    std::string script1 = "~/Github/GrtWork/GrtWork/realtime_classification.sh " + std::to_string(count - 4) + " " + std::to_string(classifyCount) + " classifiedKick &";
                    system(script1.c_str());
                    handAboveHip = false;
                    handVelocity = false;
                    footVelocity = false;
                    classifyCount++;
                }
			}

		}


		std::vector<XnSkeletonJointPosition> joints;
		joints.push_back(head);
		joints.push_back(neck);
		joints.push_back(rightShoulder);
		joints.push_back(rightElbow);
		joints.push_back(rightHand);
		joints.push_back(leftShoulder);
		joints.push_back(leftElbow);
		joints.push_back(leftHand);
		joints.push_back(torso);
		joints.push_back(rightHip);
		joints.push_back(rightKnee);
		joints.push_back(rightFoot);
		joints.push_back(leftHip);
		joints.push_back(leftKnee);
		joints.push_back(leftFoot);


		if(first) {

			prevHand = rightHand;
			prevFoot = rightFoot;

			first = false;
		}

		if(rightHand.position.Y > torso.position.Y ) {
			handAboveHip = true;
		}
        

		//write joint data to every file in use
		for(std::string s : filesInUse) {
			std::ofstream myfile;

			myfile.open (directory + "/" + s, std::ios_base::app);

			for(XnSkeletonJointPosition joint : joints){
				myfile << std::to_string(joint.position.X - head.position.X) + " " +
					std::to_string(joint.position.Y - head.position.Y) + " " + std::to_string(joint.position.Z - head.position.Z) + "\n";
			}
		}



		//add velocities to text file!

		// myfile << std::to_string(rightHand.position.X - prevHand.position.X) + " " +
		// std::to_string(rightHand.position.Y - prevHand.position.Y) + " " +
		// std::to_string(rightHand.position.Z - prevHand.position.Z) + "\n";
		//
		// myfile << std::to_string(rightFoot.position.X - prevFoot.position.X) + " " +
		// std::to_string(rightFoot.position.Y - prevFoot.position.Y) + " " +
		// std::to_string(rightFoot.position.Z - prevFoot.position.Z) + "\n";

		float handVel = pow(rightHand.position.X - prevHand.position.X, 2) +
		pow(rightHand.position.Y - prevHand.position.Y, 2) +
		pow(rightHand.position.Z- prevHand.position.Z, 2);


		if(handVel >3000) {
			handVelocity = true;
		}
        
        
        //foot velocity
        float footVel = pow(rightFoot.position.X - prevFoot.position.X, 2) +
        pow(rightFoot.position.Y - prevFoot.position.Y, 2) +
        pow(rightFoot.position.Z- prevFoot.position.Z, 2);
        
        if(footVel >12500) {
            footVelocity = true;
        }


		prevHand = rightHand;
		prevFoot = rightFoot;

		//calculate angles
		float RightHandX = rightHand.position.X;
		float RightHandY = rightHand.position.Y;
		float RightHandZ = rightHand.position.Z;

		float RightElbowX = rightElbow.position.X;
		float RightElbowY = rightElbow.position.Y;
		float RightElbowZ = rightElbow.position.Z;

		float LeftHandX = leftHand.position.X;
		float LeftHandY = leftHand.position.Y;
		float LeftHandZ = leftHand.position.Z;

		float LeftElbowX = leftElbow.position.X;
		float LeftElbowY = leftElbow.position.Y;
		float LeftElbowZ = leftElbow.position.Z;


		float deltaX = RightElbowX - RightHandX;
		float deltaY = RightElbowY - RightHandY;
		float angleHandElbow = atan2(deltaY,deltaX);

		//calculate angles
		float RightFootX = rightFoot.position.X;
		float RightFootY = rightFoot.position.Y;
		float RightFootZ = rightFoot.position.Z;

		float RightKneeX = rightKnee.position.X;
		float RightKneeY = rightKnee.position.Y;
		float RightKneeZ = rightKnee.position.Z;


		deltaX = RightKneeX - RightFootX;
		deltaY = RightKneeY - RightFootY;
		float angleFootKnee = atan2(deltaY,deltaX);


		bool waving = false;

		if(rightHand.position.Y > rightElbow.position.Y) {
			waving = true;
		}


		float height = (head.position.Y - knee.position.Y);

		float thresholdHeight = 1100;

		char strLabel[20] = "";


		if (height > thresholdHeight) {
			if(waving ) {
				label = "Adult Waving";
			} else {
				label = "Adult";
			}

		} else if (depth != 0) {
			if(waving) {
				label = "Child Waving";
			} else {
				label = "Child";
			}
		}

		XnPoint3D com;
		g_UserGenerator.GetCoM(player, com);
		glRasterPos2i(com.X, com.Y);
		glPrintString(GLUT_BITMAP_HELVETICA_18, strLabel);
		glEnd();
	}
}
